#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QThread>
#include <iostream>
#include "src/core/TransportViewModel.h"

static bool waitForCount(QObject *plugin, int minimum, int timeoutMs) {
    QElapsedTimer timer;
    timer.start();
    while (plugin->property("discoveryCount").toInt() < minimum && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(10);
    }
    return plugin->property("discoveryCount").toInt() >= minimum;
}
static bool require(bool condition, const char *message) {
    if (!condition) std::cerr << message << std::endl;
    return condition;
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("hdgnss-regression"));
    QCoreApplication::setApplicationName(QStringLiteral("transport-discovery-regression"));
    if (argc != 2) return 1;
    const QString fixture = QString::fromLocal8Bit(argv[1]);
    qunsetenv("HDGNSS_PLUGIN_PATH");
    qputenv("HDGNSS_PLUGIN_DIR", QFileInfo(fixture).absolutePath().toLocal8Bit());
    QPluginLoader loader(fixture);
    QObject *plugin = loader.instance();
    if (!require(plugin, "fixture failed to load")) return 1;
    hdgnss::TransportViewModel viewModel;
    const QString id = QStringLiteral("discovery-regression");
    viewModel.setActiveTransport(id);
    if (!require(waitForCount(plugin, 1, 3000), "active plugin was not scanned")) return 1;

    int notifications = 0;
    QObject::connect(&viewModel, &hdgnss::TransportViewModel::transportDescriptorChanged,
                     &viewModel, [&](const QString &name) { if (name == id) ++notifications; });
    for (int i = 0; i < 3; ++i) viewModel.reloadTransportPlugins();
    notifications = 0;
    if (!QMetaObject::invokeMethod(plugin, "notifyDiscovery", Qt::DirectConnection) ||
        !require(notifications == 1, "reloading multiplied discovery notifications")) return 1;
    notifications = 0;
    if (!QMetaObject::invokeMethod(plugin, "notifyTransportOpenChanged", Qt::DirectConnection) ||
        !require(notifications == 2, "reloading multiplied transport open callbacks")) return 1;

    QTemporaryDir emptyDirectory;
    if (!emptyDirectory.isValid()) return 1;
    qputenv("HDGNSS_PLUGIN_DIR", emptyDirectory.path().toLocal8Bit());
    viewModel.reloadTransportPlugins();
    notifications = 0;
    if (!QMetaObject::invokeMethod(plugin, "notifyDiscovery", Qt::DirectConnection) ||
        !require(notifications == 0, "removed plugin still emitted discovery notifications")) return 1;
    if (!QMetaObject::invokeMethod(plugin, "notifyTransportOpenChanged", Qt::DirectConnection) ||
        !require(notifications == 0, "removed transport still emitted open callbacks")) return 1;

    qputenv("HDGNSS_PLUGIN_DIR", QFileInfo(fixture).absolutePath().toLocal8Bit());
    viewModel.reloadTransportPlugins();
    viewModel.setActiveTransport(id);
    const int beforeOpen = plugin->property("discoveryCount").toInt();
    if (!require(waitForCount(plugin, beforeOpen + 1, 3000), "discovery did not resume after reload")) return 1;
    if (!require(viewModel.openTransport(id), "fixture transport did not open")) return 1;
    const int whileOpen = plugin->property("discoveryCount").toInt();
    QElapsedTimer delay; delay.start();
    while (delay.elapsed() < 1200) { QCoreApplication::processEvents(QEventLoop::AllEvents, 20); QThread::msleep(10); }
    if (!require(plugin->property("discoveryCount").toInt() == whileOpen, "discovery ran while open")) return 1;
    viewModel.closeTransport(id);
    if (!require(waitForCount(plugin, whileOpen + 1, 3000), "discovery did not resume after close")) return 1;
    std::cout << "PASS discovery scheduling and plugin reload" << std::endl;
    return 0;
}
