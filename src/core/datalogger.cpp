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
#include "datalogger.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

DataLogger::DataLogger(QObject *parent) : QObject{parent} {
}

DataLogger::~DataLogger() {
    stopLogging();
}

void DataLogger::startLogging(const QString &prefix) {
    stopLogging();

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString fileName = QString("%1_%2.txt").arg(prefix, timestamp);

    // Use configured directory or current directory if not set
    QString fullPath = fileName;
    if (!m_logDirectory.isEmpty()) {
        QDir dir(m_logDirectory);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        fullPath = dir.filePath(fileName);
    }

    m_file.setFileName(fullPath);

    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_isLogging = true;
        qDebug() << "Logging to" << QFileInfo(m_file).absoluteFilePath();
    } else {
        qWarning() << "Failed to open log file:" << fullPath << m_file.errorString();
    }
}

void DataLogger::setLogDirectory(const QString &path) {
    m_logDirectory = path;
}

void DataLogger::stopLogging() {
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_isLogging = false;
}

void DataLogger::writeData(const QByteArray &data) {
    if (m_isLogging && m_file.isOpen()) {
        m_file.write(data);
        m_file.flush();
    }
}
