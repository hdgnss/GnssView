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
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QString>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Load Intel One Mono font from resources
    const int fontId = QFontDatabase::addApplicationFont(":/fonts/fonts/ttf/IntelOneMono-Regular.ttf");
    if (fontId != -1) {
        const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QFont monoFont(fontFamilies.at(0));
            monoFont.setPointSize(11);
            app.setFont(monoFont);
        }
    }

    QFile styleFile(":/qss/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        const QString style(styleFile.readAll());
        app.setStyleSheet(style);
    }

    MainWindow window;
    window.show();
    return app.exec();
}
