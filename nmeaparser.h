/*
 * Copyright (C) 2024 HDGNSS
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
 #include <QObject>
 #include "gnsstype.h"
 
 #include <QHash>
 #include <QString>
 #include <QStringList>

// Error codes for NMEA parsing
enum NmeaParseError {
    NMEA_PARSE_OK = 0,
    NMEA_PARSE_INVALID_FORMAT = -1,
    NMEA_PARSE_CHECKSUM_ERROR = -2,
    NMEA_PARSE_TOO_FEW_FIELDS = -3,
    NMEA_PARSE_UNSUPPORTED = -4
};

class NmeaParser : public QObject
{
    Q_OBJECT
public:
    NmeaParser(bool debug = false);
    int parseNMEASentence(const QString &sentence, bool ignoreChecksum = false);
    // Helper method to get error description
    QString getErrorDescription(int errorCode) const;

signals:
    void newNmeaParseDone(GnssInfo gnssinfo);

private:
    int GPGGA(QStringList *data);
    int GPGSV(QStringList *data);
    int GLGSV(QStringList *data);
    int QZGSV(QStringList *data);
    int BDGSV(QStringList *data);
    int GAGSV(QStringList *data);
    int NCGSV(QStringList *data);
    int GPGSA(QStringList *data);
    int QZGSA(QStringList *data);
    int BDGSA(QStringList *data);
    int GAGSA(QStringList *data);
    int GLGSA(QStringList *data);
    int GNGSA(QStringList *data);
    int NCGSA(QStringList *data);
    int GPRMC(QStringList *data);

    // Helper methods
    double toDegrees(QString nmea, QString dir);
    bool checkSum(const QString &sentence, bool ignore);
    int parseGSV(QStringList *data, int system, QChar signal = '1');
    int parseGSA(QStringList *data, int defaultSystem);

    QHash<QString, int (NmeaParser::*)(QStringList *)> mNmeaHashMap;

    GnssPosition mGnssPosition;
    QVector<GnssSatellite> mGnssSatellites;

    GnssInfo mGnssInfo;

    bool mHaveGPRMC = false;
    bool mDebugEnabled = false;
    const QString mTAG = "NMEA";
};

#endif // NMEAPARSER_H