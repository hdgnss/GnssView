#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>

class UdpServer : public QObject
{
    Q_OBJECT
public:
    explicit UdpServer(QObject *parent = nullptr);
    
    bool startServer(quint16 port);
    void stopServer();
    bool isListening() const;

signals:
    void newDataReceived(const QByteArray &data, const QHostAddress &sender, quint16 senderPort);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket *m_socket;
};

#endif // UDPSERVER_H