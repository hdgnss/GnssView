#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <memory>

#include "src/core/TransportPluginLoader.h"
#include "src/transports/SerialTransport.h"
#include "src/transports/TcpClientTransport.h"
#include "src/transports/UdpServerTransport.h"

namespace hdgnss {

class AppSettings;
class ITransport;
class ITransportPlugin;

class TransportViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList serialPorts READ serialPorts NOTIFY serialPortsChanged)
    Q_PROPERTY(QStringList uartPorts READ serialPorts NOTIFY serialPortsChanged)
    Q_PROPERTY(QString uartStatus READ uartStatus NOTIFY statusesChanged)
    Q_PROPERTY(QString tcpStatus READ tcpStatus NOTIFY statusesChanged)
    Q_PROPERTY(QString udpStatus READ udpStatus NOTIFY statusesChanged)
    Q_PROPERTY(QString activeTransport READ activeTransport WRITE setActiveTransport NOTIFY activeTransportChanged)
    Q_PROPERTY(QString lockedTransport READ lockedTransport NOTIFY lockedTransportChanged)
    Q_PROPERTY(QVariantList availableTransports READ availableTransports NOTIFY availableTransportsChanged)
    Q_PROPERTY(QStringList transportPluginLoadErrors READ transportPluginLoadErrors NOTIFY availableTransportsChanged)
    Q_PROPERTY(QStringList transportPluginSearchPaths READ transportPluginSearchPaths NOTIFY availableTransportsChanged)

public:
    explicit TransportViewModel(QObject *parent = nullptr);

    QStringList serialPorts() const;
    QString uartStatus() const;
    QString tcpStatus() const;
    QString udpStatus() const;
    QString activeTransport() const;
    QString lockedTransport() const;
    QVariantList availableTransports() const;
    QStringList transportPluginLoadErrors() const;
    QStringList transportPluginSearchPaths() const;
    void setActiveTransport(const QString &transportName);
    void setAppSettings(AppSettings *settings);

    SerialTransport *serialTransport();
    TcpClientTransport *tcpTransport();
    UdpServerTransport *udpTransport();
    QList<ITransport *> allTransports() const;

    Q_INVOKABLE QVariantMap transportDescriptor(const QString &transportName) const;
    Q_INVOKABLE QString transportStatus(const QString &transportName) const;
    Q_INVOKABLE bool transportOpen(const QString &transportName) const;
    Q_INVOKABLE QVariantMap transportStoredSettings(const QString &transportName) const;
    Q_INVOKABLE void refreshTransportDescriptors();
    Q_INVOKABLE void refreshSerialPorts();
    Q_INVOKABLE void refreshUartPorts();
    Q_INVOKABLE void reloadTransportPlugins();
    Q_INVOKABLE QString probeTransport(const QString &transportName);
    Q_INVOKABLE bool openTransport(const QString &transportName, const QVariantMap &settings = QVariantMap{});
    Q_INVOKABLE void closeTransport(const QString &transportName);
    Q_INVOKABLE bool openUart(const QVariantMap &settings);
    Q_INVOKABLE bool openUart(const QString &port, int baud, int dataBits, int stopBits, const QString &parity, const QString &flowControl);
    Q_INVOKABLE void closeUart();
    Q_INVOKABLE bool openTcp(const QVariantMap &settings);
    Q_INVOKABLE bool openTcp(const QString &host, int port);
    Q_INVOKABLE void closeTcp();
    Q_INVOKABLE bool openUdp(const QVariantMap &settings);
    Q_INVOKABLE bool openUdpServer(const QString &address, int port);
    Q_INVOKABLE void closeUdp();
    Q_INVOKABLE void closeUdpServer();
    Q_INVOKABLE bool send(const QByteArray &payload, const QString &target = QStringLiteral("Active"));

signals:
    void serialPortsChanged();
    void statusesChanged();
    void activeTransportChanged();
    void lockedTransportChanged();
    void availableTransportsChanged();
    void transportDescriptorChanged(const QString &transportName);
    void transportRegistered(ITransport *transport);

private:
    struct PluginTransportEntry {
        QString transportId;
        QString displayName;
        QStringList hardwareInterfaces;
        ITransportPlugin *plugin = nullptr;
        QObject *pluginObject = nullptr;
        ITransport *transport = nullptr;
    };

    struct BuiltinTransportEntry {
        QString transportId;
        QString displayName;
        QStringList capabilities;
        ITransport *transport = nullptr;
    };

    QString pluginSettingsId(const QString &transportName) const;
    QVariantList configurableFields(QObject *pluginObject, const QString &settingsId) const;
    QVariantMap descriptorForBuiltin(const BuiltinTransportEntry &entry) const;
    QVariantMap descriptorForPlugin(const PluginTransportEntry &entry) const;
    const BuiltinTransportEntry *findBuiltinEntry(const QString &transportName) const;
    BuiltinTransportEntry *findBuiltinEntry(const QString &transportName);
    const PluginTransportEntry *findPluginEntry(const QString &transportName) const;
    PluginTransportEntry *findPluginEntry(const QString &transportName);
    ITransport *findTransport(const QString &transportName) const;
    void hookTransport(ITransport *transport);
    void handleTransportOpenChanged();
    QString currentOpenTransportName() const;
    void setLockedTransport(const QString &transportName);

    QStringList m_serialPorts;
    QString m_activeTransport = QStringLiteral("UART");
    QString m_lockedTransport;
    AppSettings *m_settings = nullptr;
    SerialTransport m_serial;
    TcpClientTransport m_tcp;
    UdpServerTransport m_udp;
    QList<BuiltinTransportEntry> m_builtinTransports;
    QList<PluginTransportEntry> m_pluginTransports;
    TransportPluginLoader m_transportPluginLoader;
    QStringList m_transportPluginLoadErrors;
    QStringList m_transportPluginSearchPaths;
};

}  // namespace hdgnss
