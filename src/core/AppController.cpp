#include "AppController.h"

#include <cmath>
#include <utility>
#include <QFile>
#include <QGuiApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimeZone>
#include <QUrl>

#include "hdgnss/IConfigurablePlugin.h"
#include "hdgnss/IAutomationPlugin.h"
#include "src/core/PluginMetadata.h"
#include "src/tec/TecMapOverlayModel.h"
#include "src/utils/ByteUtils.h"

namespace hdgnss {

namespace {

QVariantMap protocolPluginDescriptor(QObject *pluginObject, AppSettings *settings) {
    QVariantMap descriptor;
    if (!pluginObject) {
        return descriptor;
    }

    IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject);
    const QString pluginId = configurable
        ? configurable->settingsId().trimmed()
        : QStringLiteral("protocol.%1").arg(pluginObject->metaObject()->className()).toLower();
    const QString displayName = configurable
        ? configurable->settingsDisplayName().trimmed()
        : QString::fromLatin1(pluginObject->metaObject()->className());
    const QVariantList fieldDefinitions = configurable ? configurable->settingsFields() : QVariantList{};
    QVariantList fields;
    fields.reserve(fieldDefinitions.size());
    for (const QVariant &definitionValue : fieldDefinitions) {
        QVariantMap field = definitionValue.toMap();
        const QString key = field.value(QStringLiteral("key")).toString().trimmed();
        if (key.isEmpty()) {
            continue;
        }
        field.insert(QStringLiteral("value"), settings ? settings->pluginSettingValue(pluginId, key) : QVariant{});
        fields.append(field);
    }

    descriptor.insert(QStringLiteral("pluginId"), pluginId);
    descriptor.insert(QStringLiteral("category"), QStringLiteral("protocol"));
    descriptor.insert(QStringLiteral("displayName"), displayName.isEmpty() ? pluginId : displayName);
    insertPluginVersion(descriptor, pluginObject);
    descriptor.insert(QStringLiteral("enabled"), settings ? settings->pluginEnabled(pluginId) : true);
    descriptor.insert(QStringLiteral("fields"), fields);
    descriptor.insert(QStringLiteral("fieldCount"), fields.size());
    return descriptor;
}

QString automationPluginId(IAutomationPlugin *plugin, QObject *pluginObject) {
    const QString pluginId = plugin ? plugin->automationId().trimmed() : QString();
    if (!pluginId.isEmpty()) {
        return pluginId;
    }
    return pluginObject
        ? QStringLiteral("automation.%1").arg(pluginObject->metaObject()->className()).toLower()
        : QString();
}

QVariantMap automationPluginDescriptor(IAutomationPlugin *plugin,
                                       QObject *pluginObject,
                                       bool enabled) {
    QVariantMap descriptor;
    const QString pluginId = automationPluginId(plugin, pluginObject);
    if (pluginId.isEmpty()) {
        return descriptor;
    }

    QString displayName = plugin ? plugin->automationDisplayName().trimmed() : QString();
    if (displayName.isEmpty() && pluginObject) {
        displayName = QString::fromLatin1(pluginObject->metaObject()->className());
    }

    descriptor.insert(QStringLiteral("pluginId"), pluginId);
    descriptor.insert(QStringLiteral("category"), QStringLiteral("automation"));
    descriptor.insert(QStringLiteral("displayName"), displayName.isEmpty() ? pluginId : displayName);
    insertPluginVersion(descriptor, pluginObject);
    descriptor.insert(QStringLiteral("enabled"), enabled);
    descriptor.insert(QStringLiteral("fields"), QVariantList{});
    descriptor.insert(QStringLiteral("fieldCount"), 0);
    return descriptor;
}

bool hasHttpFileScheme(const QString &payload) {
    const QString trimmed = payload.trimmed();
    return trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        || trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

bool parseHttpFileUrl(const QString &payload, QUrl *urlOut) {
    const QString trimmed = payload.trimmed();
    const QUrl url(trimmed);
    if (!url.isValid() || url.host().isEmpty()) {
        return false;
    }

    if (urlOut) {
        *urlOut = url;
    }
    return true;
}

bool isLocationQualityMessage(const ProtocolMessage &message) {
    return message.fields.contains(QStringLiteral("quality"))
        && message.fields.contains(QStringLiteral("validFix"));
}

bool isNmeaTimeOnlyUtcMessage(const ProtocolMessage &message) {
    return message.protocol == QStringLiteral("NMEA")
        && (message.messageName == QStringLiteral("GGA")
            || message.messageName == QStringLiteral("GLL"));
}

QDateTime locationUtcTime(const ProtocolMessage &message, const QDateTime &currentUtcTime) {
    const QDateTime incomingUtcTime = message.fields.value(QStringLiteral("utcTime")).toDateTime();
    if (!incomingUtcTime.isValid()
        || !currentUtcTime.isValid()
        || !isNmeaTimeOnlyUtcMessage(message)) {
        return incomingUtcTime;
    }

    const QDate currentUtcDate = currentUtcTime.toTimeZone(QTimeZone::UTC).date();
    const QTime incomingUtcClock = incomingUtcTime.toTimeZone(QTimeZone::UTC).time();
    return QDateTime(currentUtcDate, incomingUtcClock, QTimeZone::UTC);
}

}  // namespace

AppController::AppController(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_tecMapOverlayModel(std::make_unique<TecMapOverlayModel>(settings, this)) {
    m_transportViewModel.setAppSettings(settings);
    m_fileSendTimer.setSingleShot(true);
    connect(&m_fileSendTimer, &QTimer::timeout, this, &AppController::sendNextFilePacket);

    m_uiRefreshTimer.setSingleShot(true);
    m_uiRefreshTimer.setInterval(50);
    connect(&m_uiRefreshTimer, &QTimer::timeout, this, &AppController::flushUiRefresh);

    if (m_settings) {
        m_rawRecorder.setRecordRawEnabled(m_settings->recordRawData());
        m_rawRecorder.setRecordDecodeEnabled(m_settings->recordDecodeLog());
        m_rawRecorder.setLogRootDirectory(m_settings->logDirectory());
        m_deviationMapModel.setFixedCenterEnabled(m_settings->useFixedDeviationCenter());
        m_deviationMapModel.setFixedCenter(m_settings->fixedDeviationLatitude(),
                                           m_settings->fixedDeviationLongitude());

        connect(m_settings, &AppSettings::recordRawDataChanged, this, [this]() {
            m_rawRecorder.setRecordRawEnabled(m_settings->recordRawData());
        });
        connect(m_settings, &AppSettings::recordDecodeLogChanged, this, [this]() {
            m_rawRecorder.setRecordDecodeEnabled(m_settings->recordDecodeLog());
        });
        connect(m_settings, &AppSettings::logDirectoryChanged, this, [this]() {
            m_rawRecorder.setLogRootDirectory(m_settings->logDirectory());
        });
        connect(m_settings, &AppSettings::pluginsEnabledChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &AppController::reloadProtocolPlugins, Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, &AppController::reloadAutomationPlugins, Qt::QueuedConnection);
            QMetaObject::invokeMethod(&m_transportViewModel, &TransportViewModel::reloadTransportPlugins, Qt::QueuedConnection);
        });
        connect(m_settings, &AppSettings::pluginDirectoryChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &AppController::reloadProtocolPlugins, Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, &AppController::reloadAutomationPlugins, Qt::QueuedConnection);
            QMetaObject::invokeMethod(&m_transportViewModel, &TransportViewModel::reloadTransportPlugins, Qt::QueuedConnection);
        });
        connect(m_settings, &AppSettings::pluginSettingsChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &AppController::reloadProtocolPlugins, Qt::QueuedConnection);
        });
        connect(m_settings, &AppSettings::pluginAvailabilityChanged, &m_transportViewModel, &TransportViewModel::reloadTransportPlugins);
        connect(m_settings, &AppSettings::pluginAvailabilityChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &AppController::reloadAutomationPlugins, Qt::QueuedConnection);
        });
        connect(m_settings, &AppSettings::useFixedDeviationCenterChanged, this, [this]() {
            m_deviationMapModel.setFixedCenterEnabled(m_settings->useFixedDeviationCenter());
        });
        connect(m_settings, &AppSettings::fixedDeviationLatitudeChanged, this, [this]() {
            m_deviationMapModel.setFixedCenter(m_settings->fixedDeviationLatitude(),
                                               m_settings->fixedDeviationLongitude());
        });
        connect(m_settings, &AppSettings::fixedDeviationLongitudeChanged, this, [this]() {
            m_deviationMapModel.setFixedCenter(m_settings->fixedDeviationLatitude(),
                                               m_settings->fixedDeviationLongitude());
        });
    }

    reloadProtocolPlugins();
    reloadAutomationPlugins();
    for (ITransport *transport : m_transportViewModel.allTransports()) {
        attachTransport(transport);
    }
    connect(&m_transportViewModel, &TransportViewModel::transportRegistered, this, &AppController::attachTransport);
}

AppController::~AppController() {
    m_shuttingDown = true;
    m_fileSendTimer.stop();
    for (ITransport *transport : m_transportViewModel.allTransports()) {
        disconnect(transport, nullptr, this, nullptr);
    }
}

AppSettings *AppController::appSettings() {
    return m_settings;
}

QObject *AppController::automationController() const {
    const QList<IAutomationPlugin *> plugins = m_activeAutomationPlugins;
    if (!plugins.isEmpty()) {
        return plugins.constFirst()->controllerObject();
    }
    return nullptr;
}

QVariantMap AppController::automationPanelUiConfig() const {
    const QList<IAutomationPlugin *> plugins = m_activeAutomationPlugins;
    if (!plugins.isEmpty()) {
        return plugins.constFirst()->panelUi();
    }
    return {};
}

TransportViewModel *AppController::transportViewModel() {
    return &m_transportViewModel;
}

RawLogModel *AppController::rawLogModel() {
    return &m_rawLogModel;
}

SatelliteModel *AppController::satelliteModel() {
    return &m_satelliteModel;
}

SignalModel *AppController::signalModel() {
    return &m_signalModel;
}

CommandButtonModel *AppController::commandButtonModel() {
    return &m_commandButtonModel;
}

DeviationMapModel *AppController::deviationMapModel() {
    return &m_deviationMapModel;
}

TecMapOverlayModel *AppController::tecMapOverlayModel() {
    return m_tecMapOverlayModel.get();
}

double AppController::latitude() const {
    return m_location.latitude;
}

double AppController::longitude() const {
    return m_location.longitude;
}

double AppController::altitude() const {
    return m_location.altitudeMeters;
}

double AppController::undulation() const {
    return m_location.undulationMeters;
}

double AppController::speed() const {
    return m_location.speedMps;
}

double AppController::track() const {
    return m_location.courseDegrees;
}

double AppController::magneticVariation() const {
    return m_location.magneticVariationDegrees;
}

QString AppController::fixType() const {
    return m_location.fixType;
}

int AppController::quality() const {
    return m_location.quality;
}

QString AppController::mode() const {
    return m_location.mode;
}

QString AppController::navigationStatus() const {
    return m_location.status;
}

QString AppController::utcTime() const {
    return m_location.utcTime.isValid()
        ? m_location.utcTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        : QStringLiteral("--:--:--");
}

QString AppController::utcDate() const {
    return m_location.utcTime.isValid()
        ? m_location.utcTime.toString(QStringLiteral("yyyy-MM-dd"))
        : QStringLiteral("---- -- --");
}

QString AppController::utcClock() const {
    return m_location.utcTime.isValid()
        ? m_location.utcTime.toString(QStringLiteral("HH:mm:ss"))
        : QStringLiteral("--:--:--");
}

int AppController::satelliteCount() const {
    return satellitesInView();
}

int AppController::satellitesUsed() const {
    return m_location.satellitesUsed;
}

double AppController::age() const {
    return m_location.differentialAgeSeconds;
}

double AppController::hdop() const {
    return m_location.hdop;
}

double AppController::vdop() const {
    return m_location.vdop;
}

double AppController::pdop() const {
    return m_location.pdop;
}

double AppController::gstRms() const {
    return m_location.gstRms;
}

double AppController::latitudeSigma() const {
    return m_location.latitudeSigma;
}

double AppController::longitudeSigma() const {
    return m_location.longitudeSigma;
}

double AppController::altitudeSigma() const {
    return m_location.altitudeSigma;
}

QString AppController::locationText() const {
    if (!m_location.validFix) {
        return QStringLiteral("No valid navigation fix");
    }
    return QStringLiteral("%1, %2 | Alt %3 m")
        .arg(m_location.latitude, 0, 'f', 6)
        .arg(m_location.longitude, 0, 'f', 6)
        .arg(m_location.altitudeMeters, 0, 'f', 1);
}

QString AppController::velocityText() const {
    const QString speed = std::isnan(m_location.speedMps)
        ? QStringLiteral("--")
        : QString::number(m_location.speedMps, 'f', 2);
    const QString course = std::isnan(m_location.courseDegrees)
        ? QStringLiteral("--")
        : QString::number(m_location.courseDegrees, 'f', 1);
    return QStringLiteral("%1 m/s | %2°").arg(speed, course);
}

QString AppController::fixText() const {
    const QString hdop = std::isnan(m_location.hdop)
        ? QStringLiteral("--")
        : QString::number(m_location.hdop, 'f', 1);
    const QString pdop = std::isnan(m_location.pdop)
        ? QStringLiteral("--")
        : QString::number(m_location.pdop, 'f', 1);
    return QStringLiteral("%1 | HDOP %2 | PDOP %3")
        .arg(m_location.fixType.isEmpty() ? QStringLiteral("No Fix") : m_location.fixType,
             hdop, pdop);
}

QString AppController::utcText() const {
    return m_location.utcTime.isValid()
        ? m_location.utcTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'"))
        : QStringLiteral("UTC unavailable");
}

int AppController::satellitesInView() const {
    int count = 0;
    for (const SatelliteInfo &sat : m_satellites) {
        if (satelliteHasVisibleSignal(sat)) {
            ++count;
        }
    }
    return count;
}

QString AppController::satelliteUsage() const {
    return signalUsageText(QStringLiteral("ALL"));
}

QString AppController::gpsSignals() const {
    return signalUsageText(QStringLiteral("GPS"));
}

QString AppController::glonassSignals() const {
    return signalUsageText(QStringLiteral("GLONASS"));
}

QString AppController::beidouSignals() const {
    return signalUsageText(QStringLiteral("BEIDOU"));
}

QString AppController::galileoSignals() const {
    return signalUsageText(QStringLiteral("GALILEO"));
}

QString AppController::otherSignals() const {
    return signalUsageText(QStringLiteral("OTHER"));
}

QString AppController::sessionDirectory() const {
    return m_rawRecorder.sessionDirectory();
}

qulonglong AppController::totalRecordedBytes() const {
    return static_cast<qulonglong>(m_rawRecorder.bytesRecorded());
}

QStringList AppController::fileSendDecoders() const {
    return m_fileSendDecoders;
}

QVariantList AppController::protocolBuildMessages() const {
    return m_protocolBuildMessages;
}

QVariantList AppController::protocolInfoTabs() const {
    QVariantList tabs;
    tabs.reserve(m_protocolInfoPanels.size());
    for (const ProtocolInfoPanelState &panel : m_protocolInfoPanels) {
        tabs.append(QVariantMap{
            {QStringLiteral("id"), panel.definition.id},
            {QStringLiteral("tabTitle"), panel.definition.tabTitle}
        });
    }
    return tabs;
}

QVariantList AppController::availableProtocolPlugins() const {
    return m_availableProtocolPlugins;
}

QStringList AppController::protocolPluginLoadErrors() const {
    return m_protocolPluginLoadErrors;
}

QStringList AppController::protocolPluginSearchPaths() const {
    return m_protocolPluginSearchPaths;
}

QVariantList AppController::availableAutomationPlugins() const {
    return m_availableAutomationPlugins;
}

QStringList AppController::automationPluginLoadErrors() const {
    return m_automationPluginLoadErrors;
}

QStringList AppController::automationPluginSearchPaths() const {
    return m_automationPluginSearchPaths;
}

QVariantList AppController::protocolBuildMessagesForProtocol(const QString &protocolName) const {
    QVariantList matches;
    const QString normalizedProtocol = protocolName.trimmed();
    for (const QVariant &value : m_protocolBuildMessages) {
        const QVariantMap message = value.toMap();
        if (message.value(QStringLiteral("protocol")).toString().compare(normalizedProtocol, Qt::CaseInsensitive) == 0) {
            matches.append(message);
        }
    }
    return matches;
}

QVariantMap AppController::protocolBuildMessageDefinition(const QString &protocolName, const QString &messageName) const {
    const QString key = QStringLiteral("%1|%2")
                            .arg(protocolName.trimmed().toUpper(),
                                 messageName.trimmed().toUpper());
    return m_protocolBuildMessageDefinitions.value(key);
}

QVariantMap AppController::protocolInfoPanel(const QString &panelId) const {
    for (const ProtocolInfoPanelState &panel : m_protocolInfoPanels) {
        if (panel.definition.id == panelId) {
            return protocolInfoPanelMap(panel);
        }
    }
    return {};
}

void AppController::sendBytes(const QByteArray &bytes, const QString &target) {
    sendRawCommand(bytes, target);
}

void AppController::onAutomationSendRequested(const QByteArray &bytes, const QString &target) {
    sendRawCommand(bytes, target);
}

void AppController::sendCommand(const QString &payload, const QString &contentType, const QString &target) {
    QByteArray bytes = contentType.compare(QStringLiteral("HEX"), Qt::CaseInsensitive) == 0
        ? ByteUtils::fromHexString(payload)
        : payload.toUtf8();
    if (contentType.compare(QStringLiteral("ASCII"), Qt::CaseInsensitive) == 0 && !bytes.endsWith("\r\n")) {
        bytes.append("\r\n");
    }
    sendRawCommand(bytes, target);
}

bool AppController::sendRawCommand(const QByteArray &bytes, const QString &target) {
    const QString resolvedTarget = (target == QStringLiteral("Active") || target == QStringLiteral("Default"))
        ? m_transportViewModel.activeTransport()
        : target;
    if (!m_transportViewModel.send(bytes, resolvedTarget)) {
        emit statusMessage(QStringLiteral("Failed to send command through %1").arg(resolvedTarget));
        return false;
    }
    handleIncomingBytes(resolvedTarget, bytes, DataDirection::Tx);
    return true;
}

IProtocolPlugin *AppController::findProtocolPlugin(const QString &protocolName) const {
    if (protocolName.compare(QStringLiteral("NMEA"), Qt::CaseInsensitive) == 0) {
        return const_cast<NmeaProtocolPlugin *>(&m_nmea);
    }

    const QList<IProtocolPlugin *> plugins = m_protocolPluginLoader.plugins();
    const QList<QObject *> pluginObjects = m_protocolPluginLoader.pluginObjects();
    for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
        IProtocolPlugin *plugin = plugins.at(index);
        QObject *pluginObject = pluginObjects.at(index);
        if (m_settings && !m_settings->pluginsEnabled()) {
            continue;
        }
        if (pluginObject) {
            if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject)) {
                const QString pluginId = configurable->settingsId().trimmed();
                if (m_settings && !m_settings->pluginEnabled(pluginId)) {
                    continue;
                }
            }
        }
        if (plugin && plugin->protocolName().compare(protocolName, Qt::CaseInsensitive) == 0) {
            return plugin;
        }
    }
    return nullptr;
}

void AppController::triggerButton(int row) {
    const QVariantMap button = m_commandButtonModel.get(row);
    if (button.isEmpty()) {
        return;
    }
    const QString contentType = button.value(QStringLiteral("contentType")).toString();
    if (contentType.compare(QStringLiteral("FILE"), Qt::CaseInsensitive) == 0) {
        sendFileCommand(button.value(QStringLiteral("payload")).toString(),
                        button.value(QStringLiteral("fileDecoder"), QStringLiteral("None")).toString(),
                        QStringLiteral("Default"),
                        button.value(QStringLiteral("filePacketIntervalMs"), 0).toInt());
        return;
    }
    if (contentType.compare(QStringLiteral("Plugin"), Qt::CaseInsensitive) == 0) {
        const QString buildProtocol = button.value(QStringLiteral("buildProtocol")).toString().trimmed();
        QString pluginMessage = button.value(QStringLiteral("pluginMessage")).toString().trimmed().toUpper();
        const QVariantList availableMessages = protocolBuildMessagesForProtocol(buildProtocol);
        if (pluginMessage.isEmpty()) {
            if (!availableMessages.isEmpty()) {
                pluginMessage = availableMessages.constFirst().toMap().value(QStringLiteral("name")).toString().trimmed().toUpper();
            }
        }
        if (pluginMessage.isEmpty()) {
            emit statusMessage(QStringLiteral("%1 plugin does not expose any build messages").arg(buildProtocol));
            return;
        }
        IProtocolPlugin *plugin = findProtocolPlugin(buildProtocol);
        if (!plugin) {
            emit statusMessage(QStringLiteral("%1 plugin is not loaded; cannot send %2").arg(buildProtocol, pluginMessage));
            return;
        }

        QVariantMap command;
        command.insert(QStringLiteral("messageType"), pluginMessage);
        command.insert(QStringLiteral("parameterText"), button.value(QStringLiteral("payload")).toString());
        const QByteArray bytes = plugin->encode(command);
        if (bytes.isEmpty()) {
            emit statusMessage(QStringLiteral("Failed to encode Bream plugin message %1").arg(pluginMessage));
            return;
        }
        sendRawCommand(bytes, QStringLiteral("Default"));
        return;
    }
    sendCommand(button.value(QStringLiteral("payload")).toString(),
                contentType,
                QStringLiteral("Default"));
}

void AppController::importPluginTemplates() {
    m_commandButtonModel.importTemplates(m_protocolDispatcher.commandTemplates());
}

void AppController::copyText(const QString &text) {
    if (text.isEmpty()) {
        return;
    }
    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
}

QVariantMap AppController::ioDiagnostics() const {
    QVariantMap transports;
    qulonglong totalRxBytes = 0;
    qulonglong totalTxBytes = 0;
    qulonglong totalRxChunks = 0;
    qulonglong totalTxChunks = 0;

    for (auto it = m_streamCounters.cbegin(); it != m_streamCounters.cend(); ++it) {
        transports.insert(it.key(), streamCountersMap(it.key(), it.value()));
        totalRxBytes += it.value().rxBytes;
        totalTxBytes += it.value().txBytes;
        totalRxChunks += it.value().rxChunks;
        totalTxChunks += it.value().txChunks;
    }

    return {
        {QStringLiteral("totalRxBytes"), totalRxBytes},
        {QStringLiteral("totalTxBytes"), totalTxBytes},
        {QStringLiteral("totalRxChunks"), totalRxChunks},
        {QStringLiteral("totalTxChunks"), totalTxChunks},
        {QStringLiteral("recordedBytes"), static_cast<qulonglong>(m_rawRecorder.bytesRecorded())},
        {QStringLiteral("recordedEntries"), static_cast<qulonglong>(m_rawRecorder.entriesRecorded())},
        {QStringLiteral("sessionDirectory"), m_rawRecorder.sessionDirectory()},
        {QStringLiteral("binaryFilePath"), m_rawRecorder.binaryFilePath()},
        {QStringLiteral("logFilePath"), m_rawRecorder.logFilePath()},
        {QStringLiteral("jsonlFilePath"), m_rawRecorder.jsonlFilePath()},
        {QStringLiteral("transports"), transports}
    };
}

QVariantMap AppController::transportDiagnostics(const QString &transportName) const {
    return streamCountersMap(transportName, m_streamCounters.value(transportName));
}

void AppController::attachTransport(ITransport *transport) {
    connect(transport, &ITransport::dataReceived, this, [this, transport](const QByteArray &bytes) {
        handleIncomingBytes(transport->name(), bytes, DataDirection::Rx);
    });
    connect(transport, &ITransport::errorOccurred, this, [this](const QString &message) {
        emit statusMessage(message);
    });
    connect(transport, &ITransport::openChanged, this, [this, transport]() {
        handleTransportOpenStateChanged(transport);
    });
}

void AppController::handleTransportOpenStateChanged(ITransport *transport) {
    if (!transport || m_shuttingDown) {
        return;
    }

    if (!transport->isOpen()) {
        resetTransportState(transport->name());
        return;
    }

    if (m_transportViewModel.activeTransport() == transport->name()) {
        startTransportSession(transport);
        return;
    }

    resetTransportState(transport->name());
}

void AppController::startTransportSession(ITransport *transport) {
    if (!transport) {
        return;
    }

    resetAllStreamState(!m_settings || m_settings->resetUiOnNewConnection());
    m_rawRecorder.startSession(transport->sessionBaseName(),
                               QDateTime::currentDateTimeUtc(),
                               transport->sessionQualifier());
    emit sessionDirectoryChanged();
    emit diagnosticsChanged();
    if (m_rawRecorder.sessionDirectory().isEmpty()) {
        emit statusMessage(QStringLiteral("%1 session started; logging disabled").arg(transport->name()));
    } else {
        emit statusMessage(QStringLiteral("Recording %1 session to %2")
                               .arg(transport->name(), m_rawRecorder.sessionDirectory()));
    }
}

void AppController::resetTransportState(const QString &transportName) {
    const QString rxKey = QStringLiteral("%1:RX").arg(transportName);
    const QString txKey = QStringLiteral("%1:TX").arg(transportName);
    m_streamBuffers.remove(rxKey);
    m_streamBuffers.remove(txKey);
    m_protocolDispatcher.resetStream(rxKey);
    m_protocolDispatcher.resetStream(txKey);
}

void AppController::resetAllStreamState(bool clearUi) {
    m_streamBuffers.clear();
    m_protocolDispatcher.resetAllStreams();
    m_nmea.resetState();

    if (!clearUi) {
        return;
    }

    m_streamCounters.clear();
    clearUiState();
    emit diagnosticsChanged();
}

void AppController::clearUiState() {
    m_location = GnssLocation{};
    m_satellites.clear();
    m_rawLogModel.clear();
    m_satelliteModel.setSatellites({});
    m_signalModel.setSatellites({});
    m_deviationMapModel.clear();
    m_lastDeviationSampleUtcTime = {};
    m_lastDeviationSamplePriority = -1;
    for (ProtocolInfoPanelState &panel : m_protocolInfoPanels) {
        panel.values.clear();
    }
    m_locationDirty = false;
    m_diagnosticsDirty = false;
    m_protocolInfoDirty = false;
    m_satellitesDirty = false;
    if (m_tecMapOverlayModel) {
        m_tecMapOverlayModel->setObservationTime({});
    }
    emit locationChanged();
    emit protocolInfoPanelsChanged();
}

void AppController::reloadProtocolPlugins() {
    m_nmea.resetState();
    m_protocolDispatcher = ProtocolDispatcher{};
    m_protocolPluginLoader = ProtocolPluginLoader{};
    m_availableProtocolPlugins.clear();
    m_protocolPluginLoadErrors.clear();
    m_protocolPluginSearchPaths.clear();
    m_protocolInfoPanels.clear();
    m_fileSendDecoders = {QStringLiteral("None")};
    m_protocolBuildMessages.clear();
    m_protocolBuildMessageDefinitions.clear();
    m_binaryFramers.clear();

    emit availableProtocolPluginsChanged();
    emit protocolInfoTabsChanged();
    emit protocolInfoPanelsChanged();
    emit fileSendDecodersChanged();
    emit protocolBuildMessagesChanged();

    registerProtocolPlugin(m_nmea);
    const QString userPluginDirectory = m_settings ? m_settings->pluginDirectory() : QString();
    const QStringList pluginSearchPaths = ProtocolPluginLoader::defaultSearchPaths(QCoreApplication::applicationDirPath(),
                                                                                   userPluginDirectory);
    m_protocolPluginSearchPaths = pluginSearchPaths;
    m_protocolPluginLoader.loadFromDirectories(pluginSearchPaths);
    rebuildAvailableProtocolPlugins();
    m_protocolPluginLoadErrors = m_protocolPluginLoader.errors();

    const QList<IProtocolPlugin *> plugins = m_protocolPluginLoader.plugins();
    const QList<QObject *> pluginObjects = m_protocolPluginLoader.pluginObjects();
    for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
        IProtocolPlugin *plugin = plugins.at(index);
        QObject *pluginObject = pluginObjects.at(index);
        if (!plugin || !pluginObject) {
            continue;
        }
        if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject)) {
            const QString pluginId = configurable->settingsId().trimmed();
            configurable->applySettings(m_settings ? m_settings->pluginPrivateSettings(pluginId) : QVariantMap{});
            if (m_settings && !m_settings->pluginEnabled(pluginId)) {
                continue;
            }
        }
        if (m_settings && !m_settings->pluginsEnabled()) {
            continue;
        }
        registerProtocolPlugin(*plugin);
    }
    for (const QString &error : m_protocolPluginLoadErrors) {
        qWarning().noquote() << "Protocol plugin load error:" << error;
    }
    if (!m_protocolPluginLoadErrors.isEmpty()) {
        emit statusMessage(QStringLiteral("Protocol plugin load failed: %1").arg(m_protocolPluginLoadErrors.first()));
    }
    if (m_settings && !m_settings->pluginsEnabled()) {
        emit statusMessage(QStringLiteral("Protocol plugins disabled in Settings"));
    }
}

void AppController::reloadAutomationPlugins() {
    for (IAutomationPlugin *plugin : std::as_const(m_activeAutomationPlugins)) {
        if (!plugin) {
            continue;
        }
        if (QObject *controller = plugin->controllerObject()) {
            QObject::disconnect(controller, nullptr, this, nullptr);
        }
    }
    m_activeAutomationPlugins.clear();
    m_automationPluginLoader = AutomationPluginLoader{};
    m_availableAutomationPlugins.clear();
    m_automationPluginLoadErrors.clear();
    m_automationPluginSearchPaths.clear();

    const QString userPluginDirectory = m_settings ? m_settings->pluginDirectory() : QString();
    const QStringList searchPaths = AutomationPluginLoader::defaultSearchPaths(
        QCoreApplication::applicationDirPath(), userPluginDirectory);
    m_automationPluginSearchPaths = searchPaths;
    m_automationPluginLoader.loadFromDirectories(searchPaths);
    m_automationPluginLoadErrors = m_automationPluginLoader.errors();

    const QList<IAutomationPlugin *> plugins = m_automationPluginLoader.plugins();
    const QList<QObject *> pluginObjects = m_automationPluginLoader.pluginObjects();
    QString selectedPluginId;
    QStringList pluginIds;
    pluginIds.reserve(plugins.size());
    for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
        const QString pluginId = automationPluginId(plugins.at(index), pluginObjects.at(index));
        if (pluginId.isEmpty()) {
            continue;
        }
        pluginIds.append(pluginId);
        if (selectedPluginId.isEmpty() && (!m_settings || m_settings->pluginEnabled(pluginId))) {
            selectedPluginId = pluginId;
        }
    }

    QVariantList descriptors;
    descriptors.reserve(pluginIds.size());
    for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
        IAutomationPlugin *plugin = plugins.at(index);
        QObject *pluginObject = pluginObjects.at(index);
        const QString pluginId = automationPluginId(plugin, pluginObject);
        if (pluginId.isEmpty()) {
            continue;
        }

        const bool enabled = pluginId == selectedPluginId;
        descriptors.append(automationPluginDescriptor(plugin, pluginObject, enabled));
        if (!enabled || (m_settings && !m_settings->pluginsEnabled())) {
            continue;
        }

        plugin->initialize();
        m_activeAutomationPlugins.append(plugin);

        QObject *controller = plugin->controllerObject();
        if (!controller) {
            continue;
        }
        // Route sendRequested(bytes, target) from plugin → onAutomationSendRequested.
        // QueuedConnection serialises plugin sends through the event loop so
        // they cannot interleave with an active file-send job (FIFO protection).
        QObject::connect(
            controller,
            SIGNAL(sendRequested(QByteArray,QString)),
            this,
            SLOT(onAutomationSendRequested(QByteArray,QString)),
            Qt::QueuedConnection);
    }

    m_availableAutomationPlugins = descriptors;
    emit availableAutomationPluginsChanged();
}

void AppController::rebuildAvailableProtocolPlugins() {
    QVariantList plugins;
    const QList<QObject *> pluginObjects = m_protocolPluginLoader.pluginObjects();
    plugins.reserve(pluginObjects.size());
    for (QObject *pluginObject : pluginObjects) {
        const QVariantMap descriptor = protocolPluginDescriptor(pluginObject, m_settings);
        if (!descriptor.isEmpty()) {
            plugins.append(descriptor);
        }
    }
    m_availableProtocolPlugins = plugins;
    emit availableProtocolPluginsChanged();
}

void AppController::registerProtocolPlugin(IProtocolPlugin &plugin) {
    bool buildMessagesUpdated = false;
    if (&plugin == &m_nmea) {
        BuiltinProtocolRegistry::registerProtocols(m_protocolDispatcher, m_nmea);
    } else {
        m_protocolDispatcher.registerPlugin(plugin);
    }
    if (plugin.pluginKinds().contains(ProtocolPluginKind::Binary)) {
        const QString name = plugin.protocolName().trimmed().toUpper();
        if (!name.isEmpty() && !m_fileSendDecoders.contains(name)) {
            m_fileSendDecoders.append(name);
            emit fileSendDecodersChanged();
        }
        m_binaryFramers.append([&plugin](const QByteArray &buf) {
            return plugin.parseBinaryFrame(buf);
        });
    }
    for (const ProtocolBuildMessage &message : plugin.supportedBuildMessages()) {
        const QString messageName = message.name.trimmed().toUpper();
        const QString protocolName = message.protocol.trimmed().isEmpty()
            ? plugin.protocolName().trimmed()
            : message.protocol.trimmed();
        if (messageName.isEmpty() || protocolName.isEmpty()) {
            continue;
        }
        const QString key = QStringLiteral("%1|%2").arg(protocolName.toUpper(), messageName);
        if (!m_protocolBuildMessageDefinitions.contains(key)) {
            m_protocolBuildMessages.append(QVariantMap{
                {QStringLiteral("protocol"), protocolName},
                {QStringLiteral("name"), messageName},
                {QStringLiteral("description"), message.description},
                {QStringLiteral("defaultPayload"), message.defaultPayload}
            });
            buildMessagesUpdated = true;
        }
        m_protocolBuildMessageDefinitions.insert(key, QVariantMap{
            {QStringLiteral("protocol"), protocolName},
            {QStringLiteral("name"), messageName},
            {QStringLiteral("description"), message.description},
            {QStringLiteral("defaultPayload"), message.defaultPayload}
        });
    }
    registerProtocolInfoPanels(plugin.infoPanels());
    if (buildMessagesUpdated) {
        emit protocolBuildMessagesChanged();
    }
}

bool AppController::sendFileCommand(const QString &filePath, const QString &decoderName, const QString &target, int packetIntervalMs) {
    const QString source = filePath.trimmed();
    QUrl fileUrl;
    if (hasHttpFileScheme(source)) {
        if (!parseHttpFileUrl(source, &fileUrl)) {
            emit statusMessage(QStringLiteral("Invalid file URL: %1").arg(source));
            return false;
        }
        downloadFileCommand(fileUrl, decoderName, target, packetIntervalMs);
        return true;
    }

    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        emit statusMessage(QStringLiteral("Failed to open file: %1").arg(source));
        return false;
    }

    const QByteArray fileBytes = file.readAll();
    return queueFileBytesForSend(fileBytes, source, decoderName, target, packetIntervalMs);
}

bool AppController::queueFileBytesForSend(const QByteArray &fileBytes,
                                          const QString &sourceLabel,
                                          const QString &decoderName,
                                          const QString &target,
                                          int packetIntervalMs) {
    if (fileBytes.isEmpty()) {
        emit statusMessage(QStringLiteral("File is empty: %1").arg(sourceLabel));
        return false;
    }

    QList<QByteArray> packets;
    const QString trimmedDecoder = decoderName.trimmed();
    if (!trimmedDecoder.isEmpty() && trimmedDecoder.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0) {
        IProtocolPlugin *decoderPlugin = findProtocolPlugin(trimmedDecoder);
        if (!decoderPlugin) {
            emit statusMessage(QStringLiteral("File decoder not available: %1").arg(trimmedDecoder));
            return false;
        }
        QString packetizeError;
        packets = decoderPlugin->packetizeFile(fileBytes, &packetizeError);
        if (packets.isEmpty()) {
            emit statusMessage(QStringLiteral("Failed to packetize file %1: %2")
                                   .arg(sourceLabel,
                                        packetizeError.isEmpty() ? QStringLiteral("No payload to send") : packetizeError));
            return false;
        }
    } else {
        QString packetizeError;
        packets = FilePacketizer::packetize(fileBytes, &packetizeError);
        if (packets.isEmpty()) {
            emit statusMessage(QStringLiteral("Failed to packetize file %1: %2")
                                   .arg(sourceLabel,
                                        packetizeError.isEmpty() ? QStringLiteral("No payload to send") : packetizeError));
            return false;
        }
    }

    const QString resolvedTarget = (target == QStringLiteral("Active") || target == QStringLiteral("Default"))
        ? m_transportViewModel.activeTransport()
        : target;

    FileSendJob job;
    job.filePath = sourceLabel;
    job.decoderName = decoderName;
    job.resolvedTarget = resolvedTarget;
    job.packets = packets;
    job.packetIntervalMs = qMax(0, packetIntervalMs);

    const bool alreadyBusy = m_activeFileSendJob.has_value() || !m_fileSendQueue.isEmpty() || m_fileSendTimer.isActive();
    m_fileSendQueue.append(job);

    if (alreadyBusy) {
        emit statusMessage(QStringLiteral("Queued file send: %1 (%2 packet(s), %3 ms interval)")
                               .arg(sourceLabel)
                               .arg(packets.size())
                               .arg(job.packetIntervalMs));
    } else {
        startNextFileSendJob();
    }
    return true;
}

void AppController::downloadFileCommand(const QUrl &url, const QString &decoderName, const QString &target, int packetIntervalMs) {
    const QString sourceLabel = url.toDisplayString();
    emit statusMessage(QStringLiteral("Downloading file: %1").arg(sourceLabel));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "GnssView");
    request.setTransferTimeout(30000);

    QNetworkReply *reply = m_fileDownloadManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, decoderName, target, packetIntervalMs]() {
        handleFileDownloadFinished(reply, url, decoderName, target, packetIntervalMs);
    });
}

void AppController::handleFileDownloadFinished(QNetworkReply *reply,
                                               const QUrl &sourceUrl,
                                               const QString &decoderName,
                                               const QString &target,
                                               int packetIntervalMs) {
    reply->deleteLater();

    const QString sourceLabel = sourceUrl.toDisplayString();
    if (reply->error() != QNetworkReply::NoError) {
        emit statusMessage(QStringLiteral("Failed to download file %1: %2")
                               .arg(sourceLabel, reply->errorString()));
        return;
    }

    const QByteArray fileBytes = reply->readAll();
    emit statusMessage(QStringLiteral("Downloaded file: %1 (%2 bytes)")
                           .arg(sourceLabel)
                           .arg(fileBytes.size()));
    queueFileBytesForSend(fileBytes, sourceLabel, decoderName, target, packetIntervalMs);
}

void AppController::startNextFileSendJob() {
    if (m_activeFileSendJob.has_value() || m_fileSendQueue.isEmpty()) {
        return;
    }

    m_activeFileSendJob = m_fileSendQueue.takeFirst();
    const FileSendJob &job = *m_activeFileSendJob;
    emit statusMessage(QStringLiteral("Sending %1 packet(s) from %2 via %3 with %4 ms interval")
                           .arg(job.packets.size())
                           .arg(job.filePath)
                           .arg(job.resolvedTarget)
                           .arg(job.packetIntervalMs));
    sendNextFilePacket();
}

void AppController::sendNextFilePacket() {
    if (!m_activeFileSendJob.has_value()) {
        startNextFileSendJob();
        return;
    }

    FileSendJob &job = *m_activeFileSendJob;
    if (job.nextPacketIndex >= job.packets.size()) {
        emit statusMessage(QStringLiteral("Sent %1 packet(s) from %2 via %3")
                               .arg(job.packets.size())
                               .arg(job.filePath)
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    const QByteArray &packet = job.packets.at(job.nextPacketIndex);
    if (!m_transportViewModel.send(packet, job.resolvedTarget)) {
        emit statusMessage(QStringLiteral("Failed to send file packet %1/%2 through %3")
                               .arg(job.nextPacketIndex + 1)
                               .arg(job.packets.size())
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    handleIncomingBytes(job.resolvedTarget, packet, DataDirection::Tx);
    ++job.nextPacketIndex;

    if (job.nextPacketIndex >= job.packets.size()) {
        emit statusMessage(QStringLiteral("Sent %1 packet(s) from %2 via %3")
                               .arg(job.packets.size())
                               .arg(job.filePath)
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    m_fileSendTimer.start(job.packetIntervalMs);
}

void AppController::registerProtocolInfoPanels(const QList<ProtocolInfoPanel> &panels) {
    bool changed = false;
    for (const ProtocolInfoPanel &panel : panels) {
        if (panel.id.isEmpty() || panel.fields.isEmpty()) {
            continue;
        }

        bool exists = false;
        for (const ProtocolInfoPanelState &state : m_protocolInfoPanels) {
            if (state.definition.id == panel.id) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }

        ProtocolInfoPanelState state;
        state.definition = panel;
        m_protocolInfoPanels.append(state);
        changed = true;
    }

    if (changed) {
        emit protocolInfoTabsChanged();
        emit protocolInfoPanelsChanged();
    }
}

void AppController::handleIncomingBytes(const QString &transportName, const QByteArray &bytes, DataDirection direction) {
    const bool isRx = (direction == DataDirection::Rx);
    emit rawDataReceived(transportName, bytes, isRx);

    // Forward to each loaded automation plugin.
    for (IAutomationPlugin *plugin : std::as_const(m_activeAutomationPlugins)) {
        plugin->onRawBytesReceived(transportName, bytes, isRx);
    }

    const QString streamKey = QStringLiteral("%1:%2")
                                  .arg(transportName)
                                  .arg(isRx ? QStringLiteral("RX") : QStringLiteral("TX"));
    StreamCounters &counters = m_streamCounters[transportName];
    const int chunkSize = bytes.size();
    if (direction == DataDirection::Rx) {
        counters.rxBytes += static_cast<qulonglong>(chunkSize);
        counters.rxChunks += 1;
        counters.lastRxChunkSize = chunkSize;
    } else {
        counters.txBytes += static_cast<qulonglong>(chunkSize);
        counters.txChunks += 1;
        counters.lastTxChunkSize = chunkSize;
    }

    RawLogEntry entry;
    entry.timestampUtc = QDateTime::currentDateTimeUtc();
    entry.direction = direction;
    entry.transportName = transportName;
    entry.payload = bytes;
    m_rawRecorder.recordRaw(entry);
    m_diagnosticsDirty = true;
    scheduleUiRefresh();

    StreamChunker &streamBuffer = m_streamBuffers[streamKey];
    streamBuffer.append(bytes);
    const QList<StreamChunk> chunks = streamBuffer.takeAvailableChunks(m_binaryFramers);
    for (const StreamChunk &chunk : chunks) {
        const QList<ProtocolMessage> messages = m_protocolDispatcher.routeChunk(streamKey, chunk);
        if (chunk.kind == StreamChunkKind::Binary && !messages.isEmpty()) {
            for (const ProtocolMessage &message : messages) {
                StreamChunk messageChunk{StreamChunkKind::Binary, message.rawFrame};
                const QString decodedLine = message.logDecodeText.trimmed();
                m_rawRecorder.recordChunk(entry.timestampUtc,
                                          direction,
                                          messageChunk,
                                          decodedLine.isEmpty() ? QStringList{} : QStringList{decodedLine});
            }
        } else {
            QStringList decodedLines;
            decodedLines.reserve(messages.size());
            for (const ProtocolMessage &message : messages) {
                if (!message.logDecodeText.trimmed().isEmpty()) {
                    decodedLines.append(message.logDecodeText.trimmed());
                }
            }
            m_rawRecorder.recordChunk(entry.timestampUtc, direction, chunk, decodedLines);
        }
        if (messages.isEmpty()) {
            m_rawLogModel.appendChunk(entry.timestampUtc, direction, transportName, chunk.kindName(), chunk.payload);
            continue;
        }
        for (const ProtocolMessage &message : messages) {
            m_rawLogModel.appendProtocolMessage(entry.timestampUtc, direction, transportName, chunk.kindName(), message);
            applyProtocolMessage(message);
        }
    }
}

void AppController::applyProtocolMessage(const ProtocolMessage &message) {
    const QVariantMap fields = message.fields;
    auto assignDouble = [&fields](const QString &key, double &target) {
        if (fields.contains(key)) {
            target = fields.value(key).toDouble();
        }
    };
    bool protocolInfoChanged = false;

    if (fields.contains(QStringLiteral("validFix"))) {
        m_location.validFix = fields.value(QStringLiteral("validFix")).toBool();
    }
    if (fields.contains(QStringLiteral("fixType"))) {
        const QVariant value = fields.value(QStringLiteral("fixType"));
        m_location.fixType = value.typeId() == QMetaType::Int
            ? QStringLiteral("Fix %1").arg(value.toInt())
            : value.toString();
    }
    if (isLocationQualityMessage(message)) {
        m_location.quality = fields.value(QStringLiteral("quality")).toInt();
    }
    if (fields.contains(QStringLiteral("utcTime"))) {
        m_location.utcTime = locationUtcTime(message, m_location.utcTime);
    }
    assignDouble(QStringLiteral("latitude"), m_location.latitude);
    assignDouble(QStringLiteral("longitude"), m_location.longitude);
    assignDouble(QStringLiteral("altitudeMeters"), m_location.altitudeMeters);
    assignDouble(QStringLiteral("undulationMeters"), m_location.undulationMeters);
    assignDouble(QStringLiteral("speedMps"), m_location.speedMps);
    assignDouble(QStringLiteral("courseDegrees"), m_location.courseDegrees);
    assignDouble(QStringLiteral("magneticVariationDegrees"), m_location.magneticVariationDegrees);
    assignDouble(QStringLiteral("differentialAgeSeconds"), m_location.differentialAgeSeconds);
    assignDouble(QStringLiteral("hdop"), m_location.hdop);
    assignDouble(QStringLiteral("vdop"), m_location.vdop);
    assignDouble(QStringLiteral("pdop"), m_location.pdop);
    assignDouble(QStringLiteral("gstRms"), m_location.gstRms);
    assignDouble(QStringLiteral("latitudeSigma"), m_location.latitudeSigma);
    assignDouble(QStringLiteral("longitudeSigma"), m_location.longitudeSigma);
    assignDouble(QStringLiteral("altitudeSigma"), m_location.altitudeSigma);

    if (fields.contains(QStringLiteral("satellitesUsed"))) {
        m_location.satellitesUsed = fields.value(QStringLiteral("satellitesUsed")).toInt();
    }
    if (fields.contains(QStringLiteral("satellitesInView"))) {
        m_location.satellitesInView = fields.value(QStringLiteral("satellitesInView")).toInt();
    }
    if (fields.contains(QStringLiteral("mode"))) {
        m_location.mode = fields.value(QStringLiteral("mode")).toString();
    }
    if (fields.contains(QStringLiteral("status"))) {
        m_location.status = fields.value(QStringLiteral("status")).toString();
    }
    if (fields.contains(QStringLiteral("satellites"))) {
        const QVariantList sats = fields.value(QStringLiteral("satellites")).toList();
        for (const QVariant &value : sats) {
            const QVariantMap satMap = value.toMap();
            SatelliteInfo sat;
            sat.key = satMap.value(QStringLiteral("key"),
                                   QStringLiteral("%1-%2-%3")
                                       .arg(satMap.value(QStringLiteral("constellation")).toString())
                                       .arg(satMap.value(QStringLiteral("signalId")).toInt())
                                       .arg(satMap.value(QStringLiteral("svid")).toInt()))
                          .toString();
            sat.constellation = satMap.value(QStringLiteral("constellation")).toString();
            sat.band = satMap.value(QStringLiteral("band"), QStringLiteral("L1")).toString();
            sat.signalId = satMap.value(QStringLiteral("signalId")).toInt();
            sat.svid = satMap.value(QStringLiteral("svid")).toInt();
            sat.azimuth = satMap.value(QStringLiteral("azimuth")).toInt();
            sat.elevation = satMap.value(QStringLiteral("elevation")).toInt();
            sat.cn0 = satMap.value(QStringLiteral("cn0")).toInt();
            sat.usedInFix = satMap.value(QStringLiteral("usedInFix")).toBool();
            m_satellites.insert(sat.key, sat);
        }
        m_satellitesDirty = true;
    }

    for (ProtocolInfoPanelState &panel : m_protocolInfoPanels) {
        const QString panelTargetId = fields.value(QStringLiteral("infoPanelId")).toString();
        if (!panelTargetId.isEmpty() && panel.definition.id != panelTargetId) {
            continue;
        }
        if (panelTargetId.isEmpty()
            && !panel.definition.protocol.isEmpty()
            && panel.definition.protocol != message.protocol) {
            continue;
        }
        int matchedFieldCount = 0;
        for (const ProtocolInfoField &field : panel.definition.fields) {
            if (fields.contains(field.valueKey)) {
                ++matchedFieldCount;
            }
        }
        if (matchedFieldCount == 0) {
            continue;
        }

        bool panelChanged = false;
        if (panel.definition.replaceOnMessage && !panel.values.isEmpty()) {
            panel.values.clear();
            panelChanged = true;
        }
        for (const ProtocolInfoField &field : panel.definition.fields) {
            if (!fields.contains(field.valueKey)) {
                continue;
            }
            const QVariant newValue = fields.value(field.valueKey);
            if (panel.values.value(field.id) != newValue) {
                panel.values.insert(field.id, newValue);
                panelChanged = true;
            }
        }
        protocolInfoChanged = protocolInfoChanged || panelChanged;
    }

    m_locationDirty = true;
    if (m_tecMapOverlayModel) {
        m_tecMapOverlayModel->setObservationTime(m_location.utcTime);
    }
    recordDeviationSample(message);
    m_protocolInfoDirty = m_protocolInfoDirty || protocolInfoChanged;
    scheduleUiRefresh();
}

void AppController::recordDeviationSample(const ProtocolMessage &message) {
    const QVariantMap &fields = message.fields;
    if (!fields.contains(QStringLiteral("latitude"))
        || !fields.contains(QStringLiteral("longitude"))
        || !m_location.validFix
        || !std::isfinite(m_location.latitude)
        || !std::isfinite(m_location.longitude)) {
        return;
    }

    const QDateTime sampleUtcTime = fields.value(QStringLiteral("utcTime")).toDateTime().isValid()
        ? fields.value(QStringLiteral("utcTime")).toDateTime()
        : m_location.utcTime;
    const int priority = deviationSamplePriority(message.messageName);

    if (sampleUtcTime.isValid() && m_lastDeviationSampleUtcTime.isValid() && sampleUtcTime == m_lastDeviationSampleUtcTime) {
        if (priority > m_lastDeviationSamplePriority
            && m_deviationMapModel.updateLastSample(m_location.latitude, m_location.longitude)) {
            m_lastDeviationSamplePriority = priority;
        }
        return;
    }

    m_deviationMapModel.addSample(m_location.latitude, m_location.longitude);
    m_lastDeviationSampleUtcTime = sampleUtcTime;
    m_lastDeviationSamplePriority = priority;
}

void AppController::scheduleUiRefresh() {
    if (m_uiRefreshTimer.isActive()) {
        return;
    }
    m_uiRefreshTimer.start();
}

void AppController::flushUiRefresh() {
    if (m_satellitesDirty) {
        refreshSatellites();
        m_satellitesDirty = false;
    }
    if (m_locationDirty) {
        emit locationChanged();
        m_locationDirty = false;
    }
    if (m_protocolInfoDirty) {
        emit protocolInfoPanelsChanged();
        m_protocolInfoDirty = false;
    }
    if (m_diagnosticsDirty) {
        emit diagnosticsChanged();
        m_diagnosticsDirty = false;
    }
}

void AppController::refreshSatellites() {
    const QList<SatelliteInfo> satellites = m_satellites.values();
    m_satelliteModel.setSatellites(satellites);
    m_signalModel.setSatellites(satellites);
}

QVariantMap AppController::streamCountersMap(const QString &transportName, const StreamCounters &counters) const {
    return {
        {QStringLiteral("transport"), transportName},
        {QStringLiteral("rxBytes"), counters.rxBytes},
        {QStringLiteral("txBytes"), counters.txBytes},
        {QStringLiteral("rxChunks"), counters.rxChunks},
        {QStringLiteral("txChunks"), counters.txChunks},
        {QStringLiteral("lastRxChunkSize"), counters.lastRxChunkSize},
        {QStringLiteral("lastTxChunkSize"), counters.lastTxChunkSize}
    };
}

int AppController::deviationSamplePriority(const QString &messageName) {
    if (messageName == QStringLiteral("RMC")) {
        return 3;
    }
    if (messageName == QStringLiteral("GLL")) {
        return 2;
    }
    if (messageName == QStringLiteral("GGA")) {
        return 1;
    }
    return 0;
}

QVariantMap AppController::protocolInfoPanelMap(const ProtocolInfoPanelState &panel) const {
    return {
        {QStringLiteral("id"), panel.definition.id},
        {QStringLiteral("protocol"), panel.definition.protocol},
        {QStringLiteral("tabTitle"), panel.definition.tabTitle},
        {QStringLiteral("title"), panel.definition.title},
        {QStringLiteral("items"), protocolInfoPanelItems(panel)}
    };
}

QVariantList AppController::protocolInfoPanelItems(const ProtocolInfoPanelState &panel) const {
    QVariantList items;
    items.reserve(panel.definition.fields.size());
    for (const ProtocolInfoField &field : panel.definition.fields) {
        items.append(QVariantMap{
            {QStringLiteral("id"), field.id},
            {QStringLiteral("label"), field.label},
            {QStringLiteral("group"), field.group},
            {QStringLiteral("valueText"), formatProtocolInfoValue(field, panel.values.value(field.id))},
            {QStringLiteral("emphasize"), field.emphasize}
        });
    }
    return items;
}

QString AppController::formatProtocolInfoValue(const ProtocolInfoField &field, const QVariant &value) const {
    if (!value.isValid() || value.isNull()) {
        return field.fallbackText;
    }

    QString rendered;
    if (value.metaType().id() == QMetaType::Bool) {
        rendered = value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    } else if (field.precision >= 0 && value.canConvert<double>()) {
        rendered = QString::number(value.toDouble(), 'f', field.precision);
    } else {
        rendered = value.toString();
    }

    if (rendered.isEmpty()) {
        return field.fallbackText;
    }
    if (!field.unit.isEmpty()) {
        rendered += QStringLiteral(" ") + field.unit;
    }
    return rendered;
}

QString AppController::signalUsageText(const QString &group) const {
    auto belongsToGroup = [&group](const QString &constellation) {
        if (group.isEmpty() || group == QStringLiteral("ALL")) {
            return true;
        }
        if (group == QStringLiteral("OTHER")) {
            return constellation != QStringLiteral("GPS")
                && constellation != QStringLiteral("GLONASS")
                && constellation != QStringLiteral("BEIDOU")
                && constellation != QStringLiteral("GALILEO");
        }
        return constellation == group;
    };

    int used = 0;
    QSet<QString> visibleSatellites;
    QSet<QString> usedSatellites;
    for (auto it = m_satellites.cbegin(); it != m_satellites.cend(); ++it) {
        if (!satelliteHasVisibleSignal(*it) || !belongsToGroup(it->constellation)) {
            continue;
        }
        const QString satelliteKey = QStringLiteral("%1|%2").arg(it->constellation).arg(it->svid);
        visibleSatellites.insert(satelliteKey);
        if (it->usedInFix) {
            usedSatellites.insert(satelliteKey);
        }
    }
    used = usedSatellites.size();
    return QStringLiteral("%1 / %2").arg(used).arg(visibleSatellites.size());
}

}  // namespace hdgnss
