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
#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QUdpSocket>

class CommunicationManager : public QObject {
    Q_OBJECT
public:
    explicit CommunicationManager(QObject *parent = nullptr);
    ~CommunicationManager();

    // Serial Channel: 0 = A, 1 = B
    bool connectSerial(const QString &portName, int baudRate, int channel = 0);
    void disconnectSerial(int channel = 0);
    bool isSerialConnected(int channel = 0) const;

    bool connectTcp(const QString &host, int port);
    void disconnectTcp();
    bool isTcpConnected() const;

    bool bindUdp(int port);
    void unbindUdp();
    bool isUdpBound() const;

    void cleanup();  // Close all

    // Send methods
    void sendSerial(const QByteArray &data,
                    int channel = -1);  // -1 = Both, 0=A, 1=B
    void sendTcp(const QByteArray &data);
    void sendUdp(const QByteArray &data, const QString &host = QString(), int port = 0);

signals:
    void serialADataReceived(const QByteArray &data);
    void serialBDataReceived(const QByteArray &data);
    void tcpDataReceived(const QByteArray &data);
    void udpDataReceived(const QByteArray &data);

    void errorOccurred(const QString &error);

    void serialAStatusChanged(bool connected, const QString &msg);
    void serialBStatusChanged(bool connected, const QString &msg);
    void tcpStatusChanged(bool connected, const QString &msg);
    void udpStatusChanged(bool bound, const QString &msg);

private slots:
    void handleSerialAReadyRead();
    void handleSerialBReadyRead();
    void handleTcpReadyRead();
    void handleUdpReadyRead();

private:
    QSerialPort *m_serialA = nullptr;
    QSerialPort *m_serialB = nullptr;
    QTcpSocket *m_tcpSocket = nullptr;
    QUdpSocket *m_udpSocket = nullptr;
};

#endif  // COMMUNICATIONMANAGER_H
