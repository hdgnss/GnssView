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

#ifndef GNSSTYPE_H
#define GNSSTYPE_H

#include <QHash>
#include <QMap>
#include <QString>
#include <QVector>

typedef struct {
    qint64 utc;
    QString status;
    double latitude;
    double longitude;
    double altitude;
    double undulation;
    double speed;
    double course;
    double magnetic;
    int quality;
    QString mode;
    double dop;
    double hdop;
    double vdop;
    double pdop;
    int age;
    int satellites;
    QString station;
} GnssPosition;

typedef struct {
    int prn;
    int elevation;
    int azimuth;
    int snr;
    int system;
    QChar signal;
    bool used;
} GnssSatellite;

typedef struct {
    GnssPosition position;
    QVector<GnssSatellite> satellites;
} GnssInfo;

const QMap<int, QString> SystemMap = {{0, "U"}, {1, "G"}, {2, "R"}, {3, "E"}, {4, "B"},
                                      {5, "J"}, {6, "I"}, {7, "U"}, {8, "U"}, {8, "U"}};
const QMap<QString, QString> SignalMap = {
    {"10", "L1"}, {"11", "L1"}, {"12", "L1"}, {"13", "L1"}, {"14", "L2"}, {"15", "L2"}, {"16", "L2"}, {"17", "L5"},
    {"18", "L5"}, {"20", "L1"}, {"21", "L1"}, {"22", "L1"}, {"23", "L2"}, {"24", "L2"}, {"30", "L1"}, {"31", "L5"},
    {"32", "L5"}, {"33", "L5"}, {"34", "L6"}, {"35", "L6"}, {"36", "L1"}, {"37", "L1"}, {"40", "L1"}, {"41", "L1"},
    {"42", "L1"}, {"43", "L1"}, {"44", "L1"}, {"45", "L5"}, {"46", "L5"}, {"47", "L5"}, {"48", "L6"}, {"49", "L6"},
    {"4A", "L6"}, {"4B", "L5"}, {"4C", "L5"}, {"50", "L1"}, {"51", "L1"}, {"52", "L1"}, {"53", "L1"}, {"54", "L1"},
    {"55", "L2"}, {"56", "L2"}, {"57", "L5"}, {"58", "L5"}, {"59", "L6"}, {"5A", "L6"}, {"60", "L5"}, {"61", "L5"},
    {"62", "S"},  {"63", "L5"}, {"64", "S"},  {"65", "L1"}};
const QMap<QString, QString> SystemSignalMap = {
    {"10", "GPS"},     {"11", "GPSL1CA"}, {"12", "GPSL1P"},  {"13", "GPSL1M"},  {"14", "GPSL2P"},  {"15", "GPSL2CM"},
    {"16", "GPSL2CL"}, {"17", "GPSL5I"},  {"18", "GPSL5Q"},  {"20", "GLO"},     {"21", "GLOG1CA"}, {"22", "GLOG1P"},
    {"23", "GLOG2CA"}, {"24", "GLOG2P"},  {"30", "GAL"},     {"31", "GALE5A"},  {"32", "GALE5B"},  {"33", "GALE5AB"},
    {"34", "GALE6A"},  {"35", "GALE6BC"}, {"36", "GALL1A"},  {"37", "GALL1BC"}, {"40", "BDS"},     {"41", "BDSB1I"},
    {"42", "BDSB1Q"},  {"43", "BDSB1C"},  {"44", "BDSB1A"},  {"45", "BDSB2A"},  {"46", "BDSB2B"},  {"47", "BDSB2AB"},
    {"48", "BDSB3I"},  {"49", "BDSB3Q"},  {"4A", "BDSB3A"},  {"4B", "BDSB2I"},  {"4C", "BDSB2Q"},  {"50", "QZS"},
    {"51", "QZSL1CA"}, {"52", "QZSL1CD"}, {"53", "QZSL1CP"}, {"54", "QZSL1S"},  {"55", "QZSL2CM"}, {"56", "QZSL2CL"},
    {"57", "QZSL5I"},  {"58", "QZSL5Q"},  {"59", "QZSL6D"},  {"5A", "QZSL6E"},  {"60", "NIC"},     {"61", "NICL5"},
    {"62", "NICSSPS"}, {"63", "NICL5RS"}, {"64", "NICSRS"},  {"65", "NICL1"}};
const QMap<QString, int> ColorNormalMap = {
    {"red", 0xFF0000},    {"green", 0x008000}, {"blue", 0x0000FF},   {"cyan", 0x00FFFF},   {"yellow", 0xFFFF00},
    {"orange", 0xFFA500}, {"pink", 0xFFC0CB},  {"brown", 0x800000},  {"lime", 0x00FF00},   {"magenta", 0xFF00FF},
    {"navy", 0x000080},   {"olive", 0x808000}, {"purple", 0x800080}, {"silver", 0xC0C0C0}, {"teal", 0x008080},
    {"coral", 0xFF7F50},  {"gold", 0xB29600},  {"white", 0xFFFFFF}};
const QMap<QString, int> ColorDarkMap = {
    {"red", 0xB20000},    {"green", 0x005900}, {"blue", 0x0000B2},   {"cyan", 0x00B2B2},   {"yellow", 0xB2B200},
    {"orange", 0xB27300}, {"pink", 0xB2868E},  {"brown", 0x590000},  {"lime", 0x00B200},   {"magenta", 0xB200B2},
    {"navy", 0x000059},   {"olive", 0x595900}, {"purple", 0x590059}, {"silver", 0x868686}, {"teal", 0x005959},
    {"coral", 0xB25838},  {"gold", 0x7C6900},  {"white", 0xE5E5E5}};
const QStringList ColorList = {"red",     "green", "blue",  "cyan",   "yellow", "orange", "pink",  "brown", "lime",
                               "magenta", "navy",  "olive", "purple", "silver", "teal",   "coral", "gold"};
#endif  // GNSSTYPE_H