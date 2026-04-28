#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QJsonArray;
class QNetworkReply;

namespace hdgnss {

class UpdateChecker : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY resultChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY resultChanged)
    Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY resultChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY resultChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY resultChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(bool hasChecked READ hasChecked NOTIFY resultChanged)
    Q_PROPERTY(bool hasUpdate READ hasUpdate NOTIFY resultChanged)

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    QString currentVersion() const;
    QString latestVersion() const;
    QString releaseUrl() const;
    QString downloadUrl() const;
    QString releaseNotes() const;
    QString statusText() const;
    QString lastError() const;
    bool checking() const;
    bool hasChecked() const;
    bool hasUpdate() const;

    Q_INVOKABLE void checkForUpdates(bool silent);
    Q_INVOKABLE bool openReleasePage() const;
    Q_INVOKABLE bool openDownloadPage() const;

    static QString applicationVersion();
    static int compareVersions(const QString &left, const QString &right);

signals:
    void updateAvailable(const QString &version, const QString &releaseUrl, const QString &releaseNotes);
    void noUpdateAvailable();
    void checkFailed(const QString &error);
    void checkingChanged();
    void statusTextChanged();
    void resultChanged();

private:
    void handleNetworkReply(QNetworkReply *reply);
    void failCheck(const QString &message);
    void setChecking(bool checking);
    void setStatusText(const QString &statusText);
    QString preferredAssetUrl(const QJsonArray &assets) const;
    QString releasesPageUrl() const;

    QNetworkAccessManager m_networkManager;
    QString m_latestVersion;
    QString m_releaseUrl;
    QString m_downloadUrl;
    QString m_releaseNotes;
    QString m_statusText;
    QString m_lastError;
    bool m_checking = false;
    bool m_hasChecked = false;
    bool m_hasUpdate = false;
    bool m_silent = false;
};

}  // namespace hdgnss
