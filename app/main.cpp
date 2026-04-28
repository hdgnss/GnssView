#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QWindow>

#include "src/platform/MacWindowStyler.h"
#include "src/core/AppController.h"
#include "src/core/AppSettings.h"
#include "src/core/TransportViewModel.h"
#include "src/core/UpdateChecker.h"
#include "src/models/CommandButtonModel.h"
#include "src/models/DeviationMapModel.h"
#include "src/models/RawLogModel.h"
#include "src/models/SatelliteModel.h"
#include "src/models/SignalModel.h"
#include "src/tec/TecMapOverlayModel.h"

namespace {
QString pickFontFamily(const QStringList &candidates, const QString &fallback) {
    const QStringList available = QFontDatabase::families();
    for (const QString &candidate : candidates) {
        if (available.contains(candidate, Qt::CaseInsensitive)) {
            return candidate;
        }
    }
    return fallback;
}

QIcon loadAppIcon(const QString &applicationDirPath) {
    const QStringList candidates = {
        QDir(applicationDirPath).filePath(QStringLiteral("assets/GnssView.ico")),
        QDir(applicationDirPath).filePath(QStringLiteral("assets/gnssview-icon.ico"))
    };

    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            const QIcon icon(path);
            if (!icon.isNull()) {
                return icon;
            }
        }
    }
    return {};
}
}  // namespace

int main(int argc, char *argv[]) {
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QQuickStyle::setFallbackStyle(QStringLiteral("Basic"));

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("GnssView");
    QCoreApplication::setApplicationVersion(hdgnss::UpdateChecker::applicationVersion());
    QCoreApplication::setOrganizationName("hdgnss");

    const QString bodyFontFamily = pickFontFamily(
        {QStringLiteral("SF Pro Text"),
         QStringLiteral("Helvetica Neue"),
         QStringLiteral("PingFang SC"),
         QStringLiteral("Hiragino Sans GB"),
         QStringLiteral("Arial")},
        app.font().family());

    const QString monoFontFamily = pickFontFamily(
        {QStringLiteral("SF Mono"),
         QStringLiteral("Menlo"),
         QStringLiteral("Monaco"),
         QStringLiteral("Courier New")},
        QStringLiteral("Menlo"));

    QFont appFont(bodyFontFamily);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);

    const QIcon appIcon = loadAppIcon(QCoreApplication::applicationDirPath());
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    hdgnss::AppSettings settings;
    hdgnss::AppController controller(&settings);
    hdgnss::UpdateChecker updateChecker;
    const bool rawDataScrollDebug = qEnvironmentVariableIntValue("HDGNSS_RAWDATA_SCROLL_DEBUG") > 0;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("uiBodyFontFamily", bodyFontFamily);
    engine.rootContext()->setContextProperty("uiMonoFontFamily", monoFontFamily);
    engine.rootContext()->setContextProperty("rawDataScrollDebug", rawDataScrollDebug);
    engine.rootContext()->setContextProperty("appSettings", &settings);
    engine.rootContext()->setContextProperty("appController", &controller);
    engine.rootContext()->setContextProperty("updateChecker", &updateChecker);
    engine.rootContext()->setContextProperty("transportViewModel", controller.transportViewModel());
    engine.rootContext()->setContextProperty("rawLogModel", controller.rawLogModel());
    engine.rootContext()->setContextProperty("satelliteModel", controller.satelliteModel());
    engine.rootContext()->setContextProperty("signalModel", controller.signalModel());
    engine.rootContext()->setContextProperty("commandButtonModel", controller.commandButtonModel());
    engine.rootContext()->setContextProperty("deviationMapModel", controller.deviationMapModel());
    engine.rootContext()->setContextProperty("tecMapOverlayModel", controller.tecMapOverlayModel());
    // AutoTest plugin controller – nullptr when the plugin is not installed;
    // AutoTestPanel.qml checks hasController and degrades gracefully.
    engine.rootContext()->setContextProperty("autoTestController", controller.automationController());

    const QString qmlPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("qml/Main.qml"));
    const QUrl url = QUrl::fromLocalFile(qmlPath);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);
    if (settings.checkForUpdatesOnStartup()) {
        QTimer::singleShot(1500, &updateChecker, [&updateChecker]() {
            updateChecker.checkForUpdates(true);
        });
    }
    if (!engine.rootObjects().isEmpty()) {
        if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            if (!appIcon.isNull()) {
                window->setIcon(appIcon);
            }
            hdgnss::applyPlatformWindowStyle(window);
        }
    }
    return app.exec();
}
