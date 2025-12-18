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
 * ALLYSTAR Binary Protocol Decoder Implementation
 *
 * Note: ALLYSTAR binary is essentially a UBX clone with different sync bytes.
 * The message structure is identical: header, ID, length, payload, checksum.
 * Only the sync bytes changed from UBX's 0xB5 0x62 to 0xF1 0xD9.
 *
 * Reference:
 * - GPSD driver_allystar.c
 */

#include "allystar.h"

#include <QDebug>

//=============================================================================
// Constructor / Destructor
//=============================================================================

AllystarDecoder::AllystarDecoder() : m_totalPackets(0) {
    m_lastPacket.valid = false;
    m_lastPacket.msgClass = 0;
    m_lastPacket.msgId = 0;
    m_lastPacket.length = 0;
}

AllystarDecoder::~AllystarDecoder() {
}

//=============================================================================
// Fletcher Checksum Calculation (Same as UBX)
//=============================================================================

void AllystarDecoder::calculateChecksum(const uint8_t *data, int length, uint8_t &ck_a, uint8_t &ck_b) {
    ck_a = 0;
    ck_b = 0;

    for (int i = 0; i < length; i++) {
        ck_a += data[i];
        ck_b += ck_a;
    }
}

//=============================================================================
// Little-Endian Data Extraction Helpers (Same as UBX)
//=============================================================================

uint16_t AllystarDecoder::getU2(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

uint32_t AllystarDecoder::getU4(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

int16_t AllystarDecoder::getI2(const uint8_t *data) {
    return (int16_t)getU2(data);
}

int32_t AllystarDecoder::getI4(const uint8_t *data) {
    return (int32_t)getU4(data);
}

//=============================================================================
// Frame Synchronization
// Changed to look for 0xF1 0xD9 instead of UBX's 0xB5 0x62
//=============================================================================

int AllystarDecoder::findNextPacket(const QByteArray &data, int startPos) {
    for (int i = startPos; i < data.length() - 1; i++) {
        if ((uint8_t)data[i] == ALLYSTAR_SYNC1 && (uint8_t)data[i + 1] == ALLYSTAR_SYNC2) {
            // Found ALLYSTAR sync bytes
            if (i + ALLYSTAR_HEADER_LEN <= data.length()) {
                // Extract length field (little-endian, bytes 4-5)
                uint16_t len = (uint8_t)data[i + 4] | ((uint8_t)data[i + 5] << 8);

                // Basic sanity check on length
                if (len <= 4096) {
                    return i;
                }
            }
        }
    }
    return -1;  // No packet found
}

//=============================================================================
// Checksum Validation (Same algorithm as UBX)
//=============================================================================

bool AllystarDecoder::validateChecksum(const QByteArray &packet) {
    if (packet.length() < ALLYSTAR_HEADER_LEN + ALLYSTAR_CHECKSUM_LEN) {
        return false;
    }

    // Checksum calculated from class through end of payload
    int checksumStart = 2;  // Skip sync bytes
    int checksumEnd = packet.length() - ALLYSTAR_CHECKSUM_LEN;
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

bool AllystarDecoder::decodePacket(const QByteArray &data, int &bytesProcessed) {
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
    if (data.length() < ALLYSTAR_HEADER_LEN) {
        return false;  // Need more data
    }

    // Extract packet info from header
    m_lastPacket.msgClass = (uint8_t)data[2];
    m_lastPacket.msgId = (uint8_t)data[3];
    m_lastPacket.length = (uint8_t)data[4] | ((uint8_t)data[5] << 8);  // Little-endian

    // Calculate total packet length
    int packetLen = ALLYSTAR_HEADER_LEN + m_lastPacket.length + ALLYSTAR_CHECKSUM_LEN;

    // Check if we have complete packet
    if (data.length() < packetLen) {
        return false;  // Need more data
    }

    // Extract packet
    QByteArray packet = data.left(packetLen);
    bytesProcessed = packetLen;

    // Validate checksum
    if (!validateChecksum(packet)) {
        qWarning() << "ALLYSTAR: Checksum validation failed for"
                   << QString("0x%1-0x%2")
                          .arg(m_lastPacket.msgClass, 2, 16, QChar('0'))
                          .arg(m_lastPacket.msgId, 2, 16, QChar('0'));
        return false;
    }

    // Extract payload
    QByteArray payload = packet.mid(ALLYSTAR_HEADER_LEN, m_lastPacket.length);
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
// ALLYSTAR may use different class/ID values than UBX
//=============================================================================

bool AllystarDecoder::parsePacket(const QByteArray &payload) {
    // For now, just acknowledge receipt and log
    qDebug() << "ALLYSTAR: Message class" << QString("0x%1").arg(m_lastPacket.msgClass, 2, 16, QChar('0')) << "ID"
             << QString("0x%1").arg(m_lastPacket.msgId, 2, 16, QChar('0')) << "length" << m_lastPacket.length;

    // TODO: Add specific ALLYSTAR message parsers as needed
    // The message structure follows UBX, so parsers can be similar

    return true;
}

//=============================================================================
// Message Name Utilities
//=============================================================================

QString AllystarDecoder::messageClassName(uint8_t msgClass) {
    // ALLYSTAR may define its own class IDs
    // For now, use hex representation
    return QString("0x%1").arg(msgClass, 2, 16, QChar('0'));
}

QString AllystarDecoder::messageName(uint8_t msgClass, uint8_t msgId) {
    // ALLYSTAR may define its own message IDs
    // For now, use hex representation
    return QString("0x%1-0x%2").arg(msgClass, 2, 16, QChar('0')).arg(msgId, 2, 16, QChar('0'));
}

//=============================================================================
// Statistics
//=============================================================================

AllystarPacketInfo AllystarDecoder::getLastPacketInfo() const {
    return m_lastPacket;
}

uint32_t AllystarDecoder::getMessageCount(uint8_t msgClass, uint8_t msgId) const {
    uint32_t key = ((uint32_t)msgClass << 8) | msgId;
    return m_packetStats.value(key, 0);
}

uint32_t AllystarDecoder::getTotalPackets() const {
    return m_totalPackets;
}

void AllystarDecoder::resetStatistics() {
    m_packetStats.clear();
    m_totalPackets = 0;
}
