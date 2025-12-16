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

#include "nmeaparser.h"

#include <QDate>
#include <QDebug>
#include <QTimeZone>
#include <numeric>

NmeaParser::NmeaParser(bool debug) {
    m_debugEnabled = debug;

    m_nmeaHashMap.insert("$GPGGA", &NmeaParser::parseGpgga);
    m_nmeaHashMap.insert("$GNGGA", &NmeaParser::parseGpgga);
    m_nmeaHashMap.insert("$GPRMC", &NmeaParser::parseGprmc);
    m_nmeaHashMap.insert("$GNRMC", &NmeaParser::parseGprmc);
    m_nmeaHashMap.insert("$INRMC", &NmeaParser::parseGprmc);

    m_nmeaHashMap.insert("$GPGSV", &NmeaParser::parseGpgsv);
    m_nmeaHashMap.insert("$GLGSV", &NmeaParser::parseGlgsv);
    m_nmeaHashMap.insert("$QZGSV", &NmeaParser::parseQzgsv);
    m_nmeaHashMap.insert("$GQGSV", &NmeaParser::parseQzgsv);
    m_nmeaHashMap.insert("$BDGSV", &NmeaParser::parseBdgsv);
    m_nmeaHashMap.insert("$GBGSV", &NmeaParser::parseBdgsv);
    m_nmeaHashMap.insert("$GAGSV", &NmeaParser::parseGagsv);
    m_nmeaHashMap.insert("$NCGSV", &NmeaParser::parseNcgsv);
    m_nmeaHashMap.insert("$GIGSV", &NmeaParser::parseNcgsv);
    m_nmeaHashMap.insert("$GPGSA", &NmeaParser::parseGpgsa);
    m_nmeaHashMap.insert("$QZGSA", &NmeaParser::parseQzgsa);
    m_nmeaHashMap.insert("$BDGSA", &NmeaParser::parseBdgsa);
    m_nmeaHashMap.insert("$GAGSA", &NmeaParser::parseGagsa);
    m_nmeaHashMap.insert("$GNGSA", &NmeaParser::parseGngsa);
    m_nmeaHashMap.insert("$NCGSA", &NmeaParser::parseNcgsa);
}

bool NmeaParser::checkSum(const QString &nmeaSentence, bool ignore) {
    if (nmeaSentence.at(0) == '$' && nmeaSentence.at(nmeaSentence.size() - 3) == '*') {
        if (ignore) {
            return true;
        }

        bool ok;
        unsigned int checksum = nmeaSentence.mid(nmeaSentence.size() - 2, 2).toInt(&ok, 16);
        unsigned int sum = std::accumulate(nmeaSentence.begin() + 1, nmeaSentence.end() - 3, 0,
                                           [](unsigned int sum, QChar c) { return sum ^ c.toLatin1(); });
        if (sum == checksum) {
            return true;
        } else {
            if (m_debugEnabled) {
                qDebug() << "Checksum mismatch: " << nmeaSentence << "sum: " << Qt::hex << sum
                         << "checksum: " << Qt::hex << checksum;
            }
            return false;
        }
    }
    if (m_debugEnabled) {
        qDebug() << "Not NMEA: " << nmeaSentence;
    }
    return false;
}

double NmeaParser::toDegrees(const QString &nmea, const QString &dir) {
    double decimal = 0;
    if (nmea.size() > 5) {
        decimal = nmea.toDouble();
        int deg = (int)(decimal / 100.0);
        decimal = (double)deg + (decimal - (double)deg * 100) / 60.0;
    }
    if (dir == "W" || dir == "S") {
        decimal = -decimal;
    }
    return decimal;
}

int NmeaParser::parseNmeaSentence(const QString &sentence, bool ignore) {
    QStringList parts = sentence.split("*");
    if (parts.size() != 2) {
        return NMEA_PARSE_INVALID_FORMAT;
    }

    if (!checkSum(sentence, ignore)) {
        return NMEA_PARSE_CHECKSUM_ERROR;
    }

    QStringList fields = parts[0].split(",");
    if (fields.size() < 2) {
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    QString type;
    if (sentence.at(1) == 'P') {
        ;
    } else {
        type = fields[0];
        fields.removeFirst();
    }

    if (m_nmeaHashMap.contains(type)) {
        return (this->*m_nmeaHashMap[type])(&fields);
    } else {
        if (m_debugEnabled) {
            // qDebug() << "Not support: " << type;
        }
    }
    return NMEA_PARSE_UNSUPPORTED;
}

/***************************************************************************
    $GPRMC - Recommended Minimum Specific GPS/TRANSIT Data
*******************************************************************************/
int NmeaParser::parseGprmc(QStringList *data) {
    // Verify we have the minimum required fields
    if (data->size() < 9) {
        if (m_debugEnabled) {
            qDebug() << "GPRMC: Not enough fields:" << data->size();
        }
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    // Mark that we have processed an RMC sentence (preferred for position data)
    m_haveGprmc = true;

    // Check if time and date fields are not empty for proper timestamp processing
    if (!data->at(0).isEmpty() && !data->at(8).isEmpty()) {
        QString t = data->at(8) + " " + data->at(0);
        QDateTime datetime = QDateTime::fromString(t, "ddMMyy hhmmss.z").addYears(100);

        // Handle potential parsing errors
        if (!datetime.isValid()) {
            if (m_debugEnabled) {
                qDebug() << "GPRMC: Invalid datetime format:" << t;
            }
        } else {
            datetime.setTimeZone(QTimeZone::utc());
            m_gnssPosition.utc = datetime.toMSecsSinceEpoch();
        }
    } else if (m_debugEnabled) {
        qDebug() << "GPRMC: Empty time or date field";
    }

    // Check status field (A=active/valid, V=void/invalid)
    if (!data->at(1).isEmpty()) {
        m_gnssPosition.status = data->at(1);

        // If status is void/invalid, we might want to handle it specially
        if (m_gnssPosition.status == "V" && m_debugEnabled) {
            qDebug() << "GPRMC: Position data marked invalid (V)";
        }
    }

    // Parse latitude and longitude if fields are not empty
    if (!data->at(2).isEmpty() && !data->at(4).isEmpty()) {
        m_gnssPosition.latitude = toDegrees(data->at(2), data->at(3));
        m_gnssPosition.longitude = toDegrees(data->at(4), data->at(5));
    } else if (m_debugEnabled) {
        qDebug() << "GPRMC: Empty latitude/longitude fields";
    }

    // Speed over ground (in knots, might need conversion)
    if (data->size() > 6 && !data->at(6).isEmpty()) {
        m_gnssPosition.speed = data->at(6).toDouble();
        // Convert knots to km/h or m/s if needed
        m_gnssPosition.speed *= 1.852;  // Convert knots to km/h
    }

    // Course over ground (degrees true)
    if (data->size() > 7 && !data->at(7).isEmpty()) {
        m_gnssPosition.course = data->at(7).toDouble();
    }

    // Magnetic variation (degrees)
    if (data->size() > 9 && !data->at(9).isEmpty()) {
        double magVar = data->at(9).toDouble();
        // Handle E/W indicator for magnetic variation
        if (data->size() > 10 && !data->at(10).isEmpty()) {
            if (data->at(10) == "W") {
                magVar = -magVar;  // West variation is negative
            }
        }
        m_gnssPosition.magnetic = magVar;
    }

    // Mode indicator
    if (data->size() > 11 && !data->at(11).isEmpty()) {
        m_gnssPosition.mode = data->at(11);
    } else {
        // Default mode if not specified (backwards compatibility)
        m_gnssPosition.mode = "N";
    }

    // Copy data to GnssInfo struct and emit signal
    m_gnssInfo.position = GnssPosition();  // Reset to defaults safely
    m_gnssInfo.satellites.clear();

    if (m_haveGprmc) {
        m_gnssInfo.position = m_gnssPosition;
        m_gnssInfo.satellites = m_gnssSatellites;
        if (m_debugEnabled) {
            qDebug() << "Emitting newNmeaParseDone with" << m_gnssSatellites.size() << "satellites";
        }
        emit newNmeaParseDone(m_gnssInfo);
    }

    // Clear temporary storage for next update
    m_gnssPosition = GnssPosition();  // Reset to defaults safely
    m_gnssSatellites.clear();

    return NMEA_PARSE_OK;
}

/***************************************************************************
    $GPGGA - Global positioning system (GPS) fix data
******************************************************************************/
int NmeaParser::parseGpgga(QStringList *data) {
    // Verify we have the minimum required fields (at least up to quality
    // indicator)
    if (data->size() < 6) {
        if (m_debugEnabled) {
            qDebug() << "GPGGA: Not enough fields:" << data->size();
        }
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    // Only update position if we don't have RMC data (RMC is preferred for
    // position)
    if (!m_haveGprmc) {
        // Check if latitude/longitude fields are not empty
        if (!data->at(1).isEmpty() && !data->at(3).isEmpty()) {
            m_gnssPosition.latitude = toDegrees(data->at(1), data->at(2));
            m_gnssPosition.longitude = toDegrees(data->at(3), data->at(4));
        } else if (m_debugEnabled) {
            qDebug() << "GPGGA: Empty latitude/longitude fields";
        }
    }

    // Quality indicator (0-8, see NMEA spec)
    if (!data->at(5).isEmpty()) {
        m_gnssPosition.quality = data->at(5).toInt();
    }

    // Number of satellites in use
    if (data->size() > 6 && !data->at(6).isEmpty()) {
        m_gnssPosition.satellites = data->at(6).toInt();
    }

    // DOP - Horizontal Dilution of Precision
    if (data->size() > 7 && !data->at(7).isEmpty()) {
        m_gnssPosition.dop = data->at(7).toDouble();
    }

    // Altitude
    if (data->size() > 8 && !data->at(8).isEmpty()) {
        m_gnssPosition.altitude = data->at(8).toDouble();

        // Verify altitude units if available
        if (data->size() > 9 && data->at(9) != "M") {
            if (m_debugEnabled) {
                qDebug() << "GPGGA: Unexpected altitude units:" << data->at(9);
            }
        }
    }

    // Geoidal separation (undulation)
    if (data->size() > 10 && !data->at(10).isEmpty()) {
        m_gnssPosition.undulation = data->at(10).toDouble();

        // Verify undulation units if available
        if (data->size() > 11 && data->at(11) != "M") {
            if (m_debugEnabled) {
                qDebug() << "GPGGA: Unexpected undulation units:" << data->at(11);
            }
        }
    }

    // Age of differential corrections
    if (data->size() > 12 && !data->at(12).isEmpty()) {
        m_gnssPosition.age = data->at(12).toInt();
    }

    // Differential reference station ID
    if (data->size() > 13 && !data->at(13).isEmpty()) {
        m_gnssPosition.station = data->at(13);
    }

    return NMEA_PARSE_OK;
}

// Implementations of the specific GSV parsers using the unified helper
int NmeaParser::parseGpgsv(QStringList *data) {
    return parseGsv(data, 1);  // GPS = 1
}

int NmeaParser::parseGlgsv(QStringList *data) {
    return parseGsv(data, 2);  // GLONASS = 2
}

int NmeaParser::parseGagsv(QStringList *data) {
    return parseGsv(data, 3, '7');  // Galileo = 3, default signal = '7'
}

int NmeaParser::parseBdgsv(QStringList *data) {
    return parseGsv(data, 4);  // BeiDou = 4
}

int NmeaParser::parseQzgsv(QStringList *data) {
    return parseGsv(data, 5);  // QZSS = 5
}

int NmeaParser::parseNcgsv(QStringList *data) {
    return parseGsv(data, 6, '5');  // NavIC = 6, default signal = '5'
}

// Helper method to reduce duplication in GSV parsing
int NmeaParser::parseGsv(QStringList *data, int system, QChar signal) {
    int size = data->size();
    bool hasSignalField = (size % 2 == 0);

    // Calculate appropriate loop end to avoid going out of bounds
    int loopEnd = hasSignalField ? size - 2 : size - 1;

    for (int i = 3; i < loopEnd; i += 4) {
        // Check if we have valid data (non-empty PRN and SNR)
        if (!data->at(i).isEmpty() && !data->at(i + 3).isEmpty()) {
            GnssSatellite sat;
            sat.prn = data->at(i).toInt();
            sat.elevation = data->at(i + 1).toInt();
            sat.azimuth = data->at(i + 2).toInt();
            sat.snr = data->at(i + 3).toInt();
            sat.system = system;

            // If we have a signal field, use it; otherwise use the default
            sat.signal = hasSignalField ? data->at(size - 1)[0] : signal;
            sat.used = false;

            m_gnssSatellites.append(sat);
        }
    }

    return NMEA_PARSE_OK;
}

/***************************************************************************
    $--GSA - GNSS DOP and Active Satellites
******************************************************************************/
// New method to handle all GSA sentence types in a uniform way
int NmeaParser::parseGsa(QStringList *data, int defaultSystem) {
    // Check for minimum required fields
    if (data->size() < 17) {  // At least need fields through VDOP
        if (m_debugEnabled) {
            qDebug() << "GSA: Not enough fields:" << data->size();
        }
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    // Determine the system based on default and/or system ID field
    int system = defaultSystem;
    QChar signalType = '1';  // Default signal

    // NMEA 4.10+ includes a System ID field at the end
    bool hasSystemIdField = (data->size() % 2 == 0);

    if (hasSystemIdField && data->size() > 17) {
        bool conversionOk = false;
        int systemId = data->at(17).toInt(&conversionOk);

        if (conversionOk) {
            system = systemId;
        } else if (m_debugEnabled) {
            qDebug() << "GSA: Invalid System ID:" << data->at(17);
        }

        // If there's a signal type in the last field, use it
        if (data->size() > 18 && !data->at(18).isEmpty()) {
            signalType = data->at(18)[0];
        }
    }

    // Parse Fix Type field
    int fixType = 0;
    if (!data->at(1).isEmpty()) {
        fixType = data->at(1).toInt();
    }

    // Parse satellite PRNs and mark them as used
    for (int i = 2; i < 14; i++) {  // Fields 2-13 contain PRNs
        if (i < data->size() && !data->at(i).isEmpty() && data->at(i) != "0") {
            int prn = data->at(i).toInt();

            // If prn is valid, find matching satellite and mark it as used
            if (prn > 0) {
                // Find this satellite in our list and mark it as used
                for (int j = 0; j < m_gnssSatellites.size(); j++) {
                    GnssSatellite &sat = m_gnssSatellites[j];

                    // Match both PRN and system, and signal if available
                    if (sat.prn == prn && sat.system == system) {
                        // If we have a specific signal type and it matches, or if no signal
                        // is specified
                        if (hasSystemIdField) {
                            // With system ID field present, we can be more precise about
                            // signal matching
                            if (sat.signal == signalType) {
                                sat.used = true;
                                break;
                            }
                        } else {
                            // Without system ID, we're less strict about signal matching
                            sat.used = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Parse DOP values if available
    if (data->size() > 14 && !data->at(14).isEmpty()) {
        double pdop = data->at(14).toDouble();
        m_gnssPosition.pdop = pdop;
    }

    if (data->size() > 15 && !data->at(15).isEmpty()) {
        double hdop = data->at(15).toDouble();
        m_gnssPosition.hdop = hdop;
    }

    if (data->size() > 16 && !data->at(16).isEmpty()) {
        double vdop = data->at(16).toDouble();
        m_gnssPosition.vdop = vdop;
    }

    return NMEA_PARSE_OK;
}

// Individual GSA parsers for each constellation, using the common
// implementation
int NmeaParser::parseGpgsa(QStringList *data) {
    return parseGsa(data, 1);  // GPS = 1
}

int NmeaParser::parseGlgsa(QStringList *data) {
    return parseGsa(data, 2);  // GLONASS = 2
}

int NmeaParser::parseGagsa(QStringList *data) {
    return parseGsa(data, 3);  // Galileo = 3
}

int NmeaParser::parseBdgsa(QStringList *data) {
    return parseGsa(data, 4);  // BeiDou = 4
}

int NmeaParser::parseQzgsa(QStringList *data) {
    return parseGsa(data, 5);  // QZSS = 5
}

int NmeaParser::parseNcgsa(QStringList *data) {
    return parseGsa(data, 6);  // IRNSS/NavIC = 6
}

// GNGSA is a mixed constellation message, we need special handling
int NmeaParser::parseGngsa(QStringList *data) {
    // For GN messages without a system ID, we need to guess the system based on
    // the PRN
    if (data->size() < 18) {  // No system ID field
        // Try to determine system from PRN ranges
        // This is a simplified approach - consult NMEA docs for your receiver
        int prn = 0;
        for (int i = 2; i < 14 && i < data->size(); i++) {
            if (!data->at(i).isEmpty() && data->at(i) != "0") {
                prn = data->at(i).toInt();
                if (prn > 0) {
                    break;  // Found a valid PRN
                }
            }
        }

        // Guess system based on PRN range
        if (prn >= 1 && prn <= 32) {
            return parseGsa(data, 1);  // GPS
        } else if (prn >= 33 && prn <= 64) {
            return parseGsa(data, 2);  // GLONASS
        } else if (prn >= 65 && prn <= 96) {
            return parseGsa(data, 3);  // Galileo
        } else if (prn >= 97 && prn <= 128) {
            return parseGsa(data, 4);  // BeiDou
        } else if (prn >= 129 && prn <= 160) {
            return parseGsa(data, 5);  // QZSS
        } else if (prn >= 161 && prn <= 192) {
            return parseGsa(data, 6);  // IRNSS
        } else {
            return parseGsa(data, 1);  // Default to GPS if unknown
        }
    } else {
        // If system ID is provided, use it
        return parseGsa(data, 0);  // 0 means use the system ID from the message
    }
}

// Error description helper
QString NmeaParser::getErrorDescription(int errorCode) const {
    switch (errorCode) {
        case NMEA_PARSE_OK:
            return "No error";
        case NMEA_PARSE_INVALID_FORMAT:
            return "Invalid NMEA format";
        case NMEA_PARSE_CHECKSUM_ERROR:
            return "Checksum error";
        case NMEA_PARSE_TOO_FEW_FIELDS:
            return "Too few fields";
        case NMEA_PARSE_UNSUPPORTED:
            return "Unsupported NMEA sentence";
        default:
            return "Unknown error";
    }
}
