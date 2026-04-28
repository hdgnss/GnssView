#include "UpdateChecker.h"

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <algorithm>

#include "hdgnss_config.h"

namespace hdgnss {

namespace {

QString stripTagPrefix(QString version) {
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        version.remove(0, 1);
    }

    const int buildIndex = version.indexOf(QLatin1Char('+'));
    if (buildIndex >= 0) {
        version.truncate(buildIndex);
    }
    const int prereleaseIndex = version.indexOf(QLatin1Char('-'));
    if (prereleaseIndex >= 0) {
        version.truncate(prereleaseIndex);
    }
    return version.trimmed();
}

QList<int> numericVersionParts(const QString &version) {
    const QString cleaned = stripTagPrefix(version);
    const QStringList textParts = cleaned.split(QLatin1Char('.'), Qt::KeepEmptyParts);
    QList<int> parts;
    parts.reserve(std::max<qsizetype>(qsizetype{3}, textParts.size()));

    for (const QString &textPart : textParts) {
        int value = 0;
        bool sawDigit = false;
        for (const QChar ch : textPart) {
            if (!ch.isDigit()) {
                break;
            }
            sawDigit = true;
            value = value * 10 + ch.digitValue();
        }
        parts.append(sawDigit ? value : 0);
    }

    while (parts.size() < 3) {
        parts.append(0);
    }
    return parts;
}

QString jsonString(const QJsonObject &object, const QString &key) {
    return object.value(key).toString().trimmed();
}

int platformAssetScore(const QString &assetName) {
    const QString name = assetName.toLower();
    int score = 1;

#if defined(Q_OS_WIN)
    if (name.endsWith(QStringLiteral(".exe")) || name.endsWith(QStringLiteral(".msi"))) score += 60;
    if (name.contains(QStringLiteral("setup")) || name.contains(QStringLiteral("installer"))) score += 25;
    if (name.contains(QStringLiteral("win")) || name.contains(QStringLiteral("windows"))) score += 30;
    if (name.contains(QStringLiteral("x64")) || name.contains(QStringLiteral("amd64"))) score += 10;
#elif defined(Q_OS_MACOS)
    if (name.endsWith(QStringLiteral(".dmg"))) score += 70;
    if (name.endsWith(QStringLiteral(".zip"))) score += 20;
    if (name.contains(QStringLiteral("mac")) || name.contains(QStringLiteral("osx")) || name.contains(QStringLiteral("darwin"))) score += 30;
    if (name.contains(QStringLiteral("arm64")) || name.contains(QStringLiteral("universal")) || name.contains(QStringLiteral("x86_64"))) score += 8;
#else
    if (name.endsWith(QStringLiteral(".appimage")) || name.endsWith(QStringLiteral(".deb")) || name.endsWith(QStringLiteral(".rpm"))) score += 50;
    if (name.endsWith(QStringLiteral(".tar.gz")) || name.endsWith(QStringLiteral(".tgz"))) score += 25;
    if (name.contains(QStringLiteral("linux"))) score += 30;
#endif

    return score;
}

}  // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_statusText(QStringLiteral("Update status has not been checked")) {
}

QString UpdateChecker::currentVersion() const {
    return applicationVersion();
}

QString UpdateChecker::latestVersion() const {
    return m_latestVersion;
}

QString UpdateChecker::releaseUrl() const {
    return m_releaseUrl;
}

QString UpdateChecker::downloadUrl() const {
    return m_downloadUrl;
}

QString UpdateChecker::releaseNotes() const {
    return m_releaseNotes;
}

QString UpdateChecker::statusText() const {
    return m_statusText;
}

QString UpdateChecker::lastError() const {
    return m_lastError;
}

bool UpdateChecker::checking() const {
    return m_checking;
}

bool UpdateChecker::hasChecked() const {
    return m_hasChecked;
}

bool UpdateChecker::hasUpdate() const {
    return m_hasUpdate;
}

void UpdateChecker::checkForUpdates(bool silent) {
    if (m_checking) {
        return;
    }

    m_silent = silent;
    m_lastError.clear();
    setChecking(true);
    setStatusText(QStringLiteral("Checking GitHub releases..."));
    emit resultChanged();

    const QUrl apiUrl(QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
                          .arg(QStringLiteral(HDGNSS_GITHUB_OWNER), QStringLiteral(HDGNSS_GITHUB_REPO)));
    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", QStringLiteral("%1/%2").arg(QStringLiteral(HDGNSS_APP_NAME), applicationVersion()).toUtf8());
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleNetworkReply(reply);
    });
}

bool UpdateChecker::openReleasePage() const {
    const QString target = m_releaseUrl.isEmpty() ? releasesPageUrl() : m_releaseUrl;
    return QDesktopServices::openUrl(QUrl(target));
}

bool UpdateChecker::openDownloadPage() const {
    if (!m_downloadUrl.isEmpty()) {
        return QDesktopServices::openUrl(QUrl(m_downloadUrl));
    }
    return openReleasePage();
}

QString UpdateChecker::applicationVersion() {
    return QStringLiteral(HDGNSS_APP_VERSION);
}

int UpdateChecker::compareVersions(const QString &left, const QString &right) {
    const QList<int> leftParts = numericVersionParts(left);
    const QList<int> rightParts = numericVersionParts(right);
    const int count = std::max(leftParts.size(), rightParts.size());
    for (int index = 0; index < count; ++index) {
        const int leftValue = index < leftParts.size() ? leftParts.at(index) : 0;
        const int rightValue = index < rightParts.size() ? rightParts.at(index) : 0;
        if (leftValue < rightValue) {
            return -1;
        }
        if (leftValue > rightValue) {
            return 1;
        }
    }
    return 0;
}

void UpdateChecker::handleNetworkReply(QNetworkReply *reply) {
    reply->deleteLater();
    setChecking(false);

    if (reply->error() != QNetworkReply::NoError) {
        failCheck(reply->errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        failCheck(QStringLiteral("GitHub release response is not valid JSON"));
        return;
    }

    const QJsonObject release = document.object();
    const QString tagName = jsonString(release, QStringLiteral("tag_name"));
    const QString latest = stripTagPrefix(tagName);
    if (latest.isEmpty()) {
        failCheck(QStringLiteral("GitHub release response did not include a tag name"));
        return;
    }

    m_latestVersion = latest;
    m_releaseUrl = jsonString(release, QStringLiteral("html_url"));
    if (m_releaseUrl.isEmpty()) {
        m_releaseUrl = releasesPageUrl();
    }
    m_releaseNotes = release.value(QStringLiteral("body")).toString();
    m_downloadUrl = preferredAssetUrl(release.value(QStringLiteral("assets")).toArray());
    m_lastError.clear();
    m_hasChecked = true;
    m_hasUpdate = compareVersions(applicationVersion(), m_latestVersion) < 0;

    if (m_hasUpdate) {
        setStatusText(QStringLiteral("GnssView %1 is available").arg(m_latestVersion));
    } else {
        setStatusText(QStringLiteral("GnssView %1 is up to date").arg(applicationVersion()));
    }

    emit resultChanged();
    if (m_hasUpdate) {
        emit updateAvailable(m_latestVersion, m_releaseUrl, m_releaseNotes);
    } else if (!m_silent) {
        emit noUpdateAvailable();
    }
}

void UpdateChecker::failCheck(const QString &message) {
    m_latestVersion.clear();
    m_releaseUrl.clear();
    m_downloadUrl.clear();
    m_releaseNotes.clear();
    m_lastError = message.trimmed().isEmpty() ? QStringLiteral("Unknown update check error") : message.trimmed();
    m_hasChecked = true;
    m_hasUpdate = false;
    setStatusText(QStringLiteral("Update check failed: %1").arg(m_lastError));
    emit resultChanged();
    if (!m_silent) {
        emit checkFailed(m_lastError);
    }
}

void UpdateChecker::setChecking(bool checking) {
    if (m_checking == checking) {
        return;
    }
    m_checking = checking;
    emit checkingChanged();
}

void UpdateChecker::setStatusText(const QString &statusText) {
    if (m_statusText == statusText) {
        return;
    }
    m_statusText = statusText;
    emit statusTextChanged();
}

QString UpdateChecker::preferredAssetUrl(const QJsonArray &assets) const {
    QString fallbackUrl;
    QString bestUrl;
    int bestScore = -1;

    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString url = jsonString(asset, QStringLiteral("browser_download_url"));
        if (url.isEmpty()) {
            continue;
        }
        if (fallbackUrl.isEmpty()) {
            fallbackUrl = url;
        }

        const QString name = jsonString(asset, QStringLiteral("name"));
        const int score = platformAssetScore(name);
        if (score > bestScore) {
            bestScore = score;
            bestUrl = url;
        }
    }

    return bestUrl.isEmpty() ? fallbackUrl : bestUrl;
}

QString UpdateChecker::releasesPageUrl() const {
    return QStringLiteral("https://github.com/%1/%2/releases")
        .arg(QStringLiteral(HDGNSS_GITHUB_OWNER), QStringLiteral(HDGNSS_GITHUB_REPO));
}

}  // namespace hdgnss
