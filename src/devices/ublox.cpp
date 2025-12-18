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
 * u-blox UBX Protocol Decoder Implementation
 *
 * References:
 * - u-blox M8 Receiver Description including Protocol Specification
 * - GPSD driver_ubx.c (https://gitlab.com/gpsd/gpsd/-/blob/master/drivers/driver_ubx.c)
 * - RTKLIB ublox.c (https://github.com/rtklibexplorer/RTKLIB/blob/main/src/rcv/ublox.c)
 */

#include "ublox.h"

#include <QDebug>

//=============================================================================
// Constructor / Destructor
//=============================================================================

UbloxDecoder::UbloxDecoder() : m_totalPackets(0) {
    m_lastPacket.valid = false;
    m_lastPacket.msgClass = 0;
    m_lastPacket.msgId = 0;
    m_lastPacket.length = 0;
}

UbloxDecoder::~UbloxDecoder() {
}

//=============================================================================
// Fletcher Checksum Calculation (UBX Standard)
//=============================================================================

void UbloxDecoder::calculateChecksum(const uint8_t *data, int length, uint8_t &ck_a, uint8_t &ck_b) {
    ck_a = 0;
    ck_b = 0;

    // Calculate Fletcher checksum over data
    for (int i = 0; i < length; i++) {
        ck_a += data[i];
        ck_b += ck_a;
    }
}

//=============================================================================
// Little-Endian Data Extraction Helpers
//=============================================================================

uint16_t UbloxDecoder::getU2(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

uint32_t UbloxDecoder::getU4(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

int16_t UbloxDecoder::getI2(const uint8_t *data) {
    return (int16_t)getU2(data);
}

int32_t UbloxDecoder::getI4(const uint8_t *data) {
    return (int32_t)getU4(data);
}

//=============================================================================
// Frame Synchronization
//=============================================================================

int UbloxDecoder::findNextPacket(const QByteArray &data, int startPos) {
    for (int i = startPos; i < data.length() - 1; i++) {
        if ((uint8_t)data[i] == UBX_SYNC1 && (uint8_t)data[i + 1] == UBX_SYNC2) {
            // Found sync bytes, check if we have enough for header
            if (i + UBX_HEADER_LEN <= data.length()) {
                // Extract length field (little-endian, bytes 4-5)
                uint16_t len = (uint8_t)data[i + 4] | ((uint8_t)data[i + 5] << 8);

                // Basic sanity check on length (max reasonable payload)
                if (len <= 4096) {  // Reasonable max for most UBX messages
                    return i;
                }
            }
        }
    }
    return -1;  // No packet found
}

//=============================================================================
// Checksum Validation
//=============================================================================

bool UbloxDecoder::validateChecksum(const QByteArray &packet) {
    if (packet.length() < UBX_HEADER_LEN + UBX_CHECKSUM_LEN) {
        return false;
    }

    // Checksum is calculated from class through end of payload
    // (excludes sync bytes and checksum itself)
    int checksumStart = 2;  // Skip sync bytes
    int checksumEnd = packet.length() - UBX_CHECKSUM_LEN;
    int checksumLen = checksumEnd - checksumStart;

    uint8_t ck_a_calc, ck_b_calc;
    calculateChecksum((const uint8_t *)packet.constData() + checksumStart, checksumLen, ck_a_calc, ck_b_calc);

    // Extract checksum from packet
    uint8_t ck_a_msg = (uint8_t)packet[checksumEnd];
    uint8_t ck_b_msg = (uint8_t)packet[checksumEnd + 1];

    return (ck_a_calc == ck_a_msg) && (ck_b_calc == ck_b_msg);
}

//=============================================================================
// Main Decoding Function
//=============================================================================

bool UbloxDecoder::decodePacket(const QByteArray &data, int &bytesProcessed) {
    bytesProcessed = 0;
    m_lastPacket.valid = false;

    // Find next packet
    int packetStart = findNextPacket(data);
    if (packetStart < 0) {
        bytesProcessed = data.length();
        return false;
    }

    // Skip any bytes before packet
    if (packetStart > 0) {
        bytesProcessed = packetStart;
        return false;
    }

    // Check if we have complete header
    if (data.length() < UBX_HEADER_LEN) {
        return false;  // Need more data
    }

    // Extract packet info from header
    m_lastPacket.msgClass = (uint8_t)data[2];
    m_lastPacket.msgId = (uint8_t)data[3];
    m_lastPacket.length = (uint8_t)data[4] | ((uint8_t)data[5] << 8);  // Little-endian

    // Calculate total packet length
    int packetLen = UBX_HEADER_LEN + m_lastPacket.length + UBX_CHECKSUM_LEN;

    // Check if we have complete packet
    if (data.length() < packetLen) {
        return false;  // Need more data
    }

    // Extract packet
    QByteArray packet = data.left(packetLen);
    bytesProcessed = packetLen;

    // Validate checksum
    if (!validateChecksum(packet)) {
        qWarning() << "UBX: Checksum validation failed for"
                   << QString("0x%1-0x%2")
                          .arg(m_lastPacket.msgClass, 2, 16, QChar('0'))
                          .arg(m_lastPacket.msgId, 2, 16, QChar('0'));
        return false;
    }

    // Extract payload
    QByteArray payload = packet.mid(UBX_HEADER_LEN, m_lastPacket.length);
    m_lastPacket.payload = payload;

    // Parse packet
    bool parseOk = parsePacket(payload);

    if (parseOk) {
        m_lastPacket.valid = true;
        m_lastPacket.className = messageClassName(m_lastPacket.msgClass);
        m_lastPacket.msgName = messageName(m_lastPacket.msgClass, m_lastPacket.msgId);
        m_totalPackets++;

        uint32_t key = ((uint32_t)m_lastPacket.msgClass << 8) | m_lastPacket.msgId;
        m_packetStats[key]++;
    }

    return parseOk;
}

//=============================================================================
// Packet Parsing
//=============================================================================

bool UbloxDecoder::parsePacket(const QByteArray &payload) {
    const uint8_t *data = (const uint8_t *)payload.constData();

    // Parse specific message types
    switch (m_lastPacket.msgClass) {
        case UBX_CLASS_NAV:
            switch (m_lastPacket.msgId) {
                case UBX_NAV_PVT:
                    return parseNavPvt(payload);
                default:
                    qDebug() << "UBX: NAV message" << QString("0x%1").arg(m_lastPacket.msgId, 2, 16, QChar('0'))
                             << "from class" << m_lastPacket.className;
                    return true;  // Acknowledged but not parsed
            }

        case UBX_CLASS_MON:
            switch (m_lastPacket.msgId) {
                case UBX_MON_VER:
                    return parseMonVer(payload);
                default:
                    qDebug() << "UBX: MON message" << QString("0x%1").arg(m_lastPacket.msgId, 2, 16, QChar('0'));
                    return true;
            }

        case UBX_CLASS_RXM:
            qDebug() << "UBX: RXM raw measurement message"
                     << QString("0x%1").arg(m_lastPacket.msgId, 2, 16, QChar('0'));
            return true;

        case UBX_CLASS_ACK:
            qDebug() << "UBX:" << (m_lastPacket.msgId == UBX_ACK_ACK ? "ACK" : "NAK");
            return true;

        default:
            qDebug() << "UBX: Message class" << QString("0x%1").arg(m_lastPacket.msgClass, 2, 16, QChar('0')) << "ID"
                     << QString("0x%1").arg(m_lastPacket.msgId, 2, 16, QChar('0'));
            return true;  // Unknown but valid
    }
}

//=============================================================================
// NAV-PVT Parsing (Position Velocity Time)
//=============================================================================

bool UbloxDecoder::parseNavPvt(const QByteArray &payload) {
    if (payload.length() < 92) {  // Minimum length for NAV-PVT
        return false;
    }

    const uint8_t *data = (const uint8_t *)payload.constData();

    // Parse NAV-PVT payload
    m_lastNavPvt.iTOW = getU4(data);      // GPS time of week (ms)
    m_lastNavPvt.year = getU2(data + 4);  // Year (UTC)
    m_lastNavPvt.month = data[6];         // Month
    m_lastNavPvt.day = data[7];           // Day
    m_lastNavPvt.hour = data[8];          // Hour
    m_lastNavPvt.min = data[9];           // Minute
    m_lastNavPvt.sec = data[10];          // Second
    m_lastNavPvt.valid = data[11];        // Validity flags

    m_lastNavPvt.lon = getI4(data + 24);     // Longitude (1e-7 deg)
    m_lastNavPvt.lat = getI4(data + 28);     // Latitude (1e-7 deg)
    m_lastNavPvt.height = getI4(data + 32);  // Height above ellipsoid (mm)
    m_lastNavPvt.hMSL = getI4(data + 36);    // Height above MSL (mm)
    m_lastNavPvt.hAcc = getU4(data + 40);    // Horizontal accuracy (mm)
    m_lastNavPvt.vAcc = getU4(data + 44);    // Vertical accuracy (mm)

    m_lastNavPvt.fixType = data[20];  // GNSS fix type
    m_lastNavPvt.numSV = data[23];    // Number of satellites

    qDebug() << QString("UBX NAV-PVT: %1-%2-%3 %4:%5:%6 Fix=%7 SV=%8 Lat=%.7f Lon=%.7f Alt=%.3fm")
                    .arg(m_lastNavPvt.year)
                    .arg(m_lastNavPvt.month, 2, 10, QChar('0'))
                    .arg(m_lastNavPvt.day, 2, 10, QChar('0'))
                    .arg(m_lastNavPvt.hour, 2, 10, QChar('0'))
                    .arg(m_lastNavPvt.min, 2, 10, QChar('0'))
                    .arg(m_lastNavPvt.sec, 2, 10, QChar('0'))
                    .arg(m_lastNavPvt.fixType)
                    .arg(m_lastNavPvt.numSV)
                    .arg(m_lastNavPvt.lat * 1e-7)
                    .arg(m_lastNavPvt.lon * 1e-7)
                    .arg(m_lastNavPvt.hMSL * 0.001);

    return true;
}

//=============================================================================
// MON-VER Parsing (Receiver/Software Version)
//=============================================================================

bool UbloxDecoder::parseMonVer(const QByteArray &payload) {
    if (payload.length() < 40) {
        return false;
    }

    const uint8_t *data = (const uint8_t *)payload.constData();

    // Extract software version (30 bytes, null-terminated)
    QByteArray swVersion((const char *)data, 30);
    int nullPos = swVersion.indexOf('\0');
    if (nullPos >= 0) {
        swVersion = swVersion.left(nullPos);
    }

    // Extract hardware version (10 bytes, null-terminated)
    QByteArray hwVersion((const char *)(data + 30), 10);
    nullPos = hwVersion.indexOf('\0');
    if (nullPos >= 0) {
        hwVersion = hwVersion.left(nullPos);
    }

    qDebug() << "UBX MON-VER: SW =" << swVersion << ", HW =" << hwVersion;

    // Extensions follow (30-byte blocks), but we'll skip them for now

    return true;
}

//=============================================================================
// Message Name Utilities
//=============================================================================

QString UbloxDecoder::messageClassName(uint8_t msgClass) {
    static const QMap<uint8_t, QString> classNames = {
        {UBX_CLASS_NAV, "NAV"}, {UBX_CLASS_RXM, "RXM"}, {UBX_CLASS_INF, "INF"}, {UBX_CLASS_ACK, "ACK"},
        {UBX_CLASS_CFG, "CFG"}, {UBX_CLASS_UPD, "UPD"}, {UBX_CLASS_MON, "MON"}, {UBX_CLASS_AID, "AID"},
        {UBX_CLASS_TIM, "TIM"}, {UBX_CLASS_ESF, "ESF"}, {UBX_CLASS_MGA, "MGA"}, {UBX_CLASS_LOG, "LOG"},
        {UBX_CLASS_SEC, "SEC"}, {UBX_CLASS_HNR, "HNR"},
    };

    return classNames.value(msgClass, QString("0x%1").arg(msgClass, 2, 16, QChar('0')));
}

QString UbloxDecoder::messageName(uint8_t msgClass, uint8_t msgId) {
    static const QMap<uint16_t, QString> messageNames = {
        // NAV messages
        {(UBX_CLASS_NAV << 8) | UBX_NAV_POSECEF, "NAV-POSECEF"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_POSLLH, "NAV-POSLLH"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_STATUS, "NAV-STATUS"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_DOP, "NAV-DOP"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_SOL, "NAV-SOL"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_PVT, "NAV-PVT"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_VELECEF, "NAV-VELECEF"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_VELNED, "NAV-VELNED"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_TIMEGPS, "NAV-TIMEGPS"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_TIMEUTC, "NAV-TIMEUTC"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_CLOCK, "NAV-CLOCK"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_SVINFO, "NAV-SVINFO"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_SAT, "NAV-SAT"},
        {(UBX_CLASS_NAV << 8) | UBX_NAV_EOE, "NAV-EOE"},

        // RXM messages
        {(UBX_CLASS_RXM << 8) | UBX_RXM_RAW, "RXM-RAW"},
        {(UBX_CLASS_RXM << 8) | UBX_RXM_SFRB, "RXM-SFRB"},
        {(UBX_CLASS_RXM << 8) | UBX_RXM_RAWX, "RXM-RAWX"},
        {(UBX_CLASS_RXM << 8) | UBX_RXM_SFRBX, "RXM-SFRBX"},

        // ACK messages
        {(UBX_CLASS_ACK << 8) | UBX_ACK_NAK, "ACK-NAK"},
        {(UBX_CLASS_ACK << 8) | UBX_ACK_ACK, "ACK-ACK"},

        // CFG messages
        {(UBX_CLASS_CFG << 8) | UBX_CFG_PRT, "CFG-PRT"},
        {(UBX_CLASS_CFG << 8) | UBX_CFG_MSG, "CFG-MSG"},
        {(UBX_CLASS_CFG << 8) | UBX_CFG_RATE, "CFG-RATE"},
        {(UBX_CLASS_CFG << 8) | UBX_CFG_NAV5, "CFG-NAV5"},

        // MON messages
        {(UBX_CLASS_MON << 8) | UBX_MON_VER, "MON-VER"},
        {(UBX_CLASS_MON << 8) | UBX_MON_HW, "MON-HW"},
        {(UBX_CLASS_MON << 8) | UBX_MON_HW2, "MON-HW2"},
    };

    uint16_t key = ((uint16_t)msgClass << 8) | msgId;
    return messageNames.value(key, QString("%1-0x%2").arg(messageClassName(msgClass)).arg(msgId, 2, 16, QChar('0')));
}

//=============================================================================
// Statistics
//=============================================================================

UbxPacketInfo UbloxDecoder::getLastPacketInfo() const {
    return m_lastPacket;
}

uint32_t UbloxDecoder::getMessageCount(uint8_t msgClass, uint8_t msgId) const {
    uint32_t key = ((uint32_t)msgClass << 8) | msgId;
    return m_packetStats.value(key, 0);
}

uint32_t UbloxDecoder::getTotalPackets() const {
    return m_totalPackets;
}

void UbloxDecoder::resetStatistics() {
    m_packetStats.clear();
    m_totalPackets = 0;
}
