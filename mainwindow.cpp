#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QtCharts/QValueAxis>
#include <QToolTip>
#include <QRegularExpression>
#include "config.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Set up the UI from the .ui file
    ui.setupUi(this);
    applyTheme();
    
    // Set window properties
    setWindowTitle(tr(PROJECT_NAME));
    
    // Set column stretch factors for gridLayout
    ui.gridLayout->setColumnStretch(0, 1);
    ui.gridLayout->setColumnStretch(1, 1);
    ui.gridLayout->setColumnStretch(2, 1);

    // Set row stretch factors for gridLayout
    ui.gridLayout->setRowStretch(0, 1);
    ui.gridLayout->setRowStretch(1, 0);
    ui.gridLayout->setRowStretch(2, 2);
    ui.gridLayout->setRowStretch(3, 2);

    ui.pushButtonUdpTrue->setVisible(false);
    ui.pushButtonSerialTrue->setVisible(false);
    ui.label_2->setVisible(false);
    ui.label_5->setVisible(false);
    ui.lineEditTureSerial->setVisible(false);
    ui.lineEditTureUdp->setVisible(false);
    ui.tabWidgetInfo->setTabVisible(1, false);
    //ui.chartSnrL5->setVisible(false);

    // Initialize objects
    mUdpServer = new UdpServer();
    mSerialPort = new QSerialPort();
    mNmeaParser = new NmeaParser(true);
    
    // Set up the charts
    setupSnrChart(L1_CHART);
    setupSnrChart(L5_CHART);
    setupSkyChart();
    
    // Connect signals and slots
    connectSignals();

    // Initially populate the port list
    updatePortList();

    if (readSettings(PROJECT_NAME, "Version").isEmpty())
    {
        writeSettings(PROJECT_NAME, "Version", PROJECT_VER);
        writeSettings("COMMAND", "Cmd0Text", "Example");
        writeSettings("COMMAND", "Cmd0Cmd", "gnss start");
    }
    initUI();
}



MainWindow::~MainWindow()
{
    // Proper cleanup
    if (mUdpServer) {
        if(mUdpServer->isListening()){
            mUdpServer->stopServer();
        }
        delete mUdpServer;
        mUdpServer = nullptr;
    }
    
    if (mSerialPort) {
        if (mSerialPort->isOpen()) {
            mSerialPort->close();
        }
        delete mSerialPort;
        mSerialPort = nullptr;
    }
    
    if (mNmeaParser) {
        delete mNmeaParser;
        mNmeaParser = nullptr;
    }
}

QString MainWindow::readSettings(QString group, QString name)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", PROJECT_NAME);
    settings.beginGroup(group);
    QString value = settings.value(name, "").toString();
    settings.endGroup();

    return value;
}

void MainWindow::writeSettings(QString group, QString name, QString value)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", PROJECT_NAME);

    settings.beginGroup(group);
    settings.setValue(name, value);
    settings.endGroup();
}

void MainWindow::updatePortList()
{
    ui.comboBoxSerialPort->clear();
    
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui.comboBoxSerialPort->addItem(port.portName() + " " + port.description(), port.portName());
    }
    
    // If no ports are found, disable the connect button
    ui.pushButtonSerialConnect->setEnabled(ui.comboBoxSerialPort->count() > 0);
}


void MainWindow::updateSatelliteData(const QVector<GnssSatellite>& satellites)
{
    if (!mSnrL1Chart || !mSnrL5Chart) {
        return;
    }
    
    // Update both charts with the satellite data
    updateChartWithSatellites(satellites, L1_CHART);
    // updateChartWithSatellites(satellites, L5_CHART);
}


void MainWindow::updateChartWithSatellites(const QVector<GnssSatellite>& satellites, ChartType chartType)
{
    QChart *chart = nullptr;
    QChartView *chartView = nullptr;
    QStringList signalType;
    
    // Set up references based on chart type
    if (chartType == L1_CHART) {
        chart = mSnrL1Chart;
        chartView = ui.chartSnrL1;
        signalType = {"L1"};
    } else { // L5_CHART
        chart = mSnrL5Chart;
        chartView = ui.chartSnrL5;
        signalType = {"L5","L2","L6"};
    }
    
    // Create a new bar set
    QBarSet *barSet = new QBarSet("");
    
    // Clear previous categories
    mSatelliteCategories.clear();
    
    // Map to track which satellites we've already added
    QMap<QString, int> addedSatellites;
    
    // Process each satellite
    for (const GnssSatellite& sat : satellites) {
        // Compare signal type directly with the mapped signal
        QString signalKey = QString("%1%2").arg(sat.system).arg(sat.signal);
        if (signalType.contains(mSignalMap[signalKey])) {
            QString prefix = mSystemMap[sat.system];
            
            // Create the satellite identifier
            QString satId = QString("%1%2").arg(prefix).arg(sat.prn, 2, 10, QChar('0'));
            
            // If we've already added this satellite, skip it
            if (addedSatellites.contains(satId)) {
                continue;
            }
            
            // Add the satellite to our tracking map
            addedSatellites[satId] = sat.snr;
            
            // Add the satellite to our categories and add its SNR to the bar set
            mSatelliteCategories << satId;
            *barSet << sat.snr;
        }
    }
    
    // Remove old series
    chart->removeAllSeries();
    
    // Create new series with our updated data
    QBarSeries *series = new QBarSeries();
    series->append(barSet);
    
    // Enable labels for the bar set
    barSet->setLabelFont(QFont("Arial", 7));
    barSet->setLabelColor(Qt::black);
    
    // Enable labels on the series and position them
    series->setLabelsVisible(true);
    series->setLabelsPosition(QBarSeries::LabelsPosition::LabelsInsideEnd);
    
    // // Connect to the hovered signal to show tooltips (optional)
    // connect(series, &QBarSeries::hovered, this, [=](bool status, int index, QBarSet *barSet) {
    //     if (status && index >= 0 && index < mSatelliteCategories.size()) {
    //         QString satId = mSatelliteCategories[index];
    //         int snr = barSet->at(index);
    //         QToolTip::showText(QCursor::pos(), QString("%1: %2 dB-Hz").arg(satId).arg(snr));
    //     }
    // });
    
    chart->addSeries(series);
    
    // Remove old axes
    QList<QAbstractAxis*> axes = chart->axes();
    for (QAbstractAxis* axis : axes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    // Create and set up X axis
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(mSatelliteCategories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    // Rotate labels by 90 degrees
    axisX->setLabelsAngle(90);
    
    QFont axisFont = axisX->labelsFont();
    axisFont.setPointSize(5);
    axisX->setLabelsFont(axisFont);
    
    // Create and set up Y axis
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 60);
    axisY->setTitleText("");
    axisY->setTickCount(7);
    
    // Make the Y-axis font smaller
    QFont axisYFont = axisY->labelsFont();
    axisYFont.setPointSize(6);
    axisY->setLabelsFont(axisYFont);
    
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    // Set the value format for labels to show no decimal places
    series->setLabelsFormat("@value");
    
    // Apply colors
    recolorSnrBars(chartView, chart);
}

// Unified method for recoloring chart bars
void MainWindow::recolorSnrBars(QChartView* chartView, QChart* chart)
{
    // Find all bar rect items
    QList<QGraphicsRectItem *> rectItems;
    for (QGraphicsItem *item : chartView->items()) {
        if (QGraphicsRectItem *rect = qgraphicsitem_cast<QGraphicsRectItem *>(item)) {
            if (rect->parentItem() != chart && rect->parentItem()->parentItem() == chart) {
                rectItems << rect;
            }
        }
    }

    // Define colors for each constellation
    QColor gpsColor(0, 120, 215);      // Blue for GPS (G)
    QColor glonassColor(232, 17, 35);  // Red for GLONASS (R)
    QColor galileoColor(255, 140, 0);  // Orange for Galileo (E)
    QColor beidouColor(0, 153, 0);     // Green for BeiDou (B)
    QColor qzssColor(138, 43, 226);    // Purple for QZSS (J)
    QColor irnssColor(255, 215, 0);    // Gold for IRNSS/NavIC (I)
    QColor sbasColor(169, 169, 169);   // Gray for SBAS (S)
    QColor unknownColor(100, 100, 100); // Dark gray for unknown

    // Create a map of actual indices to satellite prefixes
    QMap<int, QColor> rectColorMap;
    for (int i = 0; i < mSatelliteCategories.size(); ++i) {
        QString category = mSatelliteCategories[i];
        QColor color;
        
        if (category.startsWith("G")) {
            color = gpsColor;
        } else if (category.startsWith("R")) {
            color = glonassColor;
        } else if (category.startsWith("E")) {
            color = galileoColor;
        } else if (category.startsWith("B")) {
            color = beidouColor;
        } else if (category.startsWith("J")) {
            color = qzssColor;
        } else if (category.startsWith("I")) {
            color = irnssColor;
        } else if (category.startsWith("S")) {
            color = sbasColor;
        } else {
            color = unknownColor;
        }
        
        // In the chart, items are in reverse order, so map to the reverse index
        int reverseIndex = mSatelliteCategories.size() - 1 - i;
        if (reverseIndex >= 0 && reverseIndex < rectItems.size()) {
            rectColorMap[reverseIndex] = color;
        }
    }
    
    // Apply the colors based on our mapping
    for (int i = 0; i < rectItems.size(); ++i) {
        if (rectColorMap.contains(i)) {
            QColor color = rectColorMap[i];
            rectItems[i]->setBrush(color);
            rectItems[i]->setPen(QPen(color.darker(120)));
        }
    }
}

// Helper method to apply common setup to a chart view
void MainWindow::applyChartToView(QChart* chart, QChartView* view)
{
    view->setChart(chart);
    view->setRenderHint(QPainter::Antialiasing);
}

// Unified setup method for both L1 and L5 charts
void MainWindow::setupSnrChart(ChartType chartType)
{
    QChart **chart;
    QChartView *chartView;
    
    // Select the appropriate chart components based on type
    if (chartType == L1_CHART) {
        chart = &mSnrL1Chart;
        chartView = ui.chartSnrL1;
    } else { // L5_CHART
        chart = &mSnrL5Chart;
        chartView = ui.chartSnrL5;
    }
    
    // Create a chart
    *chart = new QChart();
    (*chart)->setTitle("");
    (*chart)->setAnimationOptions(QChart::NoAnimation);

    // Create empty bar set for SNR values
    QBarSet *barSet = new QBarSet("");
    
    // Create empty bar series and add the bar set
    QBarSeries *series = new QBarSeries();
    series->append(barSet);
    (*chart)->addSeries(series);
    
    // Create axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    QValueAxis *axisY = new QValueAxis();
    
    // Set up Y axis
    axisY->setRange(0, 60);
    axisY->setTitleText("");
    axisY->setTickCount(7); 
    
    QFont axisYFont = axisY->labelsFont();
    axisYFont.setPointSize(6);
    axisY->setLabelsFont(axisYFont);
    
    // Add axes to chart
    (*chart)->addAxis(axisX, Qt::AlignBottom);
    (*chart)->addAxis(axisY, Qt::AlignLeft);
    
    // Attach series to axes
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    
    // Hide the legend
    (*chart)->legend()->setVisible(false);
    
    // Apply chart to view
    applyChartToView(*chart, chartView);
}


// Sky chart setup with cleaner organization
void MainWindow::setupSkyChart()
{
    // Create a polar chart for the sky view
    mSkyChart = new QPolarChart();
    mSkyChart->setTitle("");
    mSkyChart->setAnimationOptions(QChart::NoAnimation);
    
    // Create angle (azimuth) axis - 0 is North, 90 is East
    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setRange(0, 360);
    angularAxis->setLabelFormat("%.0f");
    angularAxis->setTickCount(9); // N, NE, E, SE, S, SW, W, NW, N
    angularAxis->setLabelsVisible(true);
    
    // Create radial (elevation) axis - 0 at edge (0° elevation), 90 at center (90° elevation)
    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setRange(0, 90);
    radialAxis->setTickCount(4); // 0°, 30°, 60°, 90° elevation
    radialAxis->setLabelsVisible(false);  // Explicitly make labels visible


    // Hide the legend
    mSkyChart->legend()->setVisible(false);
    
    // Set up the chart - use the proper orientation for polar chart axes
    mSkyChart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);
    mSkyChart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);
    
    // Apply chart to view using the helper method
    applyChartToView(mSkyChart, ui.chartSky);
    
    // Set background to a light color
    mSkyChart->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    
    // Initialize with empty satellites
    QVector<GnssSatellite> emptySats;
    updateSkyChart(emptySats);
}


void MainWindow::updateSkyChart(const QVector<GnssSatellite>& satellites)
{
    if (!mSkyChart) {
        return;
    }
    
    // Get the axes
    QValueAxis *angularAxis = nullptr;
    QValueAxis *radialAxis = nullptr;
    
    // Find the axes by type - first is angular, second is radial in a polar chart
    const auto axes = mSkyChart->axes();
    if (axes.size() >= 2) {
        angularAxis = qobject_cast<QValueAxis*>(axes.at(0));
        radialAxis = qobject_cast<QValueAxis*>(axes.at(1));
    }
    
    if (!angularAxis || !radialAxis) {
        return; // Can't continue without both axes
    }
    
    // Remove all existing series
    mSkyChart->removeAllSeries();
    
    // Define colors for each constellation
    QColor gpsColor(0, 120, 215);      // Blue for GPS (G)
    QColor glonassColor(232, 17, 35);  // Red for GLONASS (R)
    QColor galileoColor(255, 140, 0);  // Orange for Galileo (E)
    QColor beidouColor(0, 153, 0);     // Green for BeiDou (B)
    QColor qzssColor(138, 43, 226);    // Purple for QZSS (J)
    QColor irnssColor(255, 215, 0);    // Gold for IRNSS/NavIC (I)
    QColor sbasColor(169, 169, 169);   // Gray for SBAS (S)
    QColor unknownColor(100, 100, 100); // Dark gray for unknown


    // Create a scatter series for each satellite system
    QScatterSeries *gpsSeries = new QScatterSeries();
    QScatterSeries *glonassSeries = new QScatterSeries();
    QScatterSeries *galileoSeries = new QScatterSeries();
    QScatterSeries *beidouSeries = new QScatterSeries();
    QScatterSeries *qzssSeries = new QScatterSeries();
    QScatterSeries *irnssSeries = new QScatterSeries();
    QScatterSeries *sbasSeries = new QScatterSeries();
    QScatterSeries *unknownSeries = new QScatterSeries();
    
    // Set up series properties
    gpsSeries->setMarkerSize(12);
    glonassSeries->setMarkerSize(12);
    galileoSeries->setMarkerSize(12);
    beidouSeries->setMarkerSize(12);
    qzssSeries->setMarkerSize(12);
    irnssSeries->setMarkerSize(12);
    sbasSeries->setMarkerSize(12);
    unknownSeries->setMarkerSize(12);
    
    gpsSeries->setColor(gpsColor);
    glonassSeries->setColor(glonassColor);
    galileoSeries->setColor(galileoColor);
    beidouSeries->setColor(beidouColor);
    qzssSeries->setColor(qzssColor);
    irnssSeries->setColor(irnssColor);
    sbasSeries->setColor(sbasColor);
    unknownSeries->setColor(unknownColor);
    
    // Process each satellite
    for (const GnssSatellite& sat : satellites) {
        // Skip satellites with elevation <= 0
        if (sat.elevation <= 0) {
            continue;
        }
        
        // Convert to polar coordinates
        // Azimuth is the angular coordinate (0-360°)
        // Elevation is the radial coordinate (0-90°)
        qreal azimuth = sat.azimuth;
        qreal elevation = 90.0 - sat.elevation; // Inverse mapping: 90° at center, 0° at edge
        
        // Create a data point
        QPointF point(azimuth, elevation);
        
        // Add the point to the appropriate series based on system
        switch (sat.system) {
            case 1: // GPS
                gpsSeries->append(point);
                break;
            case 2: // GLONASS
                glonassSeries->append(point);
                break;
            case 3: // Galileo
                galileoSeries->append(point);
                break;
            case 4: // BeiDou
                beidouSeries->append(point);
                break;
            case 5: // QZSS
                qzssSeries->append(point);
                break;
            case 6: // IRNSS/NavIC
                irnssSeries->append(point);
                break;
            case 7: // SBAS
                sbasSeries->append(point);
                break;
            default: // Unknown
                unknownSeries->append(point);
                break;
        }
    }
    
    // Helper function to add a series with proper axis attachment
    auto addSeriesToChart = [this, angularAxis, radialAxis](QScatterSeries* series) {
        if (!series->points().isEmpty()) {
            mSkyChart->addSeries(series);
            series->attachAxis(angularAxis);
            series->attachAxis(radialAxis);
        } else {
            delete series;
        }
    };
    
    // Add series to chart if they have points
    addSeriesToChart(gpsSeries);
    addSeriesToChart(glonassSeries);
    addSeriesToChart(galileoSeries);
    addSeriesToChart(beidouSeries);
    addSeriesToChart(qzssSeries);
    addSeriesToChart(irnssSeries);
    addSeriesToChart(sbasSeries);
    addSeriesToChart(unknownSeries);
}


void MainWindow::loadStyleSheet(const QString& theme)
{
    QFile styleFile(theme);
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        setStyleSheet(styleSheet);
        styleFile.close();
    }
}

// Improved UDP connection handling
void MainWindow::connectUdpServer()
{
    if (mUdpServer->isListening()) {
        disconnectUdpServer();
        return;
    }

    quint16 port = ui.lineEditUdpPort->text().toUShort();
    if (!mUdpServer->startServer(port)) {
        ui.textEditNmea->append(tr("Failed to start UDP server on port %1").arg(port));
        return;
    }
    
    ui.pushButtonUdpConnect->setText(tr("Disconnect"));
    ui.textEditNmea->append(tr("UDP server started on port %1").arg(port));
    ui.lineEditUdpAddress->setEnabled(false);
    ui.lineEditUdpPort->setEnabled(false);
    ui.tabWidgetDevice->setTabEnabled(1, false);
}

void MainWindow::disconnectUdpServer()
{
    if (mUdpServer->isListening()) {
        mUdpServer->stopServer();
        ui.pushButtonUdpConnect->setText(tr("Connect"));
        ui.textEditNmea->append(tr("UDP server stopped"));
        ui.lineEditUdpAddress->setEnabled(true);
        ui.lineEditUdpPort->setEnabled(true);
        ui.tabWidgetDevice->setTabEnabled(1, true);
    }
}

// Improved signal connection with clear organization
void MainWindow::connectSignals()
{
    // UDP connections
    connect(ui.pushButtonUdpConnect, &QPushButton::clicked, this, &MainWindow::connectUdpServer);
    connect(mUdpServer, &UdpServer::newDataReceived, this, &MainWindow::onReceivedUdpData);

    // Serial port connections
    connect(ui.pushButtonSerialConnect, &QPushButton::clicked, this, [this]() {
        if (mSerialPort->isOpen()) {
            closeSerialPort();
        } else {
            openSerialPort();
        }
    });
    connect(mSerialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(mSerialPort, &QSerialPort::readyRead, this, &MainWindow::onReadSerialData);
    
    // NMEA parser connections
    connect(mNmeaParser, &NmeaParser::newNmeaParseDone, this, &MainWindow::onNmeaParseDone);
    
    // Menu actions
    connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::about);
}


// Centralized UI update method for status display
void MainWindow::updateStatusDisplay(const GnssInfo& gnssInfo)
{
    // Update position and status information
    ui.labelLat->setText(QString::number(gnssInfo.position.latitude, 'f', 8));
    ui.labelLon->setText(QString::number(gnssInfo.position.longitude, 'f', 8));
    ui.labelAlt->setText(QString::number(gnssInfo.position.altitude, 'f', 2));
    ui.labelCoures->setText(QString::number(gnssInfo.position.course, 'f', 2));
    ui.labelSpeed->setText(QString::number(gnssInfo.position.speed, 'f', 2));
    ui.labelDop->setText(QString::number(gnssInfo.position.dop, 'f', 2));
    ui.labelQuality->setText(QString::number(gnssInfo.position.quality));
    ui.labelMode->setText(gnssInfo.position.mode);
    
    // Update date and time
    QDateTime datetime;
    datetime.setTimeZone(QTimeZone::utc());
    datetime.setMSecsSinceEpoch(gnssInfo.position.utc);
    ui.labelDate->setText(datetime.toString("yyyy-MM-dd"));
    ui.labelTime->setText(datetime.toString("HH:mm:ss.zzz"));
}

void MainWindow::onNmeaParseDone(GnssInfo gnssInfo)
{
    // Update satellite visualizations
    updateSatelliteData(gnssInfo.satellites);
    updateSkyChart(gnssInfo.satellites);
    
    // Update position and status information
    updateStatusDisplay(gnssInfo);
}


void MainWindow::onUdpData(const QByteArray &data)
{
    // Convert the binary data to a string safely, handling non-ASCII bytes
    QString dataStr = QString::fromLatin1(data);
    
    // NMEA messages typically start with $ and end with CR+LF
    // Use a regex to find all NMEA messages in the data chunk
    QRegularExpression nmeaRegex("(\\$[A-Z]{4,8},.*\\*[0-9a-fA-F]{2,2})");
    QRegularExpressionMatchIterator matches = nmeaRegex.globalMatch(dataStr);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString nmeaMessage = match.captured(0);
        
        // Parse the NMEA message
        mNmeaParser->parseNMEASentence(nmeaMessage, false);
        
        // Append to the UI text edit
        ui.textEditNmea->append(tr("%1").arg(nmeaMessage));
    }
    
    // Auto-scroll to the bottom
    QScrollBar *scrollBar = ui.textEditNmea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onReadSerialData()
{
    QByteArray data = mSerialPort->readAll();
    qDebug() << "R:"<< data;
    onUdpData(data);
}

void MainWindow::onReceivedUdpData(const QByteArray &data, const QHostAddress &sender, quint16 senderPort)
{
    onUdpData(data);
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    if (error == QSerialPort::ResourceError || error == QSerialPort::PermissionError) {
        QMessageBox::critical(this, tr("Critical Error"), mSerialPort->errorString());
        closeSerialPort();
    }
}


void MainWindow::openSerialPort()
{
    if (ui.comboBoxSerialPort->count() == 0) {
        QMessageBox::warning(this, tr("Warning"), tr("No serial ports available."));
        return;
    }

    qDebug() << "BaudRate:" << (ui.comboBoxBaudRate->currentText());
    
    mSerialPort->setPortName(ui.comboBoxSerialPort->currentData().toString());
    mSerialPort->setBaudRate(qint32(ui.comboBoxBaudRate->currentText().toInt()));
    mSerialPort->setDataBits(static_cast<QSerialPort::DataBits>(ui.comboBoxDataBits->currentText().toInt()));
    mSerialPort->setParity(QSerialPort::NoParity);//ui.comboBoxParity->currentData().toInt()));
    mSerialPort->setStopBits(static_cast<QSerialPort::StopBits>(ui.comboBoxStopBits->currentText().toInt()));
    
    if (mSerialPort->open(QIODevice::ReadWrite)) {
        ui.comboBoxSerialPort->setEnabled(false);
        ui.comboBoxDataBits->setEnabled(false);
        ui.comboBoxParity->setEnabled(false);
        ui.comboBoxBaudRate->setEnabled(false);
        ui.comboBoxStopBits->setEnabled(false);
        ui.pushButtonSerialConnect->setText(tr("Disconnect"));
        ui.tabWidgetDevice->setTabEnabled(0, false);
        
        ui.textEditNmea->append(tr("Connected to %1 : %2, %3, %4, %5")
                             .arg(mSerialPort->portName())
                             .arg(QString::number(mSerialPort->baudRate()))
                             .arg(QString::number(mSerialPort->dataBits()))
                             .arg(ui.comboBoxParity->currentText())
                             .arg(ui.comboBoxStopBits->currentText())
                            );
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open port %1: %2, %3")
                                              .arg(mSerialPort->portName())
                                              .arg(mSerialPort->errorString()) .arg(qint32(ui.comboBoxBaudRate->currentData().toInt())));
    }
}

void MainWindow::closeSerialPort()
{
    if (mSerialPort->isOpen()) {
        mSerialPort->close();
        ui.textEditNmea->append(tr("Disconnected from %1").arg(mSerialPort->portName()));
        
        ui.comboBoxSerialPort->setEnabled(true);
        ui.comboBoxBaudRate->setEnabled(true);
        ui.comboBoxDataBits->setEnabled(true);
        ui.comboBoxParity->setEnabled(true);
        ui.comboBoxStopBits->setEnabled(true);
        ui.pushButtonSerialConnect->setText(tr("Connect"));
        ui.tabWidgetDevice->setTabEnabled(0, true);
    }
}

void MainWindow::applyTheme()
{
    loadStyleSheet(":qss/style.qss");
}


void MainWindow::initUI()
{
    const QList<QPushButton *> cmdPushButtonList = {ui.pushButtonCmd1, ui.pushButtonCmd2, ui.pushButtonCmd3, ui.pushButtonCmd4, ui.pushButtonCmd5, ui.pushButtonCmd6, ui.pushButtonCmd7, ui.pushButtonCmd8, ui.pushButtonCmd9, ui.pushButtonCmd10, ui.pushButtonCmd11, ui.pushButtonCmd12, ui.pushButtonCmd13, ui.pushButtonCmd14};
    
    for (int i = 0; i < cmdPushButtonList.size(); ++i)
    {
        QString text = readSettings("COMMAND", "Cmd"+QString::number(i)+"Text");
        QString cmd = readSettings("COMMAND", "Cmd"+QString::number(i)+"Cmd");
        if (!text.isEmpty() && !cmd.isEmpty())
        {
            cmdPushButtonList.at(i)->setEnabled(true);
            cmdPushButtonList.at(i)->setText(text);
            mPushButtonCmd[cmdPushButtonList.at(i)] = cmd;
            connect(cmdPushButtonList.at(i), &QPushButton::clicked, this, &MainWindow::onPushButtonCmdClicked);
        }
        else{
            cmdPushButtonList.at(i)->setEnabled(false);
        }
    }
}

void MainWindow::onPushButtonCmdClicked()
{
    QPushButton *senderPushButton = qobject_cast<QPushButton *>(sender());

    if (!mPushButtonCmd[senderPushButton].isEmpty())
    {
        if (mSerialPort->isOpen())
        {
            qDebug() << "Send: "<< mPushButtonCmd[senderPushButton];
            mSerialPort->write(mPushButtonCmd[senderPushButton].toUtf8());
            mSerialPort->write("\r\n");
        }
    }
}

void MainWindow::about()
{
    QString aboutTitle = tr("About ") + QString(PROJECT_NAME);
    QString aboutText = tr("%1\n\n"
                         "Version %2\n\n"
                         "A professional tool for show GNSS status.\n\n")
                         .arg(PROJECT_NAME)
                         .arg(PROJECT_VER);
    
    QMessageBox::about(this, aboutTitle, aboutText);
}