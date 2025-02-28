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

#include "nmeaparser.h"

#include <QDate>
#include <QDebug>
#include <QTimeZone>

NmeaParser::NmeaParser(bool debug)
{
    mDebugEnabled = debug;

    mNmeaHashMap.insert("$GPGGA", &NmeaParser::GPGGA);
    mNmeaHashMap.insert("$GNGGA", &NmeaParser::GPGGA);
    mNmeaHashMap.insert("$GPRMC", &NmeaParser::GPRMC);
    mNmeaHashMap.insert("$GNRMC", &NmeaParser::GPRMC);
    mNmeaHashMap.insert("$INRMC", &NmeaParser::GPRMC);

    mNmeaHashMap.insert("$GPGSV", &NmeaParser::GPGSV);
    mNmeaHashMap.insert("$GLGSV", &NmeaParser::GLGSV);
    mNmeaHashMap.insert("$QZGSV", &NmeaParser::QZGSV);
    mNmeaHashMap.insert("$GQGSV", &NmeaParser::QZGSV);
    mNmeaHashMap.insert("$BDGSV", &NmeaParser::BDGSV);
    mNmeaHashMap.insert("$GBGSV", &NmeaParser::BDGSV);
    mNmeaHashMap.insert("$GAGSV", &NmeaParser::GAGSV);
    mNmeaHashMap.insert("$NCGSV", &NmeaParser::NCGSV);
    mNmeaHashMap.insert("$GIGSV", &NmeaParser::NCGSV);
    mNmeaHashMap.insert("$GPGSA", &NmeaParser::GPGSA);
    mNmeaHashMap.insert("$QZGSA", &NmeaParser::QZGSA);
    mNmeaHashMap.insert("$BDGSA", &NmeaParser::BDGSA);
    mNmeaHashMap.insert("$GAGSA", &NmeaParser::GAGSA);
    mNmeaHashMap.insert("$GNGSA", &NmeaParser::GNGSA);
    mNmeaHashMap.insert("$NCGSA", &NmeaParser::NCGSA);
}

bool NmeaParser::checkSum(const QString &nmeaSentence, bool ignore)
{
    if (nmeaSentence.at(0) == '$' && nmeaSentence.at(nmeaSentence.size() - 3) == '*')
    {
        if (ignore)
            return true;

        bool ok;
        unsigned int checksum = nmeaSentence.mid(nmeaSentence.size() - 2, 2).toInt(&ok, 16);
        unsigned int sum = std::accumulate(nmeaSentence.begin() + 1, nmeaSentence.end() - 3, 0,
                                           [](unsigned int sum, QChar c)
                                           { return sum ^ c.toLatin1(); });
        if (sum == checksum)
        {
            return true;
        }
        else
        {
            if (mDebugEnabled)
            {
                qDebug() << "Checksum mismatch: " << nmeaSentence
                         << "sum: " << Qt::hex << sum
                         << "checksum: " << Qt::hex << checksum;
            }
            return false;
        }
    }
    if (mDebugEnabled)
    {
        qDebug() << "Not NMEA: " << nmeaSentence;
    }
    return false;
}

double NmeaParser::toDegrees(QString nmea, QString dir)
{
    double decimal = 0;
    if (nmea.size() > 5)
    {
        decimal = nmea.toDouble();
        int deg = (int)(decimal / 100.0);
        decimal = (double)deg + (decimal - (double)deg * 100) / 60.0;
    }
    if (dir == "W" || dir == "S")
    {
        decimal = -decimal;
    }
    return decimal;
}

int NmeaParser::parseNMEASentence(const QString &sentence, bool ignore)
{
    QStringList parts = sentence.split("*");
    if (parts.size() != 2)
    {
        return NMEA_PARSE_INVALID_FORMAT;
    }

    if (!checkSum(sentence, ignore))
    {
        return NMEA_PARSE_CHECKSUM_ERROR;
    }

    QStringList fields = parts[0].split(",");
    if (fields.size() < 2)
    {
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    QString type;
    if (sentence.at(1) == 'P')
    {
        ;
    }
    else
    {
        type = fields[0];
        fields.removeFirst();
    }

    if (mNmeaHashMap.contains(type))
    {
        return (this->*mNmeaHashMap[type])(&fields);
    }
    else
    {
        if (mDebugEnabled)
        {
            //qDebug() << "Not support: " << type;
        }
    }
    return NMEA_PARSE_UNSUPPORTED;
}

/***************************************************************************
    $GPRMC - Recommended Minimum Specific GPS/TRANSIT Data
    https://docs.novatel.com/OEM7/Content/Logs/GPRMC.htm
    
    Example:
    $GPRMC,203522.00,A,5109.0262308,N,11401.8407342,W,0.004,133.4,130522,0.0,E,D*2B
    $GNRMC,204520.00,A,5109.0262239,N,11401.8407338,W,0.004,102.3,130522,0.0,E,D*3B
    
    Field structure:
    0: UTC Time (hhmmss.ss)
    1: Status (A=active/valid, V=void/invalid)
    2: Latitude (ddmm.mmmm)
    3: N/S Indicator
    4: Longitude (dddmm.mmmm)
    5: E/W Indicator
    6: Speed over ground (knots)
    7: Course over ground (degrees true)
    8: Date (ddmmyy)
    9: Magnetic variation (degrees)
    10: Magnetic variation direction (E/W)
    11: Mode indicator (A=autonomous, D=differential, E=estimated, N=not valid)
    12: Navigational status (optional)
*******************************************************************************/
int NmeaParser::GPRMC(QStringList *data)
{
    // Verify we have the minimum required fields
    if (data->size() < 9) {
        if (mDebugEnabled) {
            qDebug() << "GPRMC: Not enough fields:" << data->size();
        }
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    // Mark that we have processed an RMC sentence (preferred for position data)
    mHaveGPRMC = true;
    
    // Check if time and date fields are not empty for proper timestamp processing
    if (!data->at(0).isEmpty() && !data->at(8).isEmpty()) {
        QString t = data->at(8) + " " + data->at(0);
        QDateTime datetime = QDateTime::fromString(t, "ddMMyy hhmmss.z").addYears(100);
        
        // Handle potential parsing errors
        if (!datetime.isValid()) {
            if (mDebugEnabled) {
                qDebug() << "GPRMC: Invalid datetime format:" << t;
            }
        } else {
            datetime.setTimeZone(QTimeZone::utc());
            mGnssPosition.utc = datetime.toMSecsSinceEpoch();
        }
    } else if (mDebugEnabled) {
        qDebug() << "GPRMC: Empty time or date field";
    }
    
    // Check status field (A=active/valid, V=void/invalid)
    if (!data->at(1).isEmpty()) {
        mGnssPosition.status = data->at(1);
        
        // If status is void/invalid, we might want to handle it specially
        if (mGnssPosition.status == "V" && mDebugEnabled) {
            qDebug() << "GPRMC: Position data marked invalid (V)";
        }
    }
    
    // Parse latitude and longitude if fields are not empty
    if (!data->at(2).isEmpty() && !data->at(4).isEmpty()) {
        mGnssPosition.latitude = toDegrees(data->at(2), data->at(3));
        mGnssPosition.longitude = toDegrees(data->at(4), data->at(5));
    } else if (mDebugEnabled) {
        qDebug() << "GPRMC: Empty latitude/longitude fields";
    }
    
    // Speed over ground (in knots, might need conversion based on your application)
    if (data->size() > 6 && !data->at(6).isEmpty()) {
        mGnssPosition.speed = data->at(6).toDouble();
        // Convert knots to km/h or m/s if needed
        mGnssPosition.speed *= 1.852; // Convert knots to km/h
    }
    
    // Course over ground (degrees true)
    if (data->size() > 7 && !data->at(7).isEmpty()) {
        mGnssPosition.course = data->at(7).toDouble();
    }
    
    // Magnetic variation (degrees)
    if (data->size() > 9 && !data->at(9).isEmpty()) {
        double magVar = data->at(9).toDouble();
        // Handle E/W indicator for magnetic variation
        if (data->size() > 10 && !data->at(10).isEmpty()) {
            if (data->at(10) == "W") {
                magVar = -magVar; // West variation is negative
            }
        }
        mGnssPosition.magnetic = magVar;
    }
    
    // Mode indicator
    if (data->size() > 11 && !data->at(11).isEmpty()) {
        mGnssPosition.mode = data->at(11);
    } else {
        // Default mode if not specified (backwards compatibility)
        mGnssPosition.mode = "N";
    }
    
    // Optional: Navigational status field (added in NMEA 4.1)
    if (data->size() > 12 && !data->at(12).isEmpty()) {
        // You might want to add this field to your GnssPosition struct
        //mGnssPosition.navStatus = data->at(12);
    }
    
    // Copy data to GnssInfo struct and emit signal
    memset(&mGnssInfo.position, 0, sizeof(mGnssInfo.position));
    mGnssInfo.satellites.clear();
    
    mGnssInfo.position = mGnssPosition;
    mGnssInfo.satellites = mGnssSatellites;
    
    // Clear temporary storage for next update
    memset(&mGnssPosition, 0, sizeof(mGnssPosition));
    mGnssSatellites.clear();
    
    // Notify listeners of new NMEA data
    emit newNmeaParseDone(mGnssInfo);
    
    return NMEA_PARSE_OK;
}

/***************************************************************************
    $GPGGA - Global positioning system (GPS) fix data
    https://docs.novatel.com/OEM7/Content/Logs/GPGGA.htm
    $GPGGA,202530.00,5109.0262,N,11401.8407,W,5,40,0.5,1097.36,M,-17.00,M,18,TSTR*61
    
    Field structure:
    0: UTC Time (hhmmss.ss)
    1: Latitude (ddmm.mmmm)
    2: N/S Indicator
    3: Longitude (dddmm.mmmm)
    4: E/W Indicator
    5: Quality Indicator
    6: Number of satellites in use
    7: HDOP
    8: MSL Altitude
    9: Units (M for meters)
    10: Geoidal Separation (Undulation)
    11: Units (M for meters)
    12: Age of Differential Corrections (null when DGPS not used)
    13: Differential Reference Station ID
******************************************************************************/
int NmeaParser::GPGGA(QStringList *data)
{
    // Verify we have the minimum required fields (at least up to quality indicator)
    if (data->size() < 6) {
        if (mDebugEnabled) {
            qDebug() << "GPGGA: Not enough fields:" << data->size();
        }
        return NMEA_PARSE_TOO_FEW_FIELDS;
    }

    // Only update position if we don't have RMC data (RMC is preferred for position)
    if (!mHaveGPRMC) {
        // Check if latitude/longitude fields are not empty
        if (!data->at(1).isEmpty() && !data->at(3).isEmpty()) {
            mGnssPosition.latitude = toDegrees(data->at(1), data->at(2));
            mGnssPosition.longitude = toDegrees(data->at(3), data->at(4));
        } else if (mDebugEnabled) {
            qDebug() << "GPGGA: Empty latitude/longitude fields";
        }
    }
    
    // Quality indicator (0-8, see NMEA spec)
    if (!data->at(5).isEmpty()) {
        mGnssPosition.quality = data->at(5).toInt();
    }
    
    //Number of satellites in use
    if (data->size() > 6 && !data->at(6).isEmpty()) {
        mGnssPosition.satellites = data->at(6).toInt();
    }
    
    // DOP - Horizontal Dilution of Precision
    if (data->size() > 7 && !data->at(7).isEmpty()) {
        mGnssPosition.dop = data->at(7).toDouble();
    }
    
    // Altitude
    if (data->size() > 8 && !data->at(8).isEmpty()) {
        mGnssPosition.altitude = data->at(8).toDouble();
        
        // Verify altitude units if available
        if (data->size() > 9 && data->at(9) != "M") {
            if (mDebugEnabled) {
                qDebug() << "GPGGA: Unexpected altitude units:" << data->at(9);
            }
        }
    }
    
    // Geoidal separation (undulation)
    if (data->size() > 10 && !data->at(10).isEmpty()) {
        mGnssPosition.undulation = data->at(10).toDouble();
        
        // Verify undulation units if available
        if (data->size() > 11 && data->at(11) != "M") {
            if (mDebugEnabled) {
                qDebug() << "GPGGA: Unexpected undulation units:" << data->at(11);
            }
        }
    }
    
    // Age of differential corrections
    if (data->size() > 12 && !data->at(12).isEmpty()) {
        mGnssPosition.age = data->at(12).toInt();
    }
    
    // Differential reference station ID
    if (data->size() > 13 && !data->at(13).isEmpty()) {
        mGnssPosition.station = data->at(13);
    }
    
    return NMEA_PARSE_OK;
}

// Implementations of the specific GSV parsers using the unified helper
int NmeaParser::GPGSV(QStringList *data)
{
    return parseGSV(data, 1); // GPS = 1
}

int NmeaParser::GLGSV(QStringList *data)
{
    return parseGSV(data, 2); // GLONASS = 2
}

int NmeaParser::GAGSV(QStringList *data)
{
    return parseGSV(data, 3, '7'); // Galileo = 3, default signal = '7'
}

int NmeaParser::BDGSV(QStringList *data)
{
    return parseGSV(data, 4); // BeiDou = 4
}

int NmeaParser::QZGSV(QStringList *data)
{
    return parseGSV(data, 5); // QZSS = 5
}

int NmeaParser::NCGSV(QStringList *data)
{
    return parseGSV(data, 6, '5'); // NavIC = 6, default signal = '5'
}

// Helper method to reduce duplication in GSV parsing
int NmeaParser::parseGSV(QStringList *data, int system, QChar signal)
{
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

            mGnssSatellites.append(sat);
        }
    }
    
    return NMEA_PARSE_OK;
}


/***************************************************************************
    $--GSA - GNSS DOP and Active Satellites
    
    Example:
    $GPGSA,A,3,04,05,09,12,,,,,,,,,1.0,0.8,0.7,1*3E
    $GLGSA,A,3,65,67,68,77,88,,,,,,,,1.0,0.8,0.7,2*2D
    
    Field structure:
    0: Mode 1 (M=manual, A=automatic)
    1: Mode 2 (Fix type: 1=no fix, 2=2D fix, 3=3D fix)
    2-13: PRNs of satellites used in position solution (up to 12 satellites)
    14: PDOP (Position Dilution of Precision)
    15: HDOP (Horizontal Dilution of Precision)
    16: VDOP (Vertical Dilution of Precision)
    17: GNSS System ID (optional in older sentences)
       1=GPS, 2=GLONASS, 3=Galileo, 4=BeiDou, etc.
       
    Note: Sentences with GNSS System ID are from NMEA 4.10 and later
    without the ID are pre-4.10 and can be assumed to be GPS
******************************************************************************/
// New method to handle all GSA sentence types in a uniform way
int NmeaParser::parseGSA(QStringList *data, int defaultSystem) 
{
    // Check for minimum required fields
    if (data->size() < 17) {  // At least need fields through VDOP
        if (mDebugEnabled) {
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
        } else if (mDebugEnabled) {
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
        
        // Store fix type in your position structure if needed
        // mGnssPosition.fixType = fixType;
    }
    
    // Parse satellite PRNs and mark them as used
    for (int i = 2; i < 14; i++) {  // Fields 2-13 contain PRNs
        if (i < data->size() && !data->at(i).isEmpty() && data->at(i) != "0") {
            int prn = data->at(i).toInt();
            
            // If prn is valid, find matching satellite and mark it as used
            if (prn > 0) {
                // Find this satellite in our list and mark it as used
                for (int j = 0; j < mGnssSatellites.size(); j++) {
                    GnssSatellite &sat = mGnssSatellites[j];
                    
                    // Match both PRN and system, and signal if available
                    if (sat.prn == prn && sat.system == system) {
                        // If we have a specific signal type and it matches, or if no signal is specified
                        if (hasSystemIdField) {
                            // With system ID field present, we can be more precise about signal matching
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
        mGnssPosition.pdop = pdop;
    }
    
    if (data->size() > 15 && !data->at(15).isEmpty()) {
        double hdop = data->at(15).toDouble();
        mGnssPosition.hdop = hdop;
    }
    
    if (data->size() > 16 && !data->at(16).isEmpty()) {
        double vdop = data->at(16).toDouble();
        mGnssPosition.vdop = vdop;
    }
    
    return NMEA_PARSE_OK;
}

// Individual GSA parsers for each constellation, using the common implementation
int NmeaParser::GPGSA(QStringList *data) {
    return parseGSA(data, 1);  // GPS = 1
}

int NmeaParser::GLGSA(QStringList *data) {
    return parseGSA(data, 2);  // GLONASS = 2
}

int NmeaParser::GAGSA(QStringList *data) {
    return parseGSA(data, 3);  // Galileo = 3
}

int NmeaParser::BDGSA(QStringList *data) {
    return parseGSA(data, 4);  // BeiDou = 4
}

int NmeaParser::QZGSA(QStringList *data) {
    return parseGSA(data, 5);  // QZSS = 5
}

int NmeaParser::NCGSA(QStringList *data) {
    return parseGSA(data, 6);  // IRNSS/NavIC = 6
}

// GNGSA is a mixed constellation message, we need special handling
int NmeaParser::GNGSA(QStringList *data) {
    // For GN messages without a system ID, we need to guess the system based on the PRN
    if (data->size() < 18) {  // No system ID field
        // Try to determine system from PRN ranges
        // This is a simplification - proper handling depends on your receiver
        int prn = 0;
        for (int i = 2; i < 14 && i < data->size(); i++) {
            if (!data->at(i).isEmpty() && data->at(i) != "0") {
                prn = data->at(i).toInt();
                if (prn > 0) break;  // Found a valid PRN
            }
        }
        
        // Guess system based on PRN range
        // This is a simplified approach - consult NMEA docs for your receiver
        if (prn >= 1 && prn <= 32) return parseGSA(data, 1);      // GPS
        else if (prn >= 33 && prn <= 64) return parseGSA(data, 2); // GLONASS
        else if (prn >= 65 && prn <= 96) return parseGSA(data, 3); // Galileo
        else if (prn >= 97 && prn <= 128) return parseGSA(data, 4); // BeiDou
        else if (prn >= 129 && prn <= 160) return parseGSA(data, 5); // QZSS
        else if (prn >= 161 && prn <= 192) return parseGSA(data, 6); // IRNSS
        else return parseGSA(data, 1);  // Default to GPS if unknown
    } else {
        // If system ID is provided, use it
        return parseGSA(data, 0);  // 0 means use the system ID from the message
    }
}

// Error description helper
QString NmeaParser::getErrorDescription(int errorCode) const
{
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
