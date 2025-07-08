#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QPolarChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QGraphicsDropShadowEffect>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QScrollBar>
#include "ui_mainwindow.h"
#include "udpserver.h"
#include "nmeaparser.h"

// Enum for chart types to make the code more maintainable
enum ChartType {
    L1_CHART,
    L5_CHART
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void updateSatelliteData(const QVector<GnssSatellite>& satellites);
    ~MainWindow();

    private slots:
    void openSerialPort();
    void closeSerialPort();
    void onReadSerialData();
    void handleError(QSerialPort::SerialPortError error);
    void connectUdpServer();
    void disconnectUdpServer();
    void onUdpData(const QByteArray &data);
    void onReceivedUdpData(const QByteArray &data, const QHostAddress &sender, quint16 senderPort);
    void onNmeaParseDone(GnssInfo gnssinfo);
    void onPushButtonCmdClicked();
    void about();


    private:
    // Connection-related members
    UdpServer *mUdpServer;
    QSerialPort *mSerialPort;
    QByteArray mReceivedData;
    NmeaParser *mNmeaParser;

    // Chart-related members
    QChart *mSnrL1Chart;
    QChart *mSnrL5Chart;
    QPolarChart *mSkyChart;
    
    QStringList mSatelliteCategories;
    
    // Chart setup methods
    void setupSnrChart(ChartType chartType);
    // Helper to apply chart to view
    void applyChartToView(QChart* chart, QChartView* view);
    void setupSkyChart();

    // Unified chart update methods
    void updateChartWithSatellites(const QVector<GnssSatellite>& satellites, ChartType chartType);
    void updateSkyChart(const QVector<GnssSatellite>& satellites);
    void recolorSnrBars(QChartView* chartView, QChart* chart);

    // Connection and UI methods
    void connectSignals();
    void applyTheme();
    void initUI();
    void loadStyleSheet(const QString& theme);
    void updatePortList();
    void updateStatusDisplay(const GnssInfo& gnssInfo);

    QString readSettings(QString group, QString name);
    void writeSettings(QString group, QString name, QString value);

    QMap<QPushButton *, QString> mPushButtonCmd;

    Ui::MainWindow ui;
}; 

#endif // MAINWINDOW_H