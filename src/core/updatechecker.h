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
#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

// Version constants generated from CMakeLists.txt
#include "config.h"

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();

    /**
     * @brief Check for updates from GitHub releases
     * @param silent If true, don't emit noUpdateAvailable signal (for auto-check
     * on startup)
     */
    void checkForUpdates(bool silent = false);

    /**
     * @brief Get the current application version string
     * @return Version string in format "major.minor.patch"
     */
    static QString currentVersion();

    /**
     * @brief Compare two version strings
     * @return -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2
     */
    static int compareVersions(const QString &v1, const QString &v2);

signals:
    /**
     * @brief Emitted when a newer version is available
     * @param version The new version string
     * @param releaseUrl URL to the release page
     * @param releaseNotes Release notes/changelog
     */
    void updateAvailable(const QString &version, const QString &releaseUrl, const QString &releaseNotes);

    /**
     * @brief Emitted when current version is up to date
     */
    void noUpdateAvailable();

    /**
     * @brief Emitted when the update check fails
     * @param error Error message
     */
    void checkFailed(const QString &error);

private slots:
    void handleNetworkReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    bool m_silent;

    // GitHub repository configuration
    static constexpr const char *GITHUB_OWNER = "Hdgnss";
    static constexpr const char *GITHUB_REPO = "GnssView";
};

#endif  // UPDATECHECKER_H
