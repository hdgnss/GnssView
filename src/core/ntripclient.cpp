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
#include "ntripclient.h"

#include <QDebug>
#include <QTextStream>

NTRIPClient::NTRIPClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_port(0), m_connected(false) {
    connect(m_socket, &QTcpSocket::readyRead, this, &NTRIPClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &NTRIPClient::onConnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NTRIPClient::onError);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_connected = false;
        emit disconnected();
    });
}

NTRIPClient::~NTRIPClient() {
    disconnectFromServer();
    delete m_socket;
}

bool NTRIPClient::connectToServer(const QString &host, int port, const QString &mountPoint, const QString &username,
                                  const QString &password) {
    qDebug() << "Connecting to NTRIP server:" << host << ":" << port << ", mountpoint:" << mountPoint;

    m_host = host;
    m_port = port;
    m_mountPoint = mountPoint;
    m_username = username;
    m_password = password;

    m_socket->connectToHost(m_host, m_port);
    return m_socket->waitForConnected(5000);  // 5 seconds timeout
}

void NTRIPClient::disconnectFromServer() {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_connected = false;
}

bool NTRIPClient::isConnected() const {
    return m_connected;
}

void NTRIPClient::onReadyRead() {
    QByteArray data = m_socket->readAll();

    // Check if we're receiving HTTP headers (first response)
    if (!m_connected) {
        // Look for end of HTTP headers
        int headerEnd = data.indexOf("\r\n\r\n");
        if (headerEnd >= 0) {
            QString headers = QString::fromLatin1(data.left(headerEnd));
            qDebug() << "HTTP Headers received:" << headers;

            // Check for HTTP error
            if (headers.startsWith("HTTP/1.") && !headers.contains("200 OK")) {
                emit connectionError("HTTP Error: " + headers.split("\r\n").first());
                disconnectFromServer();
                return;
            }

            // Remove headers from data
            qDebug() << "Removing HTTP headers, remaining data size:" << (data.size() - headerEnd - 4);
            data = data.mid(headerEnd + 4);
            m_connected = true;
            emit connected();
        } else {
            // Still receiving headers, wait for more data
            qDebug() << "Incomplete HTTP headers, waiting for more data";
            return;
        }
    }

    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void NTRIPClient::onError(QAbstractSocket::SocketError socketError) {
    QString errorMessage = m_socket->errorString();
    emit connectionError(errorMessage);
    m_connected = false;
}

void NTRIPClient::onConnected() {
    sendRequest();
}

void NTRIPClient::sendRequest() {
    // Create NTRIP request
    QString request = QString(
                          "GET /%1 HTTP/1.0\r\n"
                          "User-Agent: NTRIP RTCMDecoder/1.0\r\n")
                          .arg(m_mountPoint);

    // Add authentication if provided
    if (!m_username.isEmpty()) {
        QString auth = m_username;
        if (!m_password.isEmpty()) {
            auth += ":" + m_password;
        }
        QByteArray authBytes = auth.toUtf8().toBase64();
        request += QString("Authorization: Basic %1\r\n").arg(QString::fromLatin1(authBytes));
    }

    request += "Accept: */*\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    qDebug() << "Sending NTRIP request";
    m_socket->write(request.toUtf8());
}

void NTRIPClient::sendGnssPosition(const QString &nmea) {
    if (m_connected && m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Sending NMEA position to NTRIP server:" << nmea;
        m_socket->write(nmea.toUtf8());
        if (!nmea.endsWith("\r\n")) {
            m_socket->write("\r\n");
        }
    }
}