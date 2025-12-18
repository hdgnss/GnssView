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

#include "goldfish.h"

#include <QDebug>

//=============================================================================
// Constructor / Destructor
//=============================================================================

GoldfishDecoder::GoldfishDecoder() : m_totalPackets(0) {
    m_lastPacket.valid = false;
    m_lastPacket.msgId = 0x0000;
    m_lastPacket.length = 0;
}

GoldfishDecoder::~GoldfishDecoder() {
}

//=============================================================================
// Checksum Calculation
//=============================================================================

uint16_t GoldfishDecoder::calculateChecksum(const uint8_t *data, int length) {
    uint16_t checksum = 0;
    // Simple 16-bit sum as placeholder
    for (int i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

uint8_t GoldfishDecoder::calculateLenCrc(uint16_t length) {
    // Placeholder 4-bit CRC (XOR of nibbles)
    uint8_t nibble1 = (length >> 8) & 0x0F;
    uint8_t nibble2 = (length >> 4) & 0x0F;
    uint8_t nibble3 = length & 0x0F;
    return (nibble1 ^ nibble2 ^ nibble3) & 0x0F;
}

//=============================================================================
// Data Extraction Helpers
//=============================================================================

uint16_t GoldfishDecoder::getU2(const uint8_t *data) {
    // Standard big-endian for goldfish based on 0xBCB2 sync
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

//=============================================================================
// Frame Synchronization
//=============================================================================

int GoldfishDecoder::findNextPacket(const QByteArray &data, int startPos) {
    for (int i = startPos; i < data.length() - 1; i++) {
        if ((uint8_t)data[i] == GOLDFISH_SYNC1 && (uint8_t)data[i + 1] == GOLDFISH_SYNC2) {
            // Found sync, check header length
            if (i + GOLDFISH_HEADER_LEN <= data.length()) {
                // Extract 12-bit length (bits 16-27)
                uint16_t len = ((uint8_t)data[i + 2] << 4) | (((uint8_t)data[i + 3] >> 4) & 0x0F);
                if (len <= 4096) {
                    return i;
                }
            }
        }
    }
    return -1;
}

//=============================================================================
// Main Decoding Function
//=============================================================================

bool GoldfishDecoder::decodePacket(const QByteArray &data, int &bytesProcessed) {
    bytesProcessed = 0;
    m_lastPacket.valid = false;

    int packetStart = findNextPacket(data);
    if (packetStart < 0) {
        bytesProcessed = data.length();
        return false;
    }

    if (packetStart > 0) {
        bytesProcessed = packetStart;
        return false;
    }

    if (data.length() < GOLDFISH_HEADER_LEN) {
        return false;
    }

    const uint8_t *buff = (const uint8_t *)data.constData();

    // 1. Extract length (12 bits) and CRC (4 bits) from header (bytes 2,3)
    // Format: [Len high 8] [Len low 4 | Len CRC 4]
    uint16_t length = (buff[2] << 4) | (buff[3] >> 4);
    // uint8_t lenCrc = buff[3] & 0x0F;

    // 2. Total packet length: Header(4) + Body(length) + Checksum(2)
    int totalLen = GOLDFISH_HEADER_LEN + length + GOLDFISH_CHECKSUM_LEN;

    if (data.length() < totalLen) {
        return false;
    }

    bytesProcessed = totalLen;

    // 3. Extract MsgID (16 bits) from the start of the body (offset 4)
    uint16_t msgId = ((uint16_t)buff[4] << 8) | (uint16_t)buff[5];

    // 4. Validation (placeholders for now)
    // uint8_t calculatedLenCrc = calculateLenCrc(length);
    // uint16_t calculatedChecksum = calculateChecksum(buff + GOLDFISH_HEADER_LEN, length);
    // uint16_t messageChecksum = (buff[totalLen - 2] << 8) | buff[totalLen - 1];

    // For now, accept based on structure
    m_lastPacket.valid = true;
    m_lastPacket.msgId = msgId;
    m_lastPacket.length = length;
    m_lastPacket.msgName = messageName(msgId);

    // Payload is the body MINUS the 2-byte Message ID
    m_lastPacket.payload = data.mid(GOLDFISH_HEADER_LEN + 2, length - 2);

    m_totalPackets++;
    m_packetStats[msgId]++;

    parsePacket(m_lastPacket.payload);

    return true;
}

//=============================================================================
// Message Parsing
//=============================================================================

bool GoldfishDecoder::parsePacket(const QByteArray &payload) {
    // Implement specific parsers here
    return true;
}

//=============================================================================
// Message Utilities
//=============================================================================

QString GoldfishDecoder::messageName(uint16_t msgId) {
    static const QMap<uint16_t, QString> names = {
        {0x0500, "GOLDFISH-ACK"},
        {0x0501, "GOLDFISH-NAK"},
    };
    return names.value(msgId, QString("0x%1").arg(msgId, 4, 16, QChar('0')).toUpper());
}

uint32_t GoldfishDecoder::getMessageCount(uint16_t msgId) const {
    return m_packetStats.value(msgId, 0);
}

uint32_t GoldfishDecoder::getTotalPackets() const {
    return m_totalPackets;
}

void GoldfishDecoder::resetStatistics() {
    m_packetStats.clear();
    m_totalPackets = 0;
}

GoldfishPacketInfo GoldfishDecoder::getLastPacketInfo() const {
    return m_lastPacket;
}
