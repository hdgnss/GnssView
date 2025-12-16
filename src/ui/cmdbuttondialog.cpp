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
#include "cmdbuttondialog.h"

#include <QHeaderView>
#include <QTableWidgetItem>

#include "ui_cmdbuttondialog.h"

CmdButtonDialog::CmdButtonDialog(const QList<CommandDefinition> &commands, QWidget *parent)
    : QDialog(parent), ui(new Ui::CmdButtonDialog) {
    ui->setupUi(this);

    ui->tableCommands->setColumnCount(3);
    ui->tableCommands->setRowCount(commands.size());
    ui->tableCommands->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int i = 0; i < commands.size(); ++i) {
        ui->tableCommands->setItem(i, 0, new QTableWidgetItem(commands[i].group));
        ui->tableCommands->setItem(i, 1, new QTableWidgetItem(commands[i].name));
        ui->tableCommands->setItem(i, 2, new QTableWidgetItem(commands[i].hexData));
    }

    // Connect signals manually
    connect(ui->buttonAdd, &QPushButton::clicked, this, &CmdButtonDialog::handleAddClicked);
    connect(ui->buttonRemove, &QPushButton::clicked, this, &CmdButtonDialog::handleRemoveClicked);
    connect(ui->buttonSave, &QPushButton::clicked, this, &CmdButtonDialog::handleSaveClicked);
    connect(ui->buttonCancel, &QPushButton::clicked, this, &CmdButtonDialog::handleCancelClicked);
}

CmdButtonDialog::~CmdButtonDialog() {
    delete ui;
}

QList<CommandDefinition> CmdButtonDialog::commands() const {
    QList<CommandDefinition> list;
    for (int i = 0; i < ui->tableCommands->rowCount(); ++i) {
        // Indices shifted: 0=Group, 1=Name, 2=Hex
        auto groupItem = ui->tableCommands->item(i, 0);
        auto nameItem = ui->tableCommands->item(i, 1);
        auto hexItem = ui->tableCommands->item(i, 2);

        if (groupItem && nameItem && hexItem) {
            list.append({nameItem->text(), hexItem->text(), groupItem->text()});
        }
    }
    return list;
}

void CmdButtonDialog::handleAddClicked() {
    int row = ui->tableCommands->rowCount();
    ui->tableCommands->insertRow(row);
    ui->tableCommands->setItem(row, 0, new QTableWidgetItem("General"));
    ui->tableCommands->setItem(row, 1, new QTableWidgetItem("New Cmd"));
    ui->tableCommands->setItem(row, 2, new QTableWidgetItem("00"));
}

void CmdButtonDialog::handleRemoveClicked() {
    int row = ui->tableCommands->currentRow();
    if (row >= 0) {
        ui->tableCommands->removeRow(row);
    }
}

void CmdButtonDialog::handleSaveClicked() {
    accept();
}

void CmdButtonDialog::handleCancelClicked() {
    reject();
}
