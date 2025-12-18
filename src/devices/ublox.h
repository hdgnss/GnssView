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

#ifndef UBLOX_H
#define UBLOX_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <cstdint>

// UBX Protocol Constants
#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62
#define UBX_HEADER_LEN 6    // sync1 + sync2 + class + id + length(2)
#define UBX_CHECKSUM_LEN 2  // CK_A + CK_B

// UBX Message Classes
enum UbxMessageClass {
    UBX_CLASS_NAV = 0x01,  // Navigation Results
    UBX_CLASS_RXM = 0x02,  // Receiver Manager
    UBX_CLASS_INF = 0x04,  // Information Messages
    UBX_CLASS_ACK = 0x05,  // Ack/Nak Messages
    UBX_CLASS_CFG = 0x06,  // Configuration Input
    UBX_CLASS_UPD = 0x09,  // Firmware Update
    UBX_CLASS_MON = 0x0A,  // Monitoring Messages
    UBX_CLASS_AID = 0x0B,  // AssistNow Aiding
    UBX_CLASS_TIM = 0x0D,  // Timing Messages
    UBX_CLASS_ESF = 0x10,  // External Sensor Fusion
    UBX_CLASS_MGA = 0x13,  // Multiple GNSS Assistance
    UBX_CLASS_LOG = 0x21,  // Logging Messages
    UBX_CLASS_SEC = 0x27,  // Security Feature
    UBX_CLASS_HNR = 0x28,  // High Rate Navigation
};

// NAV Class Message IDs
enum UbxNavMessages {
    UBX_NAV_POSECEF = 0x01,  // Position Solution in ECEF
    UBX_NAV_POSLLH = 0x02,   // Geodetic Position Solution
    UBX_NAV_STATUS = 0x03,   // Receiver Navigation Status
    UBX_NAV_DOP = 0x04,      // Dilution of Precision
    UBX_NAV_SOL = 0x06,      // Navigation Solution
    UBX_NAV_PVT = 0x07,      // Navigation Position Velocity Time
    UBX_NAV_VELECEF = 0x11,  // Velocity Solution in ECEF
    UBX_NAV_VELNED = 0x12,   // Velocity Solution in NED
    UBX_NAV_TIMEGPS = 0x20,  // GPS Time Solution
    UBX_NAV_TIMEUTC = 0x21,  // UTC Time Solution
    UBX_NAV_CLOCK = 0x22,    // Clock Solution
    UBX_NAV_SVINFO = 0x30,   // Space Vehicle Information
    UBX_NAV_SAT = 0x35,      // Satellite Information
    UBX_NAV_EOE = 0x61,      // End Of Epoch
};

// RXM Class Message IDs
enum UbxRxmMessages {
    UBX_RXM_RAW = 0x10,    // Raw Measurement Data
    UBX_RXM_SFRB = 0x11,   // Subframe Buffer
    UBX_RXM_RAWX = 0x15,   // Multi-GNSS Raw Measurement Data
    UBX_RXM_SFRBX = 0x13,  // Broadcast Navigation Data Subframe
};

// ACK Class Message IDs
enum UbxAckMessages {
    UBX_ACK_NAK = 0x00,  // Message Not-Acknowledged
    UBX_ACK_ACK = 0x01,  // Message Acknowledged
};

// CFG Class Message IDs
enum UbxCfgMessages {
    UBX_CFG_PRT = 0x00,   // Port Configuration
    UBX_CFG_MSG = 0x01,   // Message Configuration
    UBX_CFG_RATE = 0x08,  // Navigation/Measurement Rate Settings
    UBX_CFG_NAV5 = 0x24,  // Navigation Engine Settings
};

// MON Class Message IDs
enum UbxMonMessages {
    UBX_MON_VER = 0x04,  // Receiver/Software Version
    UBX_MON_HW = 0x09,   // Hardware Status
    UBX_MON_HW2 = 0x0B,  // Extended Hardware Status
};

// UBX Packet Information
struct UbxPacketInfo {
    uint8_t msgClass;    // Message class
    uint8_t msgId;       // Message ID
    uint16_t length;     // Payload length
    bool valid;          // Checksum validation result
    QString className;   // Human-readable class name
    QString msgName;     // Human-readable message name
    QByteArray payload;  // Raw payload data
};

// UBX NAV-PVT Structure (simplified)
struct UbxNavPvt {
    uint32_t iTOW;    // GPS time of week (ms)
    uint16_t year;    // Year (UTC)
    uint8_t month;    // Month (UTC)
    uint8_t day;      // Day (UTC)
    uint8_t hour;     // Hour (UTC)
    uint8_t min;      // Minute (UTC)
    uint8_t sec;      // Second (UTC)
    uint8_t valid;    // Validity flags
    int32_t lon;      // Longitude (1e-7 deg)
    int32_t lat;      // Latitude (1e-7 deg)
    int32_t height;   // Height above ellipsoid (mm)
    int32_t hMSL;     // Height above MSL (mm)
    uint32_t hAcc;    // Horizontal accuracy estimate (mm)
    uint32_t vAcc;    // Vertical accuracy estimate (mm)
    uint8_t fixType;  // GNSS fix type
    uint8_t numSV;    // Number of satellites used
};

// UBX Decoder Class
class UbloxDecoder {
public:
    UbloxDecoder();
    ~UbloxDecoder();

    // Main decoding functions
    bool decodePacket(const QByteArray &data, int &bytesProcessed);
    UbxPacketInfo getLastPacketInfo() const;

    // Message utilities
    static QString messageClassName(uint8_t msgClass);
    static QString messageName(uint8_t msgClass, uint8_t msgId);

    // Frame synchronization
    int findNextPacket(const QByteArray &data, int startPos = 0);

    // Statistics
    uint32_t getMessageCount(uint8_t msgClass, uint8_t msgId) const;
    uint32_t getTotalPackets() const;
    void resetStatistics();

private:
    // Core decoding functions
    void calculateChecksum(const uint8_t *data, int length, uint8_t &ck_a, uint8_t &ck_b);
    bool validateChecksum(const QByteArray &packet);

    // Helper functions for little-endian data
    uint16_t getU2(const uint8_t *data);
    uint32_t getU4(const uint8_t *data);
    int16_t getI2(const uint8_t *data);
    int32_t getI4(const uint8_t *data);

    // Message parsing
    bool parsePacket(const QByteArray &payload);
    bool parseNavPvt(const QByteArray &payload);
    bool parseMonVer(const QByteArray &payload);

    // State
    UbxPacketInfo m_lastPacket;
    UbxNavPvt m_lastNavPvt;
    QMap<uint32_t, uint32_t> m_packetStats;  // Key: (class << 8) | id
    uint32_t m_totalPackets;
};

#endif  // UBLOX_H
