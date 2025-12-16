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

#ifndef NMEAPARSER_H
#define NMEAPARSER_H
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include "gnsstype.h"

// Error codes for NMEA parsing
enum NmeaParseError {
    NMEA_PARSE_OK = 0,
    NMEA_PARSE_INVALID_FORMAT = -1,
    NMEA_PARSE_CHECKSUM_ERROR = -2,
    NMEA_PARSE_TOO_FEW_FIELDS = -3,
    NMEA_PARSE_UNSUPPORTED = -4
};

class NmeaParser : public QObject {
    Q_OBJECT
public:
    explicit NmeaParser(bool debug = false);
    int parseNmeaSentence(const QString &sentence, bool ignoreChecksum = false);
    // Helper method to get error description
    QString getErrorDescription(int errorCode) const;

signals:
    void newNmeaParseDone(GnssInfo gnssInfo);

private:
    int parseGpgga(QStringList *data);
    int parseGpgsv(QStringList *data);
    int parseGlgsv(QStringList *data);
    int parseQzgsv(QStringList *data);
    int parseBdgsv(QStringList *data);
    int parseGagsv(QStringList *data);
    int parseNcgsv(QStringList *data);
    int parseGpgsa(QStringList *data);
    int parseQzgsa(QStringList *data);
    int parseBdgsa(QStringList *data);
    int parseGagsa(QStringList *data);
    int parseGlgsa(QStringList *data);
    int parseGngsa(QStringList *data);
    int parseNcgsa(QStringList *data);
    int parseGprmc(QStringList *data);

    // Helper methods
    double toDegrees(const QString &nmea, const QString &dir);
    bool checkSum(const QString &sentence, bool ignore);
    int parseGsv(QStringList *data, int system, QChar signal = '1');
    int parseGsa(QStringList *data, int defaultSystem);

    QHash<QString, int (NmeaParser::*)(QStringList *)> m_nmeaHashMap;

    GnssPosition m_gnssPosition;
    QVector<GnssSatellite> m_gnssSatellites;

    GnssInfo m_gnssInfo;

    bool m_haveGprmc = false;
    bool m_debugEnabled = false;
    const QString m_tag = "NMEA";
};

#endif  // NMEAPARSER_H