/*
 * Copyright (C) 2025 HDGNSS
 *
 *  H   H  DDDD   GGG  N   N  SSSS  SSSS
 *  H   H  D   D G     NN  N  S     S
 *  HHHHH  D   D G  GG N N N   SSS   SSS
 *  H   H  D   D G   G N  NN      S     S
 *  H   H  DDDD   GGG  N   N  SSSS  SSSS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPortInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CommunicationManager;
class NmeaParser;
class DataLogger;
class UpdateChecker;

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QPolarChart>
#include <QScatterSeries>
#include <QValueAxis>

#include "core/commanddefinition.h"
#include "core/gnsstype.h"
#include "core/nmeaparser.h"
#include "core/ntripclient.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleButtonOpenSerialAClicked();
    void handleButtonOpenSerialBClicked();
    void handleButtonTcpConnectClicked();
    void handleButtonUdpBindClicked();
    void handleActionSettingsTriggered();
    void handleButtonAddCommandClicked();
    void handleActionCheckUpdatesTriggered();
    void handleButtonOpenNtripClicked();

    // Custom slots (renamed to avoid QMetaObject warnings)
    void handleNmeaParseDone(GnssInfo gnssInfo);
    void handleUpdateAvailable(const QString &version, const QString &releaseUrl, const QString &releaseNotes);
    void handleNoUpdateAvailable();
    void handleUpdateCheckFailed(const QString &error);
    void handleNtripDataReceived(const QByteArray &data);

private:
    Ui::MainWindow *ui;
    CommunicationManager *m_comms;
    NmeaParser *m_parser;
    DataLogger *m_serialALogger;
    DataLogger *m_serialBLogger;
    DataLogger *m_tcpLogger;
    DataLogger *m_udpLogger;
    DataLogger *m_ntripLogger;
    UpdateChecker *m_updateChecker;
    NTRIPClient *m_ntripClient;

    // Chart-related members
    QChart *m_snrL1Chart;
    QChart *m_snrL5Chart;
    QPolarChart *m_skyChart;

    QList<CommandDefinition> m_commands;
    bool m_lastWasNewline = true;
    QMap<QString, QByteArray> m_rxBuffers;

    void sendToSelectedPorts(const QByteArray &data);
    void processRawData(const QByteArray &data, const QString &source);
    void processBufferedData(const QString &source);

    // Chart helpers
    void setupSnrChart(QChart *&chart, QChartView *chartView);
    void setupSkyChart();
    void updateSkyChart(const QVector<GnssSatellite> &satellites);
    void applyChartToView(QChart *chart, QChartView *view);
    void updateSatelliteData(const QVector<GnssSatellite> &satellites);
    void updateChartWithSatellites(const QVector<GnssSatellite> &satellites, QChart *chart, QChartView *chartView,
                                   QStringList signalType);
    void recolorSnrBars(QChartView *chartView, QChart *chart);

    void setupConnections();
    void loadPorts();
    void loadCommandButtons();
    void saveCommandButtons();
    void refreshCommandButtons();
    void loadSettings();
    void saveSettings();

    // NTRIP helpers
    void sendNtripPosition();
    QString formatGPGGA(double lat, double lon, double alt);

    QStringList m_satelliteCategories;
    GnssInfo m_lastGnssInfo;

protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif  // MAINWINDOW_H
