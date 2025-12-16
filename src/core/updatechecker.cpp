/*
 * Copyright (C) 2025 HDGNSS
 *
 *  H   H  DDDD   GGG  N   N  SSSS  SSSS
 *  H   H  D   D G     NN  N  S     S
 *  HHHHH  D   D G  GG N N N   SSS   SSS
 *  H   H  D   D G   G N  NN      S     S
 *  H   H  DDDD   GGG  N   N  SSSS  SSSS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "updatechecker.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_silent(false) {
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::handleNetworkReply);
}

UpdateChecker::~UpdateChecker() {
}

QString UpdateChecker::currentVersion() {
    return APP_VERSION_STRING;
}

int UpdateChecker::compareVersions(const QString &v1, const QString &v2) {
    QStringList parts1 = v1.split('.');
    QStringList parts2 = v2.split('.');

    // Pad with zeros if needed
    while (parts1.size() < 3) {
        parts1.append("0");
    }
    while (parts2.size() < 3) {
        parts2.append("0");
    }

    for (int i = 0; i < 3; ++i) {
        int num1 = parts1[i].toInt();
        int num2 = parts2[i].toInt();

        if (num1 < num2) {
            return -1;
        }
        if (num1 > num2) {
            return 1;
        }
    }

    return 0;  // Equal
}

void UpdateChecker::checkForUpdates(bool silent) {
    m_silent = silent;

    // Build GitHub API URL for latest release
    QString apiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(GITHUB_OWNER).arg(GITHUB_REPO);

    QUrl url(apiUrl);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    request.setRawHeader("User-Agent", "GnssView-UpdateChecker");

    m_networkManager->get(request);
}

void UpdateChecker::handleNetworkReply(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        qWarning() << "Update check failed:" << errorMsg;

        if (!m_silent) {
            emit checkFailed(errorMsg);
        }
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull() || !doc.isObject()) {
        if (!m_silent) {
            emit checkFailed("Failed to parse update response");
        }
        return;
    }

    QJsonObject json = doc.object();

    // Extract version from tag_name (usually "v1.2.3" format)
    QString tagName = json["tag_name"].toString();
    QString latestVersion = tagName;

    // Remove 'v' prefix if present
    if (latestVersion.startsWith('v') || latestVersion.startsWith('V')) {
        latestVersion = latestVersion.mid(1);
    }

    QString releaseUrl = json["html_url"].toString();
    QString releaseNotes = json["body"].toString();

    qDebug() << "Current version:" << currentVersion();
    qDebug() << "Latest version:" << latestVersion;

    // Compare versions
    int cmp = compareVersions(currentVersion(), latestVersion);

    if (cmp < 0) {
        // New version available
        emit updateAvailable(latestVersion, releaseUrl, releaseNotes);
    } else {
        // Already up to date or newer (dev version)
        if (!m_silent) {
            emit noUpdateAvailable();
        }
    }
}
