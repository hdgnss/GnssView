#include "TransportViewModel.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QSerialPortInfo>
#include <QSet>

#include "hdgnss/IConfigurablePlugin.h"
#include "hdgnss/IPluginSettingsUi.h"
#include "src/core/AppSettings.h"
#include "src/core/PluginMetadata.h"
#include "src/transports/ITransport.h"

namespace hdgnss {

namespace {

QString normalizedTransportName(const QString &value) {
    return value.trimmed();
}

}  // namespace

TransportViewModel::TransportViewModel(QObject *parent)
    : QObject(parent)
    , m_builtinTransports({
          {QStringLiteral("UART"), QStringLiteral("UART"), m_serial.capabilities(), &m_serial},
          {QStringLiteral("TCP"), QStringLiteral("TCP"), m_tcp.capabilities(), &m_tcp},
          {QStringLiteral("UDP"), QStringLiteral("UDP"), m_udp.capabilities(), &m_udp},
      }) {
    hookTransport(&m_serial);
    hookTransport(&m_tcp);
    hookTransport(&m_udp);
    refreshSerialPorts();
    reloadTransportPlugins();
}

QStringList TransportViewModel::serialPorts() const {
    return m_serialPorts;
}

QString TransportViewModel::uartStatus() const {
    if (m_serial.isOpen()) {
        return m_serial.status();
    }
    if (m_serialPorts.isEmpty()) {
        return QStringLiteral("No serial ports detected");
    }
    return QStringLiteral("Detected %1 serial port%2")
        .arg(m_serialPorts.size())
        .arg(m_serialPorts.size() == 1 ? QString() : QStringLiteral("s"));
}

QString TransportViewModel::tcpStatus() const {
    return m_tcp.status();
}

QString TransportViewModel::udpStatus() const {
    return m_udp.status();
}

QString TransportViewModel::activeTransport() const {
    return m_activeTransport;
}

QString TransportViewModel::lockedTransport() const {
    return m_lockedTransport;
}

QVariantList TransportViewModel::availableTransports() const {
    QVariantList transports;
    transports.reserve(m_builtinTransports.size() + m_pluginTransports.size());
    for (const BuiltinTransportEntry &entry : m_builtinTransports) {
        transports.append(descriptorForBuiltin(entry));
    }
    for (const PluginTransportEntry &entry : m_pluginTransports) {
        transports.append(descriptorForPlugin(entry));
    }
    return transports;
}

QStringList TransportViewModel::transportPluginLoadErrors() const {
    return m_transportPluginLoadErrors;
}

QStringList TransportViewModel::transportPluginSearchPaths() const {
    return m_transportPluginSearchPaths;
}

void TransportViewModel::setActiveTransport(const QString &transportName) {
    const QString normalized = normalizedTransportName(transportName);
    if (normalized.isEmpty() || m_activeTransport == normalized) {
        return;
    }
    if (!m_lockedTransport.isEmpty()
        && m_lockedTransport.compare(normalized, Qt::CaseInsensitive) != 0) {
        return;
    }
    m_activeTransport = normalized;
    emit activeTransportChanged();
}

void TransportViewModel::setAppSettings(AppSettings *settings) {
    if (m_settings == settings) {
        return;
    }
    m_settings = settings;
    reloadTransportPlugins();
}

SerialTransport *TransportViewModel::serialTransport() {
    return &m_serial;
}

TcpClientTransport *TransportViewModel::tcpTransport() {
    return &m_tcp;
}

UdpServerTransport *TransportViewModel::udpTransport() {
    return &m_udp;
}

QList<ITransport *> TransportViewModel::allTransports() const {
    QList<ITransport *> transports;
    transports.reserve(m_builtinTransports.size() + m_pluginTransports.size());
    for (const BuiltinTransportEntry &entry : m_builtinTransports) {
        if (entry.transport) {
            transports.append(entry.transport);
        }
    }
    for (const PluginTransportEntry &entry : m_pluginTransports) {
        if (entry.transport) {
            transports.append(entry.transport);
        }
    }
    return transports;
}

QVariantMap TransportViewModel::transportDescriptor(const QString &transportName) const {
    if (const BuiltinTransportEntry *builtin = findBuiltinEntry(transportName)) {
        return descriptorForBuiltin(*builtin);
    }
    if (const PluginTransportEntry *plugin = findPluginEntry(transportName)) {
        return descriptorForPlugin(*plugin);
    }
    return {};
}

QString TransportViewModel::transportStatus(const QString &transportName) const {
    if (ITransport *transport = findTransport(transportName)) {
        return transport->status();
    }
    return {};
}

bool TransportViewModel::transportOpen(const QString &transportName) const {
    if (ITransport *transport = findTransport(transportName)) {
        return transport->isOpen();
    }
    return false;
}

QVariantMap TransportViewModel::transportStoredSettings(const QString &transportName) const {
    if (!m_settings) {
        return {};
    }
    QString settingsId = pluginSettingsId(transportName);
    if (const PluginTransportEntry *entry = findPluginEntry(transportName)) {
        if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(entry->pluginObject)) {
            const QString configurableId = configurable->settingsId().trimmed();
            if (!configurableId.isEmpty()) {
                settingsId = configurableId;
            }
        }
    }
    return m_settings->pluginPrivateSettings(settingsId);
}

void TransportViewModel::refreshTransportDescriptors() {
    emit availableTransportsChanged();
    emit statusesChanged();
}

void TransportViewModel::refreshSerialPorts() {
    QStringList ports;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (!info.portName().isEmpty() && !ports.contains(info.portName())) {
            ports.append(info.portName());
        }
    }
#ifdef Q_OS_MACOS
    const QStringList cuEntries = QDir(QStringLiteral("/dev")).entryList(
        {QStringLiteral("cu.*")},
        QDir::System | QDir::Files,
        QDir::Name | QDir::IgnoreCase);
    for (const QString &entry : cuEntries) {
        if (!ports.contains(entry)) {
            ports.append(entry);
        }
    }
#endif
    std::sort(ports.begin(), ports.end(), [](const QString &a, const QString &b) {
        return a.toLower() < b.toLower();
    });
    if (ports != m_serialPorts) {
        m_serialPorts = ports;
        emit serialPortsChanged();
    }
    emit statusesChanged();
}

void TransportViewModel::refreshUartPorts() {
    refreshSerialPorts();
}

void TransportViewModel::reloadTransportPlugins() {
    for (const PluginTransportEntry &entry : m_pluginTransports) {
        if (entry.transport) {
            entry.transport->close();
        }
    }

    m_pluginTransports.clear();
    m_transportPluginLoader = TransportPluginLoader{};
    m_transportPluginLoadErrors.clear();
    const QString userPluginDirectory = m_settings ? m_settings->pluginDirectory() : QString();
    m_transportPluginSearchPaths = TransportPluginLoader::defaultSearchPaths(QCoreApplication::applicationDirPath(),
                                                                             userPluginDirectory);

    if (!m_settings || m_settings->pluginsEnabled()) {
        m_transportPluginLoader.loadFromDirectories(m_transportPluginSearchPaths);
        m_transportPluginLoadErrors = m_transportPluginLoader.errors();
        const QList<ITransportPlugin *> plugins = m_transportPluginLoader.plugins();
        const QList<QObject *> pluginObjects = m_transportPluginLoader.pluginObjects();
        QSet<QString> loadedTransportIds;
        for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
            ITransportPlugin *plugin = plugins.at(index);
            QObject *pluginObject = pluginObjects.at(index);
            if (!plugin || !pluginObject) {
                continue;
            }

            const QString transportId = normalizedTransportName(plugin->transportId());
            if (transportId.isEmpty()) {
                m_transportPluginLoadErrors.append(QStringLiteral("%1: empty transportId()")
                                                       .arg(pluginObject->metaObject()->className()));
                continue;
            }
            if (loadedTransportIds.contains(transportId)) {
                continue;
            }

            QString settingsId = pluginSettingsId(plugin->transportId());
            QString displayName = plugin->transportDisplayName().trimmed();
            if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject)) {
                const QString configurableId = configurable->settingsId().trimmed();
                if (!configurableId.isEmpty()) {
                    settingsId = configurableId;
                }
                configurable->applySettings(m_settings ? m_settings->pluginPrivateSettings(settingsId) : QVariantMap{});
                if (displayName.isEmpty()) {
                    displayName = configurable->settingsDisplayName().trimmed();
                }
                if (m_settings && !m_settings->pluginEnabled(settingsId)) {
                    continue;
                }
            }

            ITransport *transport = plugin->transportInstance();
            if (!transport) {
                m_transportPluginLoadErrors.append(QStringLiteral("%1: transportInstance() returned null")
                                                       .arg(displayName.isEmpty() ? plugin->transportId() : displayName));
                continue;
            }

            PluginTransportEntry entry;
            entry.transportId = transportId;
            entry.displayName = displayName.isEmpty() ? entry.transportId : displayName;
            entry.hardwareInterfaces = plugin->supportedHardwareInterfaces();
            entry.plugin = plugin;
            entry.pluginObject = pluginObject;
            entry.transport = transport;
            m_pluginTransports.append(entry);
            loadedTransportIds.insert(entry.transportId);
            hookTransport(transport);
            emit transportRegistered(transport);
        }
    }

    setLockedTransport(currentOpenTransportName());
    emit availableTransportsChanged();
    emit statusesChanged();
}

QString TransportViewModel::probeTransport(const QString &transportName) {
    if (PluginTransportEntry *entry = findPluginEntry(transportName)) {
        if (!entry->plugin || !entry->plugin->supportsDiscovery()) {
            return {};
        }
        const QString summary = entry->plugin->probe();
        emit transportDescriptorChanged(entry->transportId);
        emit statusesChanged();
        return summary;
    }
    return {};
}

bool TransportViewModel::openTransport(const QString &transportName, const QVariantMap &settings) {
    const QString normalized = normalizedTransportName(transportName);
    ITransport *transport = findTransport(transportName);
    if (!transport) {
        return false;
    }

    const QString openName = currentOpenTransportName();
    if ((!m_lockedTransport.isEmpty()
         && m_lockedTransport.compare(normalized, Qt::CaseInsensitive) != 0)
        || (!openName.isEmpty()
            && openName.compare(normalized, Qt::CaseInsensitive) != 0)) {
        if (m_lockedTransport.isEmpty()) {
            setLockedTransport(openName);
        }
        return false;
    }

    setActiveTransport(normalized);
    const bool opened = transport->openWithSettings(settings);
    if (opened) {
        setLockedTransport(normalized);
    }
    return opened;
}

void TransportViewModel::closeTransport(const QString &transportName) {
    if (ITransport *transport = findTransport(transportName)) {
        transport->close();
    }
    if (m_lockedTransport.compare(normalizedTransportName(transportName), Qt::CaseInsensitive) == 0) {
        setLockedTransport(currentOpenTransportName());
    }
}

bool TransportViewModel::openUart(const QVariantMap &settings) {
    return openTransport(QStringLiteral("UART"), settings);
}

bool TransportViewModel::openUart(const QString &port, int baud, int dataBits, int stopBits, const QString &parity, const QString &flowControl) {
    return openUart({
        {QStringLiteral("port"), port},
        {QStringLiteral("baud"), baud},
        {QStringLiteral("dataBits"), dataBits},
        {QStringLiteral("stopBits"), stopBits},
        {QStringLiteral("parity"), parity},
        {QStringLiteral("flowControl"), flowControl}
    });
}

void TransportViewModel::closeUart() {
    closeTransport(QStringLiteral("UART"));
}

bool TransportViewModel::openTcp(const QVariantMap &settings) {
    return openTransport(QStringLiteral("TCP"), settings);
}

bool TransportViewModel::openTcp(const QString &host, int port) {
    return openTcp({
        {QStringLiteral("host"), host},
        {QStringLiteral("port"), port}
    });
}

void TransportViewModel::closeTcp() {
    closeTransport(QStringLiteral("TCP"));
}

bool TransportViewModel::openUdp(const QVariantMap &settings) {
    return openTransport(QStringLiteral("UDP"), settings);
}

bool TransportViewModel::openUdpServer(const QString &address, int port) {
    return openUdp({
        {QStringLiteral("address"), address},
        {QStringLiteral("port"), port}
    });
}

void TransportViewModel::closeUdp() {
    closeTransport(QStringLiteral("UDP"));
}

void TransportViewModel::closeUdpServer() {
    closeUdp();
}

bool TransportViewModel::send(const QByteArray &payload, const QString &target) {
    const QString resolved = (target == QStringLiteral("Active") || target == QStringLiteral("Default"))
        ? m_activeTransport
        : target;
    if (ITransport *transport = findTransport(resolved)) {
        return transport->sendData(payload);
    }
    return false;
}

QString TransportViewModel::pluginSettingsId(const QString &transportName) const {
    return QStringLiteral("transport.%1").arg(normalizedTransportName(transportName).toLower());
}

QVariantList TransportViewModel::configurableFields(QObject *pluginObject, const QString &settingsId) const {
    QVariantList fields;
    IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject);
    if (!configurable) {
        return fields;
    }

    const QVariantList fieldDefinitions = configurable->settingsFields();
    fields.reserve(fieldDefinitions.size());
    for (const QVariant &definitionValue : fieldDefinitions) {
        QVariantMap field = definitionValue.toMap();
        const QString key = field.value(QStringLiteral("key")).toString().trimmed();
        if (key.isEmpty()) {
            continue;
        }
        if (m_settings) {
            field.insert(QStringLiteral("value"),
                         m_settings->pluginSettingValue(settingsId,
                                                        key,
                                                        field.value(QStringLiteral("value"))));
        }
        fields.append(field);
    }
    return fields;
}

QVariantMap TransportViewModel::descriptorForBuiltin(const BuiltinTransportEntry &entry) const {
    QVariantMap descriptor{
        {QStringLiteral("transportName"), entry.transportId},
        {QStringLiteral("pluginId"), pluginSettingsId(entry.transportId)},
        {QStringLiteral("displayName"), entry.displayName},
        {QStringLiteral("category"), QStringLiteral("builtin")},
        {QStringLiteral("fields"), QVariantList{}},
        {QStringLiteral("fieldCount"), 0},
        {QStringLiteral("status"), entry.transport ? entry.transport->status() : QString()},
        {QStringLiteral("open"), entry.transport ? entry.transport->isOpen() : false},
        {QStringLiteral("capabilities"), entry.capabilities},
        {QStringLiteral("hardwareInterfaces"), QVariantList{}},
        {QStringLiteral("supportsDiscovery"), false},
        {QStringLiteral("enabled"), true},
    };
    descriptor.insert(QStringLiteral("version"), QString());
    return descriptor;
}

QVariantMap TransportViewModel::descriptorForPlugin(const PluginTransportEntry &entry) const {
    QString settingsId = pluginSettingsId(entry.transportId);
    QString displayName = entry.displayName;
    if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(entry.pluginObject)) {
        const QString configurableId = configurable->settingsId().trimmed();
        if (!configurableId.isEmpty()) {
            settingsId = configurableId;
        }
        const QString configurableName = configurable->settingsDisplayName().trimmed();
        if (!configurableName.isEmpty()) {
            displayName = configurableName;
        }
    }

    QVariantList hardwareInterfaces;
    for (const QString &value : entry.hardwareInterfaces) {
        hardwareInterfaces.append(value);
    }
    const QVariantList fields = configurableFields(entry.pluginObject, settingsId);
    QVariantMap customUi;
    if (IPluginSettingsUi *settingsUi = qobject_cast<IPluginSettingsUi *>(entry.pluginObject)) {
        customUi = settingsUi->settingsUi();
    }

    QVariantMap descriptor{
        {QStringLiteral("transportName"), entry.transportId},
        {QStringLiteral("pluginId"), settingsId},
        {QStringLiteral("displayName"), displayName.isEmpty() ? entry.transportId : displayName},
        {QStringLiteral("category"), QStringLiteral("plugin")},
        {QStringLiteral("fields"), fields},
        {QStringLiteral("fieldCount"), fields.size()},
        {QStringLiteral("customUi"), customUi},
        {QStringLiteral("status"), entry.transport ? entry.transport->status() : QString()},
        {QStringLiteral("open"), entry.transport ? entry.transport->isOpen() : false},
        {QStringLiteral("capabilities"), entry.transport ? entry.transport->capabilities() : QStringList{}},
        {QStringLiteral("hardwareInterfaces"), hardwareInterfaces},
        {QStringLiteral("supportsDiscovery"), entry.plugin ? entry.plugin->supportsDiscovery() : false},
        {QStringLiteral("enabled"), m_settings ? m_settings->pluginEnabled(settingsId) : true},
    };
    insertPluginVersion(descriptor, entry.pluginObject);
    return descriptor;
}

const TransportViewModel::BuiltinTransportEntry *TransportViewModel::findBuiltinEntry(const QString &transportName) const {
    const QString normalized = normalizedTransportName(transportName);
    for (const BuiltinTransportEntry &entry : m_builtinTransports) {
        if (entry.transportId.compare(normalized, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

TransportViewModel::BuiltinTransportEntry *TransportViewModel::findBuiltinEntry(const QString &transportName) {
    const QString normalized = normalizedTransportName(transportName);
    for (BuiltinTransportEntry &entry : m_builtinTransports) {
        if (entry.transportId.compare(normalized, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

const TransportViewModel::PluginTransportEntry *TransportViewModel::findPluginEntry(const QString &transportName) const {
    const QString normalized = normalizedTransportName(transportName);
    for (const PluginTransportEntry &entry : m_pluginTransports) {
        if (entry.transportId.compare(normalized, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

TransportViewModel::PluginTransportEntry *TransportViewModel::findPluginEntry(const QString &transportName) {
    const QString normalized = normalizedTransportName(transportName);
    for (PluginTransportEntry &entry : m_pluginTransports) {
        if (entry.transportId.compare(normalized, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

ITransport *TransportViewModel::findTransport(const QString &transportName) const {
    if (const BuiltinTransportEntry *builtin = findBuiltinEntry(transportName)) {
        return builtin->transport;
    }
    if (const PluginTransportEntry *plugin = findPluginEntry(transportName)) {
        return plugin->transport;
    }
    return nullptr;
}

void TransportViewModel::hookTransport(ITransport *transport) {
    if (!transport) {
        return;
    }
    connect(transport, &ITransport::statusChanged, this, &TransportViewModel::statusesChanged, Qt::UniqueConnection);
    connect(transport, &ITransport::openChanged, this, &TransportViewModel::handleTransportOpenChanged, Qt::UniqueConnection);
}

void TransportViewModel::handleTransportOpenChanged() {
    setLockedTransport(currentOpenTransportName());
    emit availableTransportsChanged();
    emit statusesChanged();
}

QString TransportViewModel::currentOpenTransportName() const {
    for (const BuiltinTransportEntry &entry : m_builtinTransports) {
        if (entry.transport && entry.transport->isOpen()) {
            return entry.transportId;
        }
    }
    for (const PluginTransportEntry &entry : m_pluginTransports) {
        if (entry.transport && entry.transport->isOpen()) {
            return entry.transportId;
        }
    }
    return {};
}

void TransportViewModel::setLockedTransport(const QString &transportName) {
    const QString normalized = normalizedTransportName(transportName);
    if (m_lockedTransport.compare(normalized, Qt::CaseInsensitive) == 0) {
        return;
    }
    m_lockedTransport = normalized;
    if (!m_lockedTransport.isEmpty()
        && m_activeTransport.compare(m_lockedTransport, Qt::CaseInsensitive) != 0) {
        m_activeTransport = m_lockedTransport;
        emit activeTransportChanged();
    }
    emit lockedTransportChanged();
}

}  // namespace hdgnss
