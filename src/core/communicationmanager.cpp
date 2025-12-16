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
#include "communicationmanager.h"

#include <QDebug>
#include <QNetworkDatagram>

CommunicationManager::CommunicationManager(QObject *parent) : QObject(parent) {
    m_serialA = new QSerialPort(this);
    m_serialB = new QSerialPort(this);
    m_tcpSocket = new QTcpSocket(this);
    m_udpSocket = new QUdpSocket(this);

    connect(m_serialA, &QSerialPort::readyRead, this, &CommunicationManager::handleSerialAReadyRead);
    connect(m_serialB, &QSerialPort::readyRead, this, &CommunicationManager::handleSerialBReadyRead);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &CommunicationManager::handleTcpReadyRead);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &CommunicationManager::handleUdpReadyRead);
}

CommunicationManager::~CommunicationManager() {
    cleanup();
}

void CommunicationManager::cleanup() {
    disconnectSerial(0);  // Disconnect Serial A
    disconnectSerial(1);  // Disconnect Serial B
    disconnectTcp();
    unbindUdp();
}

// Serial
bool CommunicationManager::connectSerial(const QString &portName, int baudRate, int channel) {
    QSerialPort *s = (channel == 1) ? m_serialB : m_serialA;

    if (s->isOpen()) {
        s->close();
    }

    s->setPortName(portName);
    s->setBaudRate(baudRate);

    if (s->open(QIODevice::ReadWrite)) {
        if (channel == 1) {
            emit serialBStatusChanged(true, "Connected to " + portName);
        } else {
            emit serialAStatusChanged(true, "Connected to " + portName);
        }
        return true;
    } else {
        if (channel == 1) {
            emit serialBStatusChanged(false, "Failed to open " + portName);
        } else {
            emit serialAStatusChanged(false, "Failed to open " + portName);
        }
        emit errorOccurred(s->errorString());
        return false;
    }
}

void CommunicationManager::disconnectSerial(int channel) {
    QSerialPort *s = (channel == 1) ? m_serialB : m_serialA;
    if (s->isOpen()) {
        s->close();
        if (channel == 1) {
            emit serialBStatusChanged(false, "Disconnected");
        } else {
            emit serialAStatusChanged(false, "Disconnected");
        }
    }
}

bool CommunicationManager::isSerialConnected(int channel) const {
    QSerialPort *s = (channel == 1) ? m_serialB : m_serialA;
    return s->isOpen();
}

// TCP
bool CommunicationManager::connectTcp(const QString &host, int port) {
    if (m_tcpSocket->isOpen()) {
        m_tcpSocket->close();
    }
    m_tcpSocket->connectToHost(host, port);
    if (m_tcpSocket->waitForConnected(3000)) {
        emit tcpStatusChanged(true, QString("Connected to %1:%2").arg(host).arg(port));
        return true;
    } else {
        emit tcpStatusChanged(false, "TCP Connect Failed");
        emit errorOccurred(m_tcpSocket->errorString());
        return false;
    }
}

void CommunicationManager::disconnectTcp() {
    if (m_tcpSocket->isOpen()) {
        m_tcpSocket->close();
        emit tcpStatusChanged(false, "TCP Disconnected");
    }
}

bool CommunicationManager::isTcpConnected() const {
    return m_tcpSocket->state() == QAbstractSocket::ConnectedState;
}

// UDP
bool CommunicationManager::bindUdp(int port) {
    if (m_udpSocket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "UDP already open or bound, aborting/closing first.";
        m_udpSocket->abort();
        m_udpSocket->close();
    }

    if (m_udpSocket->bind(QHostAddress::Any, port)) {
        emit udpStatusChanged(true, QString("Bound to UDP Port %1").arg(port));
        return true;
    } else {
        qDebug() << "UDP bind failed:" << m_udpSocket->errorString();
        emit udpStatusChanged(false, "UDP Bind Failed");
        emit errorOccurred(m_udpSocket->errorString());
        return false;
    }
}

void CommunicationManager::unbindUdp() {
    // Force reset regardless of state
    m_udpSocket->abort();
    m_udpSocket->close();
    emit udpStatusChanged(false, "UDP Unbound");
}

bool CommunicationManager::isUdpBound() const {
    return m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState;
}

// Send Data
void CommunicationManager::sendSerial(const QByteArray &data, int channel) {
    if (channel == 0 || channel == -1) {
        if (m_serialA && m_serialA->isOpen()) {
            m_serialA->write(data);
        }
    }
    if (channel == 1 || channel == -1) {
        if (m_serialB && m_serialB->isOpen()) {
            m_serialB->write(data);
        }
    }
}

void CommunicationManager::sendTcp(const QByteArray &data) {
    if (m_tcpSocket->isOpen()) {
        m_tcpSocket->write(data);
    }
}

void CommunicationManager::sendUdp(const QByteArray &data, const QString &host, int port) {
    if (m_udpSocket->state() == QAbstractSocket::BoundState) {
        QHostAddress addr(host.isEmpty() ? "127.0.0.1" : host);
        m_udpSocket->writeDatagram(data, addr, port);
    }
}

// Receive Slots
void CommunicationManager::handleSerialAReadyRead() {
    QByteArray data = m_serialA->readAll();
    emit serialADataReceived(data);
}

void CommunicationManager::handleSerialBReadyRead() {
    QByteArray data = m_serialB->readAll();
    emit serialBDataReceived(data);
}

void CommunicationManager::handleTcpReadyRead() {
    QByteArray data = m_tcpSocket->readAll();
    emit tcpDataReceived(data);
}

void CommunicationManager::handleUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());
        emit udpDataReceived(datagram);
    }
}
