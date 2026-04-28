#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QSet>
#include <QTimer>
#include <memory>
#include <optional>

#include "src/core/AutomationPluginLoader.h"
#include "src/core/BuiltinProtocolRegistry.h"
#include "src/core/AppSettings.h"
#include "src/core/FilePacketizer.h"
#include "src/core/ProtocolDispatcher.h"
#include "src/core/ProtocolPluginLoader.h"
#include "src/core/StreamChunker.h"
#include "src/core/TransportViewModel.h"
#include "src/models/CommandButtonModel.h"
#include "src/models/DeviationMapModel.h"
#include "src/models/RawLogModel.h"
#include "src/models/SatelliteModel.h"
#include "src/models/SignalModel.h"
#include "src/protocols/NmeaProtocolPlugin.h"
#include "src/storage/RawRecorder.h"

class QNetworkReply;
class QUrl;

namespace hdgnss {

class TecMapOverlayModel;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double latitude READ latitude NOTIFY locationChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY locationChanged)
    Q_PROPERTY(double altitude READ altitude NOTIFY locationChanged)
    Q_PROPERTY(double undulation READ undulation NOTIFY locationChanged)
    Q_PROPERTY(double speed READ speed NOTIFY locationChanged)
    Q_PROPERTY(double track READ track NOTIFY locationChanged)
    Q_PROPERTY(double magneticVariation READ magneticVariation NOTIFY locationChanged)
    Q_PROPERTY(QString fixType READ fixType NOTIFY locationChanged)
    Q_PROPERTY(int quality READ quality NOTIFY locationChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY locationChanged)
    Q_PROPERTY(QString navigationStatus READ navigationStatus NOTIFY locationChanged)
    Q_PROPERTY(QString utcTime READ utcTime NOTIFY locationChanged)
    Q_PROPERTY(QString utcDate READ utcDate NOTIFY locationChanged)
    Q_PROPERTY(QString utcClock READ utcClock NOTIFY locationChanged)
    Q_PROPERTY(int satelliteCount READ satelliteCount NOTIFY locationChanged)
    Q_PROPERTY(int satellitesUsed READ satellitesUsed NOTIFY locationChanged)
    Q_PROPERTY(double age READ age NOTIFY locationChanged)
    Q_PROPERTY(double hdop READ hdop NOTIFY locationChanged)
    Q_PROPERTY(double vdop READ vdop NOTIFY locationChanged)
    Q_PROPERTY(double pdop READ pdop NOTIFY locationChanged)
    Q_PROPERTY(double gstRms READ gstRms NOTIFY locationChanged)
    Q_PROPERTY(double latitudeSigma READ latitudeSigma NOTIFY locationChanged)
    Q_PROPERTY(double longitudeSigma READ longitudeSigma NOTIFY locationChanged)
    Q_PROPERTY(double altitudeSigma READ altitudeSigma NOTIFY locationChanged)
    Q_PROPERTY(QString locationText READ locationText NOTIFY locationChanged)
    Q_PROPERTY(QString velocityText READ velocityText NOTIFY locationChanged)
    Q_PROPERTY(QString fixText READ fixText NOTIFY locationChanged)
    Q_PROPERTY(QString utcText READ utcText NOTIFY locationChanged)
    Q_PROPERTY(int satellitesInView READ satellitesInView NOTIFY locationChanged)
    Q_PROPERTY(QString satelliteUsage READ satelliteUsage NOTIFY locationChanged)
    Q_PROPERTY(QString gpsSignals READ gpsSignals NOTIFY locationChanged)
    Q_PROPERTY(QString glonassSignals READ glonassSignals NOTIFY locationChanged)
    Q_PROPERTY(QString beidouSignals READ beidouSignals NOTIFY locationChanged)
    Q_PROPERTY(QString galileoSignals READ galileoSignals NOTIFY locationChanged)
    Q_PROPERTY(QString otherSignals READ otherSignals NOTIFY locationChanged)
    Q_PROPERTY(QString sessionDirectory READ sessionDirectory NOTIFY sessionDirectoryChanged)
    Q_PROPERTY(qulonglong totalRecordedBytes READ totalRecordedBytes NOTIFY diagnosticsChanged)
    Q_PROPERTY(QStringList fileSendDecoders READ fileSendDecoders NOTIFY fileSendDecodersChanged)
    Q_PROPERTY(QVariantList protocolBuildMessages READ protocolBuildMessages NOTIFY protocolBuildMessagesChanged)
    Q_PROPERTY(QVariantList protocolInfoTabs READ protocolInfoTabs NOTIFY protocolInfoTabsChanged)
    Q_PROPERTY(QVariantList availableProtocolPlugins READ availableProtocolPlugins NOTIFY availableProtocolPluginsChanged)
    Q_PROPERTY(QStringList protocolPluginLoadErrors READ protocolPluginLoadErrors NOTIFY availableProtocolPluginsChanged)
    Q_PROPERTY(QStringList protocolPluginSearchPaths READ protocolPluginSearchPaths NOTIFY availableProtocolPluginsChanged)
    Q_PROPERTY(QVariantList availableAutomationPlugins READ availableAutomationPlugins NOTIFY availableAutomationPluginsChanged)
    Q_PROPERTY(QStringList automationPluginLoadErrors READ automationPluginLoadErrors NOTIFY availableAutomationPluginsChanged)
    Q_PROPERTY(QStringList automationPluginSearchPaths READ automationPluginSearchPaths NOTIFY availableAutomationPluginsChanged)

public:
    explicit AppController(AppSettings *settings, QObject *parent = nullptr);
    ~AppController() override;

    AppSettings *appSettings();
    TransportViewModel *transportViewModel();
    // Returns the first loaded automation plugin's controller object (nullptr if none loaded).
    Q_INVOKABLE QObject *automationController() const;
    // Returns the UI configuration for the first loaded automation plugin's panel.
    // Contains "type" (qmlSource|qmlFile) and "qmlSource" or "qmlFile" keys.
    Q_INVOKABLE QVariantMap automationPanelUiConfig() const;
    RawLogModel *rawLogModel();
    SatelliteModel *satelliteModel();
    SignalModel *signalModel();
    CommandButtonModel *commandButtonModel();
    DeviationMapModel *deviationMapModel();
    TecMapOverlayModel *tecMapOverlayModel();

    double latitude() const;
    double longitude() const;
    double altitude() const;
    double undulation() const;
    double speed() const;
    double track() const;
    double magneticVariation() const;
    QString fixType() const;
    int quality() const;
    QString mode() const;
    QString navigationStatus() const;
    QString utcTime() const;
    QString utcDate() const;
    QString utcClock() const;
    int satelliteCount() const;
    int satellitesUsed() const;
    double age() const;
    double hdop() const;
    double vdop() const;
    double pdop() const;
    double gstRms() const;
    double latitudeSigma() const;
    double longitudeSigma() const;
    double altitudeSigma() const;
    QString locationText() const;
    QString velocityText() const;
    QString fixText() const;
    QString utcText() const;
    int satellitesInView() const;
    QString satelliteUsage() const;
    QString gpsSignals() const;
    QString glonassSignals() const;
    QString beidouSignals() const;
    QString galileoSignals() const;
    QString otherSignals() const;
    QString sessionDirectory() const;
    qulonglong totalRecordedBytes() const;
    QStringList fileSendDecoders() const;
    QVariantList protocolBuildMessages() const;
    QVariantList protocolInfoTabs() const;
    QVariantList availableProtocolPlugins() const;
    QStringList protocolPluginLoadErrors() const;
    QStringList protocolPluginSearchPaths() const;
    QVariantList availableAutomationPlugins() const;
    QStringList automationPluginLoadErrors() const;
    QStringList automationPluginSearchPaths() const;

    Q_INVOKABLE void sendCommand(const QString &payload, const QString &contentType, const QString &target = QStringLiteral("Active"));
    // Sends raw binary bytes; callable from QML and from the automation plugin slot below.
    Q_INVOKABLE void sendBytes(const QByteArray &bytes, const QString &target = QStringLiteral("Active"));
    Q_INVOKABLE void triggerButton(int row);
    Q_INVOKABLE void importPluginTemplates();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QVariantList protocolBuildMessagesForProtocol(const QString &protocolName) const;
    Q_INVOKABLE QVariantMap protocolBuildMessageDefinition(const QString &protocolName, const QString &messageName) const;
    Q_INVOKABLE QVariantMap ioDiagnostics() const;
    Q_INVOKABLE QVariantMap transportDiagnostics(const QString &transportName) const;
    Q_INVOKABLE QVariantMap protocolInfoPanel(const QString &panelId) const;

#ifdef HDGNSS_REGRESSION_TESTS
    void regressionApplyProtocolMessage(const ProtocolMessage &message) { applyProtocolMessage(message); }
    void regressionFlushUiRefresh() { flushUiRefresh(); }
    int regressionSatelliteCacheSize() const { return m_satellites.size(); }
#endif

signals:
    void locationChanged();
    void statusMessage(const QString &message);
    void sessionDirectoryChanged();
    void diagnosticsChanged();
    void fileSendDecodersChanged();
    void protocolBuildMessagesChanged();
    void protocolInfoTabsChanged();
    void protocolInfoPanelsChanged();
    void availableProtocolPluginsChanged();
    void availableAutomationPluginsChanged();
    // Emitted for every raw byte frame on any transport (Rx and Tx).
    // Automation plugins connect to this to observe the full data stream.
    void rawDataReceived(const QString &transportName, const QByteArray &bytes, bool isRx);

private:
    struct StreamCounters {
        qulonglong rxBytes = 0;
        qulonglong txBytes = 0;
        qulonglong rxChunks = 0;
        qulonglong txChunks = 0;
        int lastRxChunkSize = 0;
        int lastTxChunkSize = 0;
    };

    struct ProtocolInfoPanelState {
        ProtocolInfoPanel definition;
        QHash<QString, QVariant> values;
    };

    struct FileSendJob {
        QString filePath;
        QString decoderName;
        QString resolvedTarget;
        QList<QByteArray> packets;
        int packetIntervalMs = 0;
        int nextPacketIndex = 0;
    };

    // Qt slot wired to automation plugin's sendRequested() signal via QueuedConnection.
    // Using a real slot (not just Q_INVOKABLE) is required for old-style cross-DSO connect.
    Q_SLOT void onAutomationSendRequested(const QByteArray &bytes, const QString &target);

    void attachTransport(ITransport *transport);
    void handleTransportOpenStateChanged(ITransport *transport);
    void startTransportSession(ITransport *transport);
    void resetTransportState(const QString &transportName);
    void resetAllStreamState(bool clearUi);
    void clearUiState();
    void reloadProtocolPlugins();
    void reloadAutomationPlugins();
    void registerProtocolPlugin(IProtocolPlugin &plugin);
    void rebuildAvailableProtocolPlugins();
    void registerProtocolInfoPanels(const QList<ProtocolInfoPanel> &panels);
    bool sendRawCommand(const QByteArray &bytes, const QString &target);
    IProtocolPlugin *findProtocolPlugin(const QString &protocolName) const;
    bool sendFileCommand(const QString &filePath, const QString &decoderName, const QString &target, int packetIntervalMs);
    bool queueFileBytesForSend(const QByteArray &fileBytes, const QString &sourceLabel, const QString &decoderName, const QString &target, int packetIntervalMs);
    void downloadFileCommand(const QUrl &url, const QString &decoderName, const QString &target, int packetIntervalMs);
    void handleFileDownloadFinished(QNetworkReply *reply, const QUrl &sourceUrl, const QString &decoderName, const QString &target, int packetIntervalMs);
    void startNextFileSendJob();
    void sendNextFilePacket();
    void handleIncomingBytes(const QString &transportName, const QByteArray &bytes, DataDirection direction);
    void applyProtocolMessage(const ProtocolMessage &message);
    void recordDeviationSample(const ProtocolMessage &message);
    void scheduleUiRefresh();
    void flushUiRefresh();
    void refreshSatellites();
    QVariantMap streamCountersMap(const QString &transportName, const StreamCounters &counters) const;
    QVariantMap protocolInfoPanelMap(const ProtocolInfoPanelState &panel) const;
    QVariantList protocolInfoPanelItems(const ProtocolInfoPanelState &panel) const;
    QString formatProtocolInfoValue(const ProtocolInfoField &field, const QVariant &value) const;
    QString signalUsageText(const QString &group) const;
    static int deviationSamplePriority(const QString &messageName);

    TransportViewModel m_transportViewModel;
    RawLogModel m_rawLogModel;
    SatelliteModel m_satelliteModel;
    SignalModel m_signalModel;
    CommandButtonModel m_commandButtonModel;
    DeviationMapModel m_deviationMapModel;
    AppSettings *m_settings = nullptr;
    std::unique_ptr<TecMapOverlayModel> m_tecMapOverlayModel;
    RawRecorder m_rawRecorder;
    NmeaProtocolPlugin m_nmea;
    ProtocolDispatcher m_protocolDispatcher;
    ProtocolPluginLoader m_protocolPluginLoader;
    AutomationPluginLoader m_automationPluginLoader;
    QList<IAutomationPlugin *> m_activeAutomationPlugins;
    QVariantList m_availableProtocolPlugins;
    QStringList m_protocolPluginLoadErrors;
    QStringList m_protocolPluginSearchPaths;
    QVariantList m_availableAutomationPlugins;
    QStringList m_automationPluginLoadErrors;
    QStringList m_automationPluginSearchPaths;
    QStringList m_fileSendDecoders = {QStringLiteral("None")};
    QVariantList m_protocolBuildMessages;
    QHash<QString, QVariantMap> m_protocolBuildMessageDefinitions;
    GnssLocation m_location;
    QHash<QString, SatelliteInfo> m_satellites;
    QHash<QString, StreamCounters> m_streamCounters;
    QList<StreamChunker::BinaryFramer> m_binaryFramers;
    QHash<QString, StreamChunker> m_streamBuffers;
    QList<ProtocolInfoPanelState> m_protocolInfoPanels;
    QList<FileSendJob> m_fileSendQueue;
    std::optional<FileSendJob> m_activeFileSendJob;
    QNetworkAccessManager m_fileDownloadManager;
    QTimer m_fileSendTimer;
    QTimer m_uiRefreshTimer;
    QDateTime m_lastDeviationSampleUtcTime;
    int m_lastDeviationSamplePriority = -1;
    bool m_locationDirty = false;
    bool m_diagnosticsDirty = false;
    bool m_protocolInfoDirty = false;
    bool m_satellitesDirty = false;
    bool m_shuttingDown = false;
};

}  // namespace hdgnss
