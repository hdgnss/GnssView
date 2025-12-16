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
#include "settingsdialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Settings");
    resize(500, 150);

    // Layout setup
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Path selection row
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *label = new QLabel("Auto-Save Log Path:", this);
    m_pathEdit = new QLineEdit(this);
    QPushButton *browseBtn = new QPushButton("Browse...", this);

    pathLayout->addWidget(label);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(browseBtn);

    mainLayout->addLayout(pathLayout);

    // Check Update
    m_checkUpdate = new QCheckBox("Check for updates on startup", this);
    mainLayout->addWidget(m_checkUpdate);

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Log Directory", m_pathEdit->text(),
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            m_pathEdit->setText(dir);
        }
    });

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::setAutoSavePath(const QString &path) {
    m_pathEdit->setText(path);
}

QString SettingsDialog::autoSavePath() const {
    return m_pathEdit->text();
}

void SettingsDialog::setCheckUpdateOnStartup(bool check) {
    m_checkUpdate->setChecked(check);
}

bool SettingsDialog::checkUpdateOnStartup() const {
    return m_checkUpdate->isChecked();
}
