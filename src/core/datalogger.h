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
#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QTextStream>

class DataLogger : public QObject {
    Q_OBJECT
public:
    explicit DataLogger(QObject *parent = nullptr);
    ~DataLogger();

    void startLogging(const QString &prefix);
    void stopLogging();
    void writeData(const QByteArray &data);
    void setLogDirectory(const QString &path);

private:
    QFile m_file;
    bool m_isLogging = false;
    QString m_logDirectory;
};

#endif  // DATALOGGER_H
