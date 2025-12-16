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
#ifndef CMDBUTTONDIALOG_H
#define CMDBUTTONDIALOG_H

#include <QDialog>
#include <QList>

#include "core/commanddefinition.h"

namespace Ui {
class CmdButtonDialog;
}

class CmdButtonDialog : public QDialog {
    Q_OBJECT

public:
    explicit CmdButtonDialog(const QList<CommandDefinition> &commands, QWidget *parent = nullptr);
    ~CmdButtonDialog();

    QList<CommandDefinition> commands() const;

private slots:
    void handleAddClicked();
    void handleRemoveClicked();
    void handleSaveClicked();
    void handleCancelClicked();

private:
    Ui::CmdButtonDialog *ui;
};

#endif  // CMDBUTTONDIALOG_H
