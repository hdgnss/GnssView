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
#include "mainwindow.h"

#include <QtCore/qdebug.h>

#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QGraphicsRectItem>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>

#include "communicationmanager.h"
#include "core/updatechecker.h"
#include "datalogger.h"
#include "nmeaparser.h"
#include "ui/cmdbuttondialog.h"
#include "ui/settingsdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_comms(new CommunicationManager(this)),
      m_parser(new NmeaParser()),
      m_serialALogger(new DataLogger(this)),
      m_serialBLogger(new DataLogger(this)),
      m_tcpLogger(new DataLogger(this)),
      m_udpLogger(new DataLogger(this)),
      m_ntripLogger(new DataLogger(this)) {
    ui->setupUi(this);

    // Set Layout Stretch (4x3)
    ui->gridLayout->setRowStretch(0, 1);
    ui->gridLayout->setRowStretch(1, 1);
    ui->gridLayout->setRowStretch(2, 1);
    ui->gridLayout->setColumnStretch(0, 1);
    ui->gridLayout->setColumnStretch(1, 1);
    ui->gridLayout->setColumnStretch(2, 1);
    ui->gridLayout->setColumnStretch(3, 1);

    loadPorts();
    setupConnections();
    loadCommandButtons();

    // Initialize Charts
    setupSnrChart(m_snrL1Chart, ui->chartSnrL1);
    setupSnrChart(m_snrL5Chart, ui->chartSnrL5);
    setupSkyChart();

    // Connect column change
    connect(ui->spinCmdCols, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        saveCommandButtons();
        refreshCommandButtons();
    });

    loadSettings();

    ui->labelStatusSerialA->setStyleSheet("color: black; font-size: 18pt;");
    ui->labelStatusSerialB->setStyleSheet("color: black; font-size: 18pt;");
    ui->labelStatusTcp->setStyleSheet("color: black; font-size: 18pt;");
    ui->labelStatusUdp->setStyleSheet("color: black; font-size: 18pt;");
    ui->labelStatusNtrip->setStyleSheet("color: black; font-size: 18pt;");

    m_updateChecker = new UpdateChecker(this);
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, &MainWindow::handleUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable, this, &MainWindow::handleNoUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::checkFailed, this, &MainWindow::handleUpdateCheckFailed);

    // NTRIP Client
    m_ntripClient = new NTRIPClient(this);
    connect(m_ntripClient, &NTRIPClient::dataReceived, this, &MainWindow::handleNtripDataReceived);
    connect(m_ntripClient, &NTRIPClient::connected, this, [this]() {
        ui->buttonOpenNtrip->setText("Disconnect");
        ui->labelStatusNtrip->setStyleSheet("color: green; font-size: 18pt;");
        m_ntripLogger->startLogging("Ntrip");
        sendNtripPosition();
    });
    connect(m_ntripClient, &NTRIPClient::disconnected, this, [this]() {
        ui->buttonOpenNtrip->setText("Connect");
        ui->labelStatusNtrip->setStyleSheet("color: black; font-size: 18pt;");
        m_ntripLogger->stopLogging();
    });
    connect(m_ntripClient, &NTRIPClient::connectionError, this, [this](const QString &error) {
        ui->buttonOpenNtrip->setText("Connect");
        ui->labelStatusNtrip->setStyleSheet("color: red; font-size: 18pt;");
        ui->statusbar->showMessage("NTRIP Error: " + error, 5000);
    });

    // Check for updates on startup (silent mode)
    // Check for updates on startup (silent mode)
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");
    bool checkUpdate = settings.value("checkUpdate", true).toBool();
    if (checkUpdate) {
        m_updateChecker->checkForUpdates(true);
    }

    // Setup Menu
    ui->actionSettings->setMenuRole(QAction::NoRole);
    ui->actionExit->setMenuRole(QAction::NoRole);
    ui->menuFile_2->addAction(ui->actionSettings);
    ui->menuFile_2->addAction(ui->actionExit);
}

MainWindow::~MainWindow() {
    // Prevent signals from reaching us during destruction
    if (m_comms) {
        m_comms->disconnect(this);
    }
    if (m_ntripClient) {
        m_ntripClient->disconnectFromServer();
    }
    delete ui;
}

void MainWindow::setupConnections() {
    // Serial A
    connect(m_comms, &CommunicationManager::serialADataReceived, this, [this](const QByteArray &data) {
        m_serialALogger->writeData(data);
        processRawData(data, "SEA");
    });

    connect(m_comms, &CommunicationManager::serialAStatusChanged, this, [this](bool connected, const QString &msg) {
        ui->buttonOpenSerialA->setText(connected ? "Close" : "Open");
        ui->labelStatusSerialA->setStyleSheet(connected ? "color: green; font-size: 18pt;"
                                                        : "color: black; font-size: 18pt;");
        if (connected) {
            m_serialALogger->startLogging("SerialA");
        } else {
            m_serialALogger->stopLogging();
        }
    });

    // Serial B
    connect(m_comms, &CommunicationManager::serialBDataReceived, this, [this](const QByteArray &data) {
        m_serialBLogger->writeData(data);
        processRawData(data, "SEB");
    });

    connect(m_comms, &CommunicationManager::serialBStatusChanged, this, [this](bool connected, const QString &msg) {
        ui->buttonOpenSerialB->setText(connected ? "Close" : "Open");
        ui->labelStatusSerialB->setStyleSheet(connected ? "color: green; font-size: 18pt;"
                                                        : "color: black; font-size: 18pt;");
        if (connected) {
            m_serialBLogger->startLogging("SerialB");
        } else {
            m_serialBLogger->stopLogging();
        }
    });

    // TCP
    connect(m_comms, &CommunicationManager::tcpDataReceived, this, [this](const QByteArray &data) {
        m_tcpLogger->writeData(data);
        processRawData(data, "TCP");
    });

    connect(m_comms, &CommunicationManager::tcpStatusChanged, this, [this](bool connected, const QString &msg) {
        ui->buttonTcpConnect->setText(connected ? "Disconnect" : "Connect");
        ui->labelStatusTcp->setStyleSheet(connected ? "color: green; font-size: 18pt;"
                                                    : "color: black; font-size: 18pt;");
        if (connected) {
            m_tcpLogger->startLogging("tcp");
        } else {
            m_tcpLogger->stopLogging();
        }
    });

    // UDP
    connect(m_comms, &CommunicationManager::udpDataReceived, this, [this](const QByteArray &data) {
        m_udpLogger->writeData(data);
        processRawData(data, "UDP");
    });

    connect(m_comms, &CommunicationManager::udpStatusChanged, this, [this](bool connected, const QString &msg) {
        ui->buttonUdpBind->setText(connected ? "Unbind" : "Bind");
        ui->labelStatusUdp->setStyleSheet(connected ? "color: green; font-size: 18pt;"
                                                    : "color: black; font-size: 18pt;");
        if (connected) {
            m_udpLogger->startLogging("udp");
        } else {
            m_udpLogger->stopLogging();
        }
    });

    connect(m_comms, &CommunicationManager::errorOccurred, this,
            [this](const QString &err) { ui->statusbar->showMessage("Error: " + err, 5000); });

    // NMEA parser connection
    connect(m_parser, &NmeaParser::newNmeaParseDone, this, &MainWindow::handleNmeaParseDone);

    // Manual UI Connections (replacing auto-connect)
    connect(ui->buttonOpenSerialA, &QPushButton::clicked, this, &MainWindow::handleButtonOpenSerialAClicked);
    connect(ui->buttonOpenSerialB, &QPushButton::clicked, this, &MainWindow::handleButtonOpenSerialBClicked);
    connect(ui->buttonTcpConnect, &QPushButton::clicked, this, &MainWindow::handleButtonTcpConnectClicked);
    connect(ui->buttonUdpBind, &QPushButton::clicked, this, &MainWindow::handleButtonUdpBindClicked);
    // NTRIP
    connect(ui->buttonOpenNtrip, &QPushButton::clicked, this, &MainWindow::handleButtonOpenNtripClicked);

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::handleActionSettingsTriggered);
    connect(ui->buttonAddCommand, &QPushButton::clicked, this, &MainWindow::handleButtonAddCommandClicked);
    connect(ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::handleActionCheckUpdatesTriggered);
}

void MainWindow::loadPorts() {
    ui->comboBoxSerialA->clear();
    ui->comboBoxSerialB->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ui->comboBoxSerialA->addItem(info.portName());
        ui->comboBoxSerialB->addItem(info.portName());
    }
}

void MainWindow::handleButtonOpenSerialAClicked() {
    if (m_comms->isSerialConnected(0)) {
        m_comms->disconnectSerial(0);
    } else {
        int baud = ui->comboBoxBaudRateA->currentText().toInt();
        if (baud <= 0) {
            baud = 115200;  // Default safety
        }
        m_comms->connectSerial(ui->comboBoxSerialA->currentText(), baud, 0);
    }
}

void MainWindow::handleButtonOpenSerialBClicked() {
    if (m_comms->isSerialConnected(1)) {
        m_comms->disconnectSerial(1);
    } else {
        int baud = ui->comboBoxBaudRateB->currentText().toInt();
        if (baud <= 0) {
            baud = 115200;
        }
        m_comms->connectSerial(ui->comboBoxSerialB->currentText(), baud, 1);
    }
}

void MainWindow::handleButtonTcpConnectClicked() {
    if (m_comms->isTcpConnected()) {
        m_comms->disconnectTcp();
    } else {
        m_comms->connectTcp(ui->lineEditTcpAddress->text(), ui->lineEditTcpPort->text().toInt());
    }
}

void MainWindow::handleButtonUdpBindClicked() {
    if (m_comms->isUdpBound()) {
        m_comms->unbindUdp();
    } else {
        m_comms->bindUdp(ui->lineEditUdpPort->text().toInt());
    }
}

void MainWindow::handleButtonOpenNtripClicked() {
    if (m_ntripClient->isConnected()) {
        m_ntripClient->disconnectFromServer();
    } else {
        QString host = ui->lineEditHost->text();
        int port = ui->lineEditPort->text().toInt();
        QString mount = ui->lineEditMount->text();
        QString user = ui->lineEditUser->text();
        QString pass = ui->lineEditPass->text();
        m_ntripClient->connectToServer(host, port, mount, user, pass);
    }
}

void MainWindow::handleNtripDataReceived(const QByteArray &data) {
    if (data.isEmpty()) {
        return;
    }

    // Log the raw data
    m_ntripLogger->writeData(data);

    // Process data similar to other inputs
    // processRawData(data, "Ntrip");
    // Note: The user said "No need to decode the Ntrip data." but we typically
    // get NMEA/RTCM from NTRIP. If it's RTCM, NMEA parser might skip it or show garbage.
    // If it's NMEA, it will be parsed.
    // We'll treat it as raw data source "NTRIP".
}

void MainWindow::handleActionSettingsTriggered() {
    SettingsDialog dlg(this);

    // Load current setting into dialog
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");
    dlg.setAutoSavePath(settings.value("autoSavePath", QDir::homePath()).toString());
    dlg.setCheckUpdateOnStartup(settings.value("checkUpdate", true).toBool());

    if (dlg.exec() == QDialog::Accepted) {
        QString newPath = dlg.autoSavePath();
        settings.setValue("autoSavePath", newPath);
        settings.setValue("checkUpdate", dlg.checkUpdateOnStartup());

        // Apply immediately
        m_serialALogger->setLogDirectory(newPath);
        m_serialBLogger->setLogDirectory(newPath);
        m_tcpLogger->setLogDirectory(newPath);
        m_udpLogger->setLogDirectory(newPath);
    }
}

void MainWindow::refreshCommandButtons() {
    ui->tabCommands->clear();
    int cols = ui->spinCmdCols->value();

    // Group commands
    QMap<QString, QList<CommandDefinition>> groups;
    for (const auto &cmd : m_commands) {
        QString grp = cmd.group.isEmpty() ? "General" : cmd.group;
        groups[grp].append(cmd);
    }

    // Create tabs
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QWidget *page = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(page);

        // Scroll Area for the tab
        QScrollArea *scroll = new QScrollArea(page);
        scroll->setWidgetResizable(true);
        QWidget *content = new QWidget();
        QGridLayout *contentLayout = new QGridLayout(content);  // GRID
        contentLayout->setAlignment(Qt::AlignTop);

        int idx = 0;
        for (const auto &cmd : it.value()) {
            QPushButton *btn = new QPushButton(cmd.name, content);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            QString hex = cmd.hexData;
            connect(btn, &QPushButton::clicked, this, [this, hex]() {
                QByteArray data = QByteArray::fromHex(hex.toLatin1());
                sendToSelectedPorts(data);
            });
            contentLayout->addWidget(btn, idx / cols, idx % cols);
            idx++;
        }

        scroll->setWidget(content);
        layout->addWidget(scroll);
        layout->setContentsMargins(0, 0, 0, 0);

        ui->tabCommands->addTab(page, it.key());
    }
}

void MainWindow::handleButtonAddCommandClicked() {
    CmdButtonDialog dlg(m_commands, this);  // Pass current list
    if (dlg.exec() == QDialog::Accepted) {
        m_commands = dlg.commands();  // Get updated list
        saveCommandButtons();
        refreshCommandButtons();
    }
}

static const int MAX_RX_BUFFER_SIZE = 64 * 1024;  // 64 KB

void MainWindow::processRawData(const QByteArray &data, const QString &source) {
    QByteArray &buffer = m_rxBuffers[source];

    // Append new chunk into our rx "circular" buffer
    buffer.append(data);

    // Optional: cap the size so the buffer behaves like a circ_buff
    if (buffer.size() > MAX_RX_BUFFER_SIZE) {
        // keep only the last MAX_RX_BUFFER_SIZE bytes
        buffer = buffer.right(MAX_RX_BUFFER_SIZE);
    }

    while (true) {
        // 1. Look for start of NMEA sentence: '$' or '!'
        int startIdx = buffer.indexOf('$');
        int startIdxAlt = buffer.indexOf('!');  // some NMEA-like sentences use '!'
        if (startIdx == -1 || (startIdxAlt != -1 && startIdxAlt < startIdx)) {
            startIdx = startIdxAlt;  // pick earliest of '$'/'!'
        }

        if (startIdx == -1) {
            // No start found at all -> drop all data (pure binary/noise)
            // or keep last few bytes if you want to be conservative.
            if (buffer.size() > 256) {
                buffer.clear();
            }
            break;
        }

        // Drop everything before the start of the next possible NMEA
        if (startIdx > 0) {
            buffer.remove(0, startIdx);
        }

        // 2. Look for end-of-line (LF). NMEA typically ends with "\r\n"
        int endIdx = buffer.indexOf('\n', 1);  // search after the '$'/'!'
        if (endIdx == -1) {
            // We have a start but not the full line yet -> wait for more data
            break;
        }

        // Extract one candidate sentence [0..endIdx]
        QByteArray sentence = buffer.mid(0, endIdx + 1);
        buffer.remove(0, endIdx + 1);

        // 3. Strip CR/LF
        while (!sentence.isEmpty() && (sentence.endsWith('\r') || sentence.endsWith('\n'))) {
            sentence.chop(1);
        }

        // 4. Basic sanity checks for NMEA (optional but useful with mixed binary)
        if (sentence.size() < 6) {
            // Too short to be valid NMEA, skip
            continue;
        }

        // Optional: reject obviously non-ASCII garbage
        bool asciiOk = true;
        for (char c : sentence) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (uc < 0x20 || uc > 0x7E) {  // printable ASCII (space..~)
                // Allow comma, etc. (already in 0x20–0x7E). Control chars were
                // stripped.
                asciiOk = false;
                break;
            }
        }
        if (!asciiOk) {
            continue;
        }

        // Optional: verify NMEA checksum if present
        int starPos = sentence.lastIndexOf('*');
        if (starPos > 0 && starPos + 2 < sentence.size()) {
            // Calculate checksum of characters between '$'/'!' and '*'
            quint8 chk = 0;
            for (int i = 1; i < starPos; ++i) {
                chk ^= static_cast<quint8>(sentence.at(i));
            }

            bool ok1 = false, ok2 = false;
            int hi = QByteArray(1, sentence.at(starPos + 1)).toInt(&ok1, 16);
            int lo = QByteArray(1, sentence.at(starPos + 2)).toInt(&ok2, 16);

            if (ok1 && ok2) {
                quint8 sentChk = static_cast<quint8>((hi << 4) | lo);
                if (sentChk != chk) {
                    // Invalid checksum -> likely garbage; skip this line
                    continue;
                }
            }
        }

        m_parser->parseNmeaSentence(sentence, false);

        QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        QTextCursor cursor = ui->plainTextEditRaw->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->plainTextEditRaw->setTextCursor(cursor);
        ui->plainTextEditRaw->insertPlainText(QString("[%1][%2] ").arg(timeStr, source));
        ui->plainTextEditRaw->insertPlainText(sentence);
        ui->plainTextEditRaw->insertPlainText("\n");
        ui->plainTextEditRaw->ensureCursorVisible();
    }
}

void MainWindow::sendToSelectedPorts(const QByteArray &data) {
    // Check UI checkboxes
    // Mapped: 3->Serial A, 4->Serial B, 6->TCP, 5->UDP
    if (ui->checkBoxSerialA->isChecked()) {  // Serial A
        m_comms->sendSerial(data, 0);
    }

    if (ui->checkBoxSerialB->isChecked()) {  // Serial B
        m_comms->sendSerial(data, 1);
    }

    if (ui->checkBoxTcp->isChecked()) {
        m_comms->sendTcp(data);
    }
    if (ui->checkBoxUdp->isChecked()) {
        m_comms->sendUdp(data, ui->lineEditTcpAddress->text(), ui->lineEditTcpPort->text().toInt());
    }
}

void MainWindow::loadCommandButtons() {
    m_commands.clear();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");

    // Load Columns
    int cols = settings.value("columns", 1).toInt();
    if (cols < 1) {
        cols = 1;
    }
    ui->spinCmdCols->blockSignals(true);  // Prevent trigger during load
    ui->spinCmdCols->setValue(cols);
    ui->spinCmdCols->blockSignals(false);

    int size = settings.beginReadArray("buttons");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        CommandDefinition cmd;
        cmd.name = settings.value("name").toString();
        cmd.hexData = settings.value("hex").toString();
        cmd.group = settings.value("group").toString();
        m_commands.append(cmd);
    }
    settings.endArray();

    if (m_commands.isEmpty()) {
        CommandDefinition def;
        def.name = "Test CMD 1";
        def.hexData = "FF 00 FF";
        m_commands.append(def);
    }

    refreshCommandButtons();
}

void MainWindow::saveCommandButtons() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");
    settings.setValue("columns", ui->spinCmdCols->value());

    settings.beginWriteArray("buttons");
    for (int i = 0; i < m_commands.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_commands[i].name);
        settings.setValue("hex", m_commands[i].hexData);
        settings.setValue("group", m_commands[i].group);
    }
    settings.endArray();
}

void MainWindow::loadSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");

    // Version Check
    QString storedVersion = settings.value("version").toString();
    QString currentVersion = UpdateChecker::currentVersion();

    if (storedVersion.isEmpty()) {
        settings.clear();
        settings.setValue("version", currentVersion);
    }

    restoreGeometry(settings.value("geometry").toByteArray());

    // Serial A
    ui->comboBoxSerialA->setCurrentText(settings.value("SerialA/port").toString());
    ui->comboBoxBaudRateA->setCurrentText(settings.value("SerialA/baud").toString());
    ui->comboBoxDataBitsA->setCurrentText(settings.value("SerialA/data").toString());
    ui->comboBoxParityA->setCurrentText(settings.value("SerialA/parity").toString());
    ui->comboBoxStopBitsA->setCurrentText(settings.value("SerialA/stop").toString());

    // Serial B
    ui->comboBoxSerialB->setCurrentText(settings.value("SerialB/port").toString());
    ui->comboBoxBaudRateB->setCurrentText(settings.value("SerialB/baud").toString());
    ui->comboBoxDataBitsB->setCurrentText(settings.value("SerialB/data").toString());
    ui->comboBoxParityB->setCurrentText(settings.value("SerialB/parity").toString());
    ui->comboBoxStopBitsB->setCurrentText(settings.value("SerialB/stop").toString());

    // TCP
    ui->lineEditTcpAddress->setText(settings.value("Tcp/ip", "127.0.0.1").toString());
    ui->lineEditTcpPort->setText(settings.value("Tcp/port", "8080").toString());

    // UDP
    ui->lineEditUdpPort->setText(settings.value("Udp/port", "18520").toString());

    // NTRIP
    ui->lineEditHost->setText(settings.value("Ntrip/host", "ntrip.geodetic.gov.hk").toString());
    ui->lineEditPort->setText(settings.value("Ntrip/port", "2101").toString());
    ui->lineEditMount->setText(settings.value("Ntrip/mount", "HKTK_32").toString());
    ui->lineEditUser->setText(settings.value("Ntrip/user", "").toString());
    ui->lineEditPass->setText(settings.value("Ntrip/pass", "").toString());
    ui->lineEditLocation->setText(settings.value("Ntrip/location", "31,121").toString());

    QString autoSavePath = settings.value("autoSavePath", QDir::homePath()).toString();
    m_serialALogger->setLogDirectory(autoSavePath);
    m_serialBLogger->setLogDirectory(autoSavePath);
    m_tcpLogger->setLogDirectory(autoSavePath);
    m_udpLogger->setLogDirectory(autoSavePath);
    m_ntripLogger->setLogDirectory(autoSavePath);

    // Restore tab indices
    ui->tabComms->setCurrentIndex(settings.value("Tabs/commsIndex", 0).toInt());
    ui->tabInfoData->setCurrentIndex(settings.value("Tabs/infoDataIndex", 0).toInt());
    ui->tabCommands->setCurrentIndex(settings.value("Tabs/commandsIndex", 0).toInt());
}

void MainWindow::saveSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HDGNSS", "GnssView");
    settings.setValue("geometry", saveGeometry());

    settings.setValue("SerialA/port", ui->comboBoxSerialA->currentText());
    settings.setValue("SerialA/baud", ui->comboBoxBaudRateA->currentText());
    settings.setValue("SerialA/data", ui->comboBoxDataBitsA->currentText());
    settings.setValue("SerialA/parity", ui->comboBoxParityA->currentText());
    settings.setValue("SerialA/stop", ui->comboBoxStopBitsA->currentText());

    settings.setValue("SerialB/port", ui->comboBoxSerialB->currentText());
    settings.setValue("SerialB/baud", ui->comboBoxBaudRateB->currentText());
    settings.setValue("SerialB/data", ui->comboBoxDataBitsB->currentText());
    settings.setValue("SerialB/parity", ui->comboBoxParityB->currentText());
    settings.setValue("SerialB/stop", ui->comboBoxStopBitsB->currentText());

    settings.setValue("Tcp/ip", ui->lineEditTcpAddress->text());
    settings.setValue("Tcp/port", ui->lineEditTcpPort->text());

    settings.setValue("Udp/port", ui->lineEditUdpPort->text());

    settings.setValue("Ntrip/host", ui->lineEditHost->text());
    settings.setValue("Ntrip/port", ui->lineEditPort->text());
    settings.setValue("Ntrip/mount", ui->lineEditMount->text());
    settings.setValue("Ntrip/user", ui->lineEditUser->text());
    settings.setValue("Ntrip/pass", ui->lineEditPass->text());
    settings.setValue("Ntrip/location", ui->lineEditLocation->text());

    // Save tab indices
    settings.setValue("Tabs/commsIndex", ui->tabComms->currentIndex());
    settings.setValue("Tabs/infoDataIndex", ui->tabInfoData->currentIndex());
    settings.setValue("Tabs/commandsIndex", ui->tabCommands->currentIndex());
}

void MainWindow::sendNtripPosition() {
    QString manualLoc = ui->lineEditLocation->text().trimmed();

    QString gpgga;

    if (manualLoc.isEmpty()) {
        qDebug() << "NTRIP location empty, not sending position.";
        return;
    }

    // Attempt to parse "lat,lon,alt"
    QStringList parts = manualLoc.split(',');
    bool manualValid = false;
    double lat = 0.0, lon = 0.0, alt = 0.0;

    if (parts.size() >= 2) {  // Allow "lat,lon" or "lat,lon,alt"
        bool ok1, ok2;
        lat = parts[0].toDouble(&ok1);
        lon = parts[1].toDouble(&ok2);
        if (parts.size() > 2) {
            alt = parts[2].toDouble();
        }

        if (ok1 && ok2) {
            manualValid = true;
            gpgga = formatGPGGA(lat, lon, alt);
        }
    }

    if (!manualValid) {
        // If manual input is present but invalid/not numbers, OR if user intends
        // to use NMEA fallback (though user req says "if not valid... Get lla from NMEA")
        // We will assume if it fails parsing as numbers but isn't empty, we check NMEA.
        // Actually, user said: "if lineEditLocation is not valid 'lat,lon,alt', Get lla from NMEA"

        // Use last known NMEA position if available
        // Simple check: do we have a non-zero lat/lon? or just trust m_lastGnssInfo
        if (m_lastGnssInfo.position.latitude != 0.0 || m_lastGnssInfo.position.longitude != 0.0) {
            gpgga = formatGPGGA(m_lastGnssInfo.position.latitude, m_lastGnssInfo.position.longitude,
                                m_lastGnssInfo.position.altitude);
        } else {
            qDebug() << "NTRIP: Invalid manual location and no valid NMEA position yet.";
            return;
        }
    }

    if (!gpgga.isEmpty()) {
        m_ntripClient->sendGnssPosition(gpgga);
    }
}

QString MainWindow::formatGPGGA(double lat, double lon, double alt) {
    // Convert decimal deg to NMEA DDMM.MMMM
    auto toNmea = [](double deg) {
        double d = std::floor(std::abs(deg));
        double m = (std::abs(deg) - d) * 60.0;
        return d * 100.0 + m;
    };

    double latNmea = toNmea(lat);
    double lonNmea = toNmea(lon);

    char ns = (lat >= 0) ? 'N' : 'S';
    char ew = (lon >= 0) ? 'E' : 'W';

    // Construct essential parts of GPGGA
    // Let's use current UTC time for timestamp.
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString timeStr = now.toString("hhmmss.00");

    QString sentence = QString("GPGGA,%1,%2,%3,%4,%5,1,10,1.0,%6,M,0.0,M,,")
                           .arg(timeStr)
                           .arg(latNmea, 0, 'f', 5)
                           .rightJustified(9, '0')  // 1234.56789
                           .arg(ns)
                           .arg(lonNmea, 0, 'f', 5)
                           .rightJustified(10, '0')  // 12345.67890
                           .arg(ew)
                           .arg(alt, 0, 'f', 2);

    // Calculate checksum
    int sum = 0;
    for (QChar c : sentence) {
        sum ^= c.toLatin1();
    }

    return QString("$%1*%2\r\n").arg(sentence).arg(sum, 2, 16, QChar('0')).toUpper();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

// Unified setup method for both L1 and L5 charts
void MainWindow::setupSnrChart(QChart *&chart, QChartView *chartView) {
    // Create a chart
    chart = new QChart();
    chart->setTitle("");
    chart->setAnimationOptions(QChart::NoAnimation);

    chart->setBackgroundBrush(QBrush(QColor("#f5f6fa")));
    // Create empty bar set for SNR values
    QBarSet *barSet = new QBarSet("");

    // Create empty bar series and add the bar set
    QBarSeries *series = new QBarSeries();
    series->append(barSet);
    chart->addSeries(series);

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
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Attach series to axes
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // Hide the legend
    chart->legend()->setVisible(false);

    // Apply chart to view
    applyChartToView(chart, chartView);
}

// Helper method to apply common setup to a chart view
void MainWindow::applyChartToView(QChart *chart, QChartView *view) {
    view->setChart(chart);
    view->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::updateSatelliteData(const QVector<GnssSatellite> &satellites) {
    if (!m_snrL1Chart || !m_snrL5Chart) {
        return;
    }

    // Update both charts with the satellite data
    updateChartWithSatellites(satellites, m_snrL1Chart, ui->chartSnrL1, {"L1"});
    updateChartWithSatellites(satellites, m_snrL5Chart, ui->chartSnrL5, {"L5", "L2", "L6"});
    updateSkyChart(satellites);
}

void MainWindow::updateChartWithSatellites(const QVector<GnssSatellite> &satellites, QChart *chart,
                                           QChartView *chartView, QStringList signalType) {
    // Create a new bar set
    QBarSet *barSet = new QBarSet("");

    // Clear previous categories
    m_satelliteCategories.clear();

    // Map to track which satellites we've already added
    QMap<QString, int> addedSatellites;

    // Process each satellite
    for (const GnssSatellite &sat : satellites) {
        // Compare signal type directly with the mapped signal
        QString signalKey = QString("%1%2").arg(sat.system).arg(sat.signal);
        if (signalType.contains(SignalMap[signalKey])) {
            QString prefix = SystemMap[sat.system];

            // Create the satellite identifier
            QString satId = QString("%1%2").arg(prefix).arg(sat.prn, 2, 10, QChar('0'));

            // If we've already added this satellite, skip it
            if (addedSatellites.contains(satId)) {
                continue;
            }

            // Add the satellite to our tracking map
            addedSatellites[satId] = sat.snr;

            // Add the satellite to our categories and add its SNR to the bar set
            m_satelliteCategories << satId;
            *barSet << sat.snr;
        }
    }

    // Remove old series
    chart->removeAllSeries();

    // Create new series with our updated data
    QBarSeries *series = new QBarSeries();
    series->append(barSet);

    // Enable labels for the bar set
    barSet->setLabelFont(QFont("Arial", 10));
    barSet->setLabelColor(Qt::black);

    // Enable labels on the series and position them
    series->setLabelsVisible(true);
    series->setLabelsPosition(QBarSeries::LabelsPosition::LabelsInsideEnd);

    chart->addSeries(series);

    // Remove old axes
    QList<QAbstractAxis *> axes = chart->axes();
    for (QAbstractAxis *axis : axes) {
        chart->removeAxis(axis);
        delete axis;
    }

    // Create and set up X axis
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(m_satelliteCategories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Rotate labels by 90 degrees
    axisX->setLabelsAngle(90);

    QFont axisFont = axisX->labelsFont();
    axisFont.setPointSize(7);
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
void MainWindow::recolorSnrBars(QChartView *chartView, QChart *chart) {
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
    QColor gpsColor(0, 120, 215);        // Blue for GPS (G)
    QColor glonassColor(232, 17, 35);    // Red for GLONASS (R)
    QColor galileoColor(255, 140, 0);    // Orange for Galileo (E)
    QColor beidouColor(0, 153, 0);       // Green for BeiDou (B)
    QColor qzssColor(138, 43, 226);      // Purple for QZSS (J)
    QColor irnssColor(255, 215, 0);      // Gold for IRNSS/NavIC (I)
    QColor sbasColor(169, 169, 169);     // Gray for SBAS (S)
    QColor unknownColor(100, 100, 100);  // Dark gray for unknown

    // Create a map of actual indices to satellite prefixes
    QMap<int, QColor> rectColorMap;
    for (int i = 0; i < m_satelliteCategories.size(); ++i) {
        QString category = m_satelliteCategories[i];
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
        int reverseIndex = m_satelliteCategories.size() - 1 - i;
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

void MainWindow::handleNmeaParseDone(GnssInfo gnssInfo) {
    // Update position data
    const GnssPosition &pos = gnssInfo.position;
    // Convert UTC timestamp to time string
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(pos.utc);
    ui->labelTime->setText(dt.toString("hh:mm:ss.zzz"));
    ui->labelLatitude->setText(QString::number(pos.latitude, 'f', 6));
    ui->labelLongitude->setText(QString::number(pos.longitude, 'f', 6));
    ui->labelQuality->setText(QString::number(pos.quality));
    ui->labelSatsInUse->setText(QString::number(pos.satellites));
    ui->labelSatsInView->setText(QString::number(gnssInfo.satellites.size()));
    ui->labelAltitude->setText(QString::number(pos.altitude, 'f', 2) + " m");
    ui->labelHdop->setText(QString::number(pos.hdop, 'f', 2));
    ui->labelSpeed->setText(QString::number(pos.speed, 'f', 2) + " kn");
    ui->labelTrack->setText(QString::number(pos.course, 'f', 2));
    ui->labelDate->setText(dt.toString("yyyy-MM-dd"));
    ui->labelStatus->setText(pos.status);
    ui->labelMode->setText(pos.mode);
    ui->labelMagnetic->setText(QString::number(pos.magnetic, 'f', 2));

    // Update satellite charts
    updateSatelliteData(gnssInfo.satellites);
}

void MainWindow::handleActionCheckUpdatesTriggered() {
    ui->statusbar->showMessage("Checking for updates...", 3000);
    m_updateChecker->checkForUpdates(false);
}

void MainWindow::handleUpdateAvailable(const QString &version, const QString &releaseUrl, const QString &releaseNotes) {
    QString message = QString(
                          "A new version of GnssView is available!\n\n"
                          "Current version: %1\n"
                          "New version: %2\n\n"
                          "Would you like to download the update?")
                          .arg(UpdateChecker::currentVersion())
                          .arg(version);

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Update Available");
    msgBox.setText(message);
    msgBox.setDetailedText(releaseNotes);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(releaseUrl));
    }
}

void MainWindow::handleNoUpdateAvailable() {
    QMessageBox::information(this, "No Updates Available",
                             QString("You are running the latest version (%1).").arg(UpdateChecker::currentVersion()));
}

void MainWindow::handleUpdateCheckFailed(const QString &error) {
    ui->statusbar->showMessage("Update check failed: " + error, 5000);
}

void MainWindow::setupSkyChart() {
    // Create a polar chart for the sky view
    m_skyChart = new QPolarChart();
    m_skyChart->setTitle("");
    m_skyChart->setAnimationOptions(QChart::NoAnimation);

    // Create angle (azimuth) axis - 0 is North, 90 is East
    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setRange(0, 360);
    angularAxis->setLabelFormat("%.0f");
    angularAxis->setTickCount(9);  // N, NE, E, SE, S, SW, W, NW, N
    angularAxis->setLabelsVisible(true);

    // Create radial (elevation) axis - 0 at edge (0° elevation), 90 at center
    // (90° elevation)
    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setRange(0, 90);
    radialAxis->setTickCount(4);          // 0°, 30°, 60°, 90° elevation
    radialAxis->setLabelsVisible(false);  // Explicitly make labels visible

    // Hide the legend
    m_skyChart->legend()->setVisible(false);

    // Set up the chart - use the proper orientation for polar chart axes
    m_skyChart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);
    m_skyChart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);

    // Apply chart to view using the helper method
    applyChartToView(m_skyChart, ui->chartSky);

    // Set background to a light color
    m_skyChart->setBackgroundBrush(QBrush(QColor("#f5f6fa")));

    // Initialize with empty satellites
    QVector<GnssSatellite> emptySats;
    updateSkyChart(emptySats);
}

void MainWindow::updateSkyChart(const QVector<GnssSatellite> &satellites) {
    if (!m_skyChart) {
        return;
    }

    // Get the axes
    QValueAxis *angularAxis = nullptr;
    QValueAxis *radialAxis = nullptr;

    // Find the axes by type - first is angular, second is radial in a polar chart
    const auto axes = m_skyChart->axes();
    if (axes.size() >= 2) {
        angularAxis = qobject_cast<QValueAxis *>(axes.at(0));
        radialAxis = qobject_cast<QValueAxis *>(axes.at(1));
    }

    if (!angularAxis || !radialAxis) {
        return;  // Can't continue without both axes
    }

    // Remove all existing series
    m_skyChart->removeAllSeries();

    // Define colors for each constellation
    QColor gpsColor(0, 120, 215);        // Blue for GPS (G)
    QColor glonassColor(232, 17, 35);    // Red for GLONASS (R)
    QColor galileoColor(255, 140, 0);    // Orange for Galileo (E)
    QColor beidouColor(0, 153, 0);       // Green for BeiDou (B)
    QColor qzssColor(138, 43, 226);      // Purple for QZSS (J)
    QColor irnssColor(255, 215, 0);      // Gold for IRNSS/NavIC (I)
    QColor sbasColor(169, 169, 169);     // Gray for SBAS (S)
    QColor unknownColor(100, 100, 100);  // Dark gray for unknown

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
    for (const GnssSatellite &sat : satellites) {
        // Skip satellites with elevation <= 0
        if (sat.elevation <= 0) {
            continue;
        }

        // Convert to polar coordinates
        // Azimuth is the angular coordinate (0-360°)
        // Elevation is the radial coordinate (0-90°)
        qreal azimuth = sat.azimuth;
        qreal elevation = 90.0 - sat.elevation;  // Inverse mapping: 90° at center, 0° at edge

        // Create a data point
        QPointF point(azimuth, elevation);

        // Add the point to the appropriate series based on system
        switch (sat.system) {
            case 1:  // GPS
                gpsSeries->append(point);
                break;
            case 2:  // GLONASS
                glonassSeries->append(point);
                break;
            case 3:  // Galileo
                galileoSeries->append(point);
                break;
            case 4:  // BeiDou
                beidouSeries->append(point);
                break;
            case 5:  // QZSS
                qzssSeries->append(point);
                break;
            case 6:  // IRNSS/NavIC
                irnssSeries->append(point);
                break;
            case 7:  // SBAS
                sbasSeries->append(point);
                break;
            default:  // Unknown
                unknownSeries->append(point);
                break;
        }
    }

    // Helper function to add a series with proper axis attachment
    auto addSeriesToChart = [this, angularAxis, radialAxis](QScatterSeries *series) {
        if (!series->points().isEmpty()) {
            m_skyChart->addSeries(series);
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