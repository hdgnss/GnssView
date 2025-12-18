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

/*
 * RTCM3 Decoder Implementation
 *
 * References:
 * - RTCM 10403.x standard
 * - GPSD driver_rtcm3.c (https://gitlab.com/gpsd/gpsd/-/blob/master/drivers/driver_rtcm3.c)
 * - RTKLIB rtcm3.c (https://github.com/rtklibexplorer/RTKLIB/blob/main/src/rtcm3.c)
 */

#include "rtcm3.h"

#include <QDebug>

//=============================================================================
// Constructor / Destructor
//=============================================================================

Rtcm3Decoder::Rtcm3Decoder() : m_totalMessages(0) {
    m_lastMessage.valid = false;
    m_lastMessage.type = 0;
    m_lastMessage.stationId = 0;
    m_lastMessage.length = 0;
}

Rtcm3Decoder::~Rtcm3Decoder() {
}

//=============================================================================
// CRC-24Q Calculation (RTCM3 Standard)
//=============================================================================

uint32_t Rtcm3Decoder::calculateCrc24q(const uint8_t *data, int length) {
    // CRC-24Q polynomial: 0x1864CFB
    const uint32_t CRC24Q_POLY = 0x1864CFB;
    uint32_t crc = 0;

    for (int i = 0; i < length; i++) {
        crc ^= ((uint32_t)data[i]) << 16;
        for (int j = 0; j < 8; j++) {
            crc <<= 1;
            if (crc & 0x1000000) {
                crc ^= CRC24Q_POLY;
            }
        }
    }

    return crc & 0xFFFFFF;  // 24-bit CRC
}

//=============================================================================
// Bit Extraction Helpers (from RTKLIB)
//=============================================================================

uint32_t Rtcm3Decoder::getbitu(const uint8_t *buff, int pos, int len) {
    uint32_t bits = 0;
    for (int i = pos; i < pos + len; i++) {
        bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
    }
    return bits;
}

int32_t Rtcm3Decoder::getbits(const uint8_t *buff, int pos, int len) {
    uint32_t bits = getbitu(buff, pos, len);

    // Sign extension for negative numbers
    if (len > 0 && (bits & (1u << (len - 1)))) {
        bits |= (~0u << len);  // Extend sign bit
    }

    return (int32_t)bits;
}

//=============================================================================
// Frame Synchronization
//=============================================================================

int Rtcm3Decoder::findNextFrame(const QByteArray &data, int startPos) {
    for (int i = startPos; i < data.length(); i++) {
        if ((uint8_t)data[i] == RTCM3_PREAMBLE) {
            // Check if we have enough bytes for header
            if (i + RTCM3_HEADER_LEN <= data.length()) {
                // Extract length field (10 bits after 6 reserved bits)
                uint16_t len = ((uint8_t)data[i + 1] & 0x03) << 8 | (uint8_t)data[i + 2];

                // Basic sanity check on length
                if (len <= RTCM3_MAX_MSG_LEN) {
                    return i;
                }
            }
        }
    }
    return -1;  // No frame found
}

//=============================================================================
// CRC Validation
//=============================================================================

bool Rtcm3Decoder::validateCrc(const QByteArray &frame) {
    if (frame.length() < RTCM3_HEADER_LEN + RTCM3_CRC_LEN) {
        return false;
    }

    // CRC is calculated over message excluding the CRC itself
    int crcStart = frame.length() - RTCM3_CRC_LEN;
    uint32_t calculatedCrc = calculateCrc24q((const uint8_t *)frame.constData(), crcStart);

    // Extract CRC from message (3 bytes)
    uint32_t messageCrc =
        ((uint8_t)frame[crcStart] << 16) | ((uint8_t)frame[crcStart + 1] << 8) | ((uint8_t)frame[crcStart + 2]);

    return calculatedCrc == messageCrc;
}

//=============================================================================
// Main Decoding Function
//=============================================================================

bool Rtcm3Decoder::decodeFrame(const QByteArray &data, int &bytesProcessed) {
    bytesProcessed = 0;
    m_lastMessage.valid = false;

    // Find next frame
    int frameStart = findNextFrame(data);
    if (frameStart < 0) {
        bytesProcessed = data.length();
        return false;
    }

    // Skip any bytes before frame
    if (frameStart > 0) {
        bytesProcessed = frameStart;
        return false;
    }

    // Check if we have complete header
    if (data.length() < RTCM3_HEADER_LEN) {
        return false;  // Need more data
    }

    // Extract message length (10 bits from bytes 1-2)
    uint16_t msgLen = ((uint8_t)data[1] & 0x03) << 8 | (uint8_t)data[2];

    // Calculate total frame length
    int frameLen = RTCM3_HEADER_LEN + msgLen + RTCM3_CRC_LEN;

    // Check if we have complete frame
    if (data.length() < frameLen) {
        return false;  // Need more data
    }

    // Extract frame
    QByteArray frame = data.left(frameLen);
    bytesProcessed = frameLen;

    // Validate CRC
    if (!validateCrc(frame)) {
        qWarning() << "RTCM3: CRC validation failed";
        return false;
    }

    // Parse message
    QByteArray msgData = frame.mid(RTCM3_HEADER_LEN, msgLen);
    bool parseOk = parseMessage(msgData);

    if (parseOk) {
        m_lastMessage.valid = true;
        m_lastMessage.length = msgLen;
        m_totalMessages++;
        m_messageStats[m_lastMessage.type]++;
    }

    return parseOk;
}

//=============================================================================
// Message Parsing
//=============================================================================

bool Rtcm3Decoder::parseMessage(const QByteArray &data) {
    if (data.length() < 2) {
        return false;
    }

    const uint8_t *buff = (const uint8_t *)data.constData();

    // Extract message type (first 12 bits)
    m_lastMessage.type = getbitu(buff, 0, 12);

    // Extract station ID (next 12 bits, common to most messages)
    m_lastMessage.stationId = getbitu(buff, 12, 12);

    // Get human-readable type name
    m_lastMessage.typeName = messageTypeName(m_lastMessage.type);

    // Parse specific message types
    switch (m_lastMessage.type) {
        case RTCM3_MSG_1005:
        case RTCM3_MSG_1006:
            return parseStationInfo(data, m_lastMessage.type);

        case RTCM3_MSG_1029:
            return parseTextMessage(data);

        // Observation messages - basic acknowledgment
        case RTCM3_MSG_1001:
        case RTCM3_MSG_1002:
        case RTCM3_MSG_1003:
        case RTCM3_MSG_1004:
        case RTCM3_MSG_1009:
        case RTCM3_MSG_1010:
        case RTCM3_MSG_1011:
        case RTCM3_MSG_1012:
        case RTCM3_MSG_1074:
        case RTCM3_MSG_1075:
        case RTCM3_MSG_1076:
        case RTCM3_MSG_1077:
        case RTCM3_MSG_1084:
        case RTCM3_MSG_1085:
        case RTCM3_MSG_1086:
        case RTCM3_MSG_1087:
        case RTCM3_MSG_1094:
        case RTCM3_MSG_1095:
        case RTCM3_MSG_1096:
        case RTCM3_MSG_1097:
        case RTCM3_MSG_1124:
        case RTCM3_MSG_1125:
        case RTCM3_MSG_1126:
        case RTCM3_MSG_1127:
            // Observation messages recognized but not fully parsed
            qDebug() << "RTCM3: Observation message" << m_lastMessage.type << "from station" << m_lastMessage.stationId;
            return true;

        default:
            // Unknown message type
            qDebug() << "RTCM3: Unknown message type" << m_lastMessage.type;
            return true;  // Still valid, just not parsed
    }
}

//=============================================================================
// Station Info Parsing (MSG 1005/1006)
//=============================================================================

bool Rtcm3Decoder::parseStationInfo(const QByteArray &data, uint16_t type) {
    if (data.length() < 20) {  // Minimum length for station info
        return false;
    }

    const uint8_t *buff = (const uint8_t *)data.constData();
    int bitpos = 24;  // After type (12) and station ID (12)

    // Reserved bits (6)
    bitpos += 6;

    // ITRF realization year (6 bits)
    bitpos += 6;

    // GPS indicator (1 bit)
    bitpos += 1;

    // GLONASS indicator (1 bit)
    bitpos += 1;

    // Reserved for Galileo (1 bit)
    bitpos += 1;

    // Reference station indicator (1 bit)
    bitpos += 1;

    // ECEF X coordinate (38 bits, 0.0001 m resolution)
    int64_t ecef_x = getbits(buff, bitpos, 38);
    m_stationInfo.ecefX = ecef_x * 0.0001;
    bitpos += 38;

    // Single receiver oscillator (1 bit)
    bitpos += 1;

    // Reserved (1 bit)
    bitpos += 1;

    // ECEF Y coordinate (38 bits)
    int64_t ecef_y = getbits(buff, bitpos, 38);
    m_stationInfo.ecefY = ecef_y * 0.0001;
    bitpos += 38;

    // Quarter cycle indicator (2 bits)
    bitpos += 2;

    // ECEF Z coordinate (38 bits)
    int64_t ecef_z = getbits(buff, bitpos, 38);
    m_stationInfo.ecefZ = ecef_z * 0.0001;
    bitpos += 38;

    m_stationInfo.stationId = m_lastMessage.stationId;
    m_stationInfo.hasHeight = false;

    // MSG 1006 includes antenna height
    if (type == RTCM3_MSG_1006 && (bitpos + 16) <= data.length() * 8) {
        int32_t height = getbitu(buff, bitpos, 16);
        m_stationInfo.antennaHeight = height * 0.0001;
        m_stationInfo.hasHeight = true;
    }

    qDebug() << QString("RTCM3: Station %1 at ECEF (%.4f, %.4f, %.4f)")
                    .arg(m_stationInfo.stationId)
                    .arg(m_stationInfo.ecefX)
                    .arg(m_stationInfo.ecefY)
                    .arg(m_stationInfo.ecefZ);

    return true;
}

//=============================================================================
// Text Message Parsing (MSG 1029)
//=============================================================================

bool Rtcm3Decoder::parseTextMessage(const QByteArray &data) {
    if (data.length() < 4) {
        return false;
    }

    const uint8_t *buff = (const uint8_t *)data.constData();
    int bitpos = 24;  // After type and station ID

    // Reference station ID (12 bits)
    uint16_t refId = getbitu(buff, bitpos, 12);
    bitpos += 12;

    // Modified Julian Day (16 bits)
    bitpos += 16;

    // Seconds of day (17 bits)
    bitpos += 17;

    // Number of UTF-8 characters (7 bits)
    uint8_t numChars = getbitu(buff, bitpos, 7);
    bitpos += 7;

    // Number of UTF-8 bytes (8 bits)
    uint8_t numBytes = getbitu(buff, bitpos, 8);
    bitpos += 8;

    // Extract text (validate we have enough data)
    if ((bitpos + numBytes * 8) <= data.length() * 8) {
        QByteArray textData;
        for (int i = 0; i < numBytes; i++) {
            textData.append(getbitu(buff, bitpos, 8));
            bitpos += 8;
        }
        QString text = QString::fromUtf8(textData);
        qDebug() << "RTCM3: Text message from station" << refId << ":" << text;
    }

    return true;
}

//=============================================================================
// Message Type Utilities
//=============================================================================

QString Rtcm3Decoder::messageTypeName(uint16_t type) {
    static const QMap<uint16_t, QString> typeNames = {
        // GPS Observations
        {1001, "GPS L1 RTK"},
        {1002, "GPS L1 RTK Extended"},
        {1003, "GPS L1/L2 RTK"},
        {1004, "GPS L1/L2 RTK Extended"},

        // Station Info
        {1005, "Station Coordinates (XYZ)"},
        {1006, "Station Coordinates + Height"},
        {1007, "Antenna Descriptor"},
        {1008, "Antenna Descriptor + Serial"},

        // GLONASS Observations
        {1009, "GLONASS L1 RTK"},
        {1010, "GLONASS L1 RTK Extended"},
        {1011, "GLONASS L1/L2 RTK"},
        {1012, "GLONASS L1/L2 RTK Extended"},

        // Ephemeris
        {1019, "GPS Ephemeris"},
        {1020, "GLONASS Ephemeris"},
        {1045, "Galileo Ephemeris (F/NAV)"},
        {1046, "Galileo Ephemeris (I/NAV)"},

        // Text
        {1029, "Text Message"},

        // GPS MSM
        {1074, "GPS MSM4"},
        {1075, "GPS MSM5"},
        {1076, "GPS MSM6"},
        {1077, "GPS MSM7"},

        // GLONASS MSM
        {1084, "GLONASS MSM4"},
        {1085, "GLONASS MSM5"},
        {1086, "GLONASS MSM6"},
        {1087, "GLONASS MSM7"},

        // Galileo MSM
        {1094, "Galileo MSM4"},
        {1095, "Galileo MSM5"},
        {1096, "Galileo MSM6"},
        {1097, "Galileo MSM7"},

        // BeiDou MSM
        {1124, "BeiDou MSM4"},
        {1125, "BeiDou MSM5"},
        {1126, "BeiDou MSM6"},
        {1127, "BeiDou MSM7"},
    };

    return typeNames.value(type, QString("Unknown (%1)").arg(type));
}

QString Rtcm3Decoder::messageTypeCategory(uint16_t type) {
    if (type >= 1001 && type <= 1004) {
        return "GPS Observations";
    }
    if (type >= 1005 && type <= 1008) {
        return "Station Info";
    }
    if (type >= 1009 && type <= 1012) {
        return "GLONASS Observations";
    }
    if (type == 1019 || type == 1020) {
        return "Ephemeris";
    }
    if (type >= 1045 && type <= 1046) {
        return "Galileo Ephemeris";
    }
    if (type == 1029) {
        return "Text";
    }
    if (type >= 1074 && type <= 1077) {
        return "GPS MSM";
    }
    if (type >= 1084 && type <= 1087) {
        return "GLONASS MSM";
    }
    if (type >= 1094 && type <= 1097) {
        return "Galileo MSM";
    }
    if (type >= 1124 && type <= 1127) {
        return "BeiDou MSM";
    }
    return "Other";
}

//=============================================================================
// Statistics
//=============================================================================

Rtcm3MessageInfo Rtcm3Decoder::getLastMessageInfo() const {
    return m_lastMessage;
}

uint32_t Rtcm3Decoder::getMessageCount(uint16_t type) const {
    return m_messageStats.value(type, 0);
}

uint32_t Rtcm3Decoder::getTotalMessages() const {
    return m_totalMessages;
}

void Rtcm3Decoder::resetStatistics() {
    m_messageStats.clear();
    m_totalMessages = 0;
}
