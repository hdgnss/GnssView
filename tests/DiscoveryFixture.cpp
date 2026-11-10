#include <QObject>
#include "hdgnss/ITransport.h"
#include "hdgnss/ITransportPlugin.h"

class DiscoveryTransport final : public hdgnss::ITransport {
public:
    DiscoveryTransport() : ITransport(QStringLiteral("discovery-regression")) {}
    bool openWithSettings(const QVariantMap &) override { opened = true; emitOpenChanged(); return true; }
    void close() override { if (opened) { opened = false; emitOpenChanged(); } }
    bool isOpen() const override { return opened; }
    bool sendData(const QByteArray &) override { return false; }
    QStringList capabilities() const override { return {}; }
private:
    bool opened = false;
};

class DiscoveryFixture final : public QObject, public hdgnss::ITransportPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID HDGNSS_TRANSPORT_PLUGIN_IID)
    Q_INTERFACES(hdgnss::ITransportPlugin)
    Q_PROPERTY(int discoveryCount READ discoveryCount)
public:
    QString transportId() const override { return QStringLiteral("discovery-regression"); }
    QString transportDisplayName() const override { return transportId(); }
    hdgnss::ITransport *transportInstance() override { return &transport; }
    int discoveryCount() const { return count; }
    Q_INVOKABLE void requestDiscovery() { ++count; }
    Q_INVOKABLE void notifyDiscovery() { emit discoveryChanged(transportId()); }
    Q_INVOKABLE void notifyTransportOpenChanged() {
        transport.openWithSettings({});
        transport.close();
    }
signals:
    void discoveryChanged(const QString &transportId);
private:
    DiscoveryTransport transport;
    int count = 0;
};

#include "DiscoveryFixture.moc"
