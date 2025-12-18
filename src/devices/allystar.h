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

#ifndef ALLYSTAR_H
#define ALLYSTAR_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <cstdint>

// ALLYSTAR Binary Protocol Constants
// Note: ALLYSTAR is a UBX variant with different sync bytes
#define ALLYSTAR_SYNC1 0xF1
#define ALLYSTAR_SYNC2 0xD9
#define ALLYSTAR_HEADER_LEN 6    // sync1 + sync2 + class + id + length(2)
#define ALLYSTAR_CHECKSUM_LEN 2  // CK_A + CK_B (Fletcher-8, same as UBX)

// ALLYSTAR Packet Information
struct AllystarPacketInfo {
    uint8_t msgClass;    // Message class
    uint8_t msgId;       // Message ID
    uint16_t length;     // Payload length
    bool valid;          // Checksum validation result
    QString className;   // Human-readable class name
    QString msgName;     // Human-readable message name
    QByteArray payload;  // Raw payload data
};

// ALLYSTAR Decoder Class
class AllystarDecoder {
public:
    AllystarDecoder();
    ~AllystarDecoder();

    // Main decoding functions
    bool decodePacket(const QByteArray &data, int &bytesProcessed);
    AllystarPacketInfo getLastPacketInfo() const;

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

    // State
    AllystarPacketInfo m_lastPacket;
    QMap<uint32_t, uint32_t> m_packetStats;  // Key: (class << 8) | id
    uint32_t m_totalPackets;
};

#endif  // ALLYSTAR_H
