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
#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTcpSocket>

class NTRIPClient : public QObject {
    Q_OBJECT

public:
    explicit NTRIPClient(QObject *parent = nullptr);
    ~NTRIPClient();

    // Connect to NTRIP server
    bool connectToServer(const QString &host, int port, const QString &mountPoint, const QString &username = QString(),
                         const QString &password = QString());
    void disconnectFromServer();
    bool isConnected() const;
    void sendGnssPosition(const QString &nmea);

signals:
    void dataReceived(const QByteArray &data);
    void connectionError(const QString &errorMsg);
    void connected();
    void disconnected();

private slots:
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onConnected();

private:
    QTcpSocket *m_socket;
    QString m_host;
    int m_port;
    QString m_mountPoint;
    QString m_username;
    QString m_password;
    bool m_connected;

    void sendRequest();
};