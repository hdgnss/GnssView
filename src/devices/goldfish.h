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

#ifndef GOLDFISH_H
#define GOLDFISH_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <cstdint>

// Goldfish Protocol Constants
#define GOLDFISH_SYNC1 0xBC
#define GOLDFISH_SYNC2 0xB2
#define GOLDFISH_HEADER_LEN 4    // sync(2) + len_crc(2)
#define GOLDFISH_CHECKSUM_LEN 2  // 16-bit checksum

// Goldfish Message IDs (Example)
enum GoldfishMessageId {
    GOLDFISH_MSG_NAVPVT = 0x0001,
    GOLDFISH_MSG_STATUS = 0x0002,
};

// Goldfish Packet Information
struct GoldfishPacketInfo {
    uint16_t msgId;      // Message ID
    uint16_t length;     // Payload length (bits 16-27, 12-bit)
    bool valid;          // CRC and Checksum validation result
    QString msgName;     // Human-readable message name
    QByteArray payload;  // Raw payload data
};

// Goldfish Decoder Class
class GoldfishDecoder {
public:
    GoldfishDecoder();
    ~GoldfishDecoder();

    // Main decoding functions
    bool decodePacket(const QByteArray &data, int &bytesProcessed);
    GoldfishPacketInfo getLastPacketInfo() const;

    // Message utilities
    static QString messageName(uint16_t msgId);

    // Frame synchronization
    int findNextPacket(const QByteArray &data, int startPos = 0);

    // Statistics
    uint32_t getMessageCount(uint16_t msgId) const;
    uint32_t getTotalPackets() const;
    void resetStatistics();

private:
    // Core decoding functions
    uint16_t calculateChecksum(const uint8_t *data, int length);
    uint8_t calculateLenCrc(uint16_t length);

    // Byte extraction
    uint16_t getU2(const uint8_t *data);

    // Message parsing
    bool parsePacket(const QByteArray &payload);

    // State
    GoldfishPacketInfo m_lastPacket;
    QMap<uint16_t, uint32_t> m_packetStats;
    uint32_t m_totalPackets;
};

#endif  // GOLDFISH_H
