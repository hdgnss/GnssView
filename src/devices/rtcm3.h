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

#ifndef RTCM3_H
#define RTCM3_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <cstdint>

// RTCM3 Constants
#define RTCM3_PREAMBLE 0xD3
#define RTCM3_MAX_MSG_LEN 1023
#define RTCM3_HEADER_LEN 3  // preamble + 2 bytes for length
#define RTCM3_CRC_LEN 3     // 24-bit CRC

// Common RTCM3 Message Types
enum Rtcm3MessageType {
    // GPS Observations
    RTCM3_MSG_1001 = 1001,  // GPS L1 observations
    RTCM3_MSG_1002 = 1002,  // GPS L1 observations (extended)
    RTCM3_MSG_1003 = 1003,  // GPS L1/L2 observations
    RTCM3_MSG_1004 = 1004,  // GPS L1/L2 observations (extended)

    // Station Info
    RTCM3_MSG_1005 = 1005,  // Station coordinates (XYZ)
    RTCM3_MSG_1006 = 1006,  // Station coordinates (XYZ) + height
    RTCM3_MSG_1007 = 1007,  // Antenna descriptor
    RTCM3_MSG_1008 = 1008,  // Antenna descriptor + serial

    // GLONASS Observations
    RTCM3_MSG_1009 = 1009,  // GLONASS L1 observations
    RTCM3_MSG_1010 = 1010,  // GLONASS L1 observations (extended)
    RTCM3_MSG_1011 = 1011,  // GLONASS L1/L2 observations
    RTCM3_MSG_1012 = 1012,  // GLONASS L1/L2 observations (extended)

    // Ephemeris
    RTCM3_MSG_1019 = 1019,  // GPS ephemeris
    RTCM3_MSG_1020 = 1020,  // GLONASS ephemeris
    RTCM3_MSG_1045 = 1045,  // Galileo ephemeris (F/NAV)
    RTCM3_MSG_1046 = 1046,  // Galileo ephemeris (I/NAV)

    // Text Message
    RTCM3_MSG_1029 = 1029,  // Unicode text string

    // MSM (Multiple Signal Messages) - GPS
    RTCM3_MSG_1074 = 1074,  // GPS MSM4
    RTCM3_MSG_1075 = 1075,  // GPS MSM5
    RTCM3_MSG_1076 = 1076,  // GPS MSM6
    RTCM3_MSG_1077 = 1077,  // GPS MSM7

    // MSM - GLONASS
    RTCM3_MSG_1084 = 1084,  // GLONASS MSM4
    RTCM3_MSG_1085 = 1085,  // GLONASS MSM5
    RTCM3_MSG_1086 = 1086,  // GLONASS MSM6
    RTCM3_MSG_1087 = 1087,  // GLONASS MSM7

    // MSM - Galileo
    RTCM3_MSG_1094 = 1094,  // Galileo MSM4
    RTCM3_MSG_1095 = 1095,  // Galileo MSM5
    RTCM3_MSG_1096 = 1096,  // Galileo MSM6
    RTCM3_MSG_1097 = 1097,  // Galileo MSM7

    // MSM - BeiDou
    RTCM3_MSG_1124 = 1124,  // BeiDou MSM4
    RTCM3_MSG_1125 = 1125,  // BeiDou MSM5
    RTCM3_MSG_1126 = 1126,  // BeiDou MSM6
    RTCM3_MSG_1127 = 1127,  // BeiDou MSM7
};

// RTCM3 Message Information
struct Rtcm3MessageInfo {
    uint16_t type;       // Message type (12 bits)
    uint16_t stationId;  // Station ID (12 bits)
    uint16_t length;     // Message length in bytes
    bool valid;          // CRC validation result
    QString typeName;    // Human-readable message type name
};

// RTCM3 Station Information (MSG 1005/1006)
struct Rtcm3StationInfo {
    uint16_t stationId;
    double ecefX;          // ECEF X coordinate (meters)
    double ecefY;          // ECEF Y coordinate (meters)
    double ecefZ;          // ECEF Z coordinate (meters)
    double antennaHeight;  // Antenna height (meters, MSG1006 only)
    bool hasHeight;        // True if MSG1006 with height info
};

// RTCM3 Decoder Class
class Rtcm3Decoder {
public:
    Rtcm3Decoder();
    ~Rtcm3Decoder();

    // Main decoding functions
    bool decodeFrame(const QByteArray &data, int &bytesProcessed);
    Rtcm3MessageInfo getLastMessageInfo() const;

    // Message type utilities
    static QString messageTypeName(uint16_t type);
    static QString messageTypeCategory(uint16_t type);

    // Frame synchronization
    int findNextFrame(const QByteArray &data, int startPos = 0);

    // Statistics
    uint32_t getMessageCount(uint16_t type) const;
    uint32_t getTotalMessages() const;
    void resetStatistics();

private:
    // Core decoding functions
    uint32_t calculateCrc24q(const uint8_t *data, int length);
    bool validateCrc(const QByteArray &frame);

    // Bit extraction helpers
    uint32_t getbitu(const uint8_t *buff, int pos, int len);
    int32_t getbits(const uint8_t *buff, int pos, int len);

    // Message parsing
    bool parseMessage(const QByteArray &data);
    bool parseStationInfo(const QByteArray &data, uint16_t type);
    bool parseTextMessage(const QByteArray &data);

    // State
    Rtcm3MessageInfo m_lastMessage;
    Rtcm3StationInfo m_stationInfo;
    QMap<uint16_t, uint32_t> m_messageStats;
    uint32_t m_totalMessages;
};

#endif  // RTCM3_H
