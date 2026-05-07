#include <QByteArray>
#include <QList>
#include <QString>

#include <cstdlib>
#include <iostream>

#include "src/core/StreamChunker.h"

namespace {

using hdgnss::StreamChunk;
using hdgnss::StreamChunkKind;
using hdgnss::StreamChunker;

bool expectSingleTextChunk(const char *label, const QByteArray &payload) {
    StreamChunker chunker;
    chunker.append(payload);
    const QList<StreamChunk> chunks = chunker.takeAvailableChunks();

    if (chunks.size() != 1) {
        std::cerr << label << ": expected exactly 1 chunk, got " << chunks.size() << "\n";
        return false;
    }
    if (chunks.first().kind == StreamChunkKind::Binary) {
        std::cerr << label << ": expected text-like chunk, got binary\n";
        return false;
    }
    if (chunks.first().payload != payload) {
        std::cerr << label << ": payload mismatch after chunking\n";
        return false;
    }
    return true;
}

bool expectSingleBinaryChunk(const char *label, const QByteArray &payload) {
    StreamChunker chunker;
    chunker.append(payload);
    const QList<StreamChunk> chunks = chunker.takeAvailableChunks();

    if (chunks.size() != 1) {
        std::cerr << label << ": expected exactly 1 chunk, got " << chunks.size() << "\n";
        return false;
    }
    if (chunks.first().kind != StreamChunkKind::Binary) {
        std::cerr << label << ": expected binary chunk\n";
        return false;
    }
    if (chunks.first().payload != payload) {
        std::cerr << label << ": payload mismatch after chunking\n";
        return false;
    }
    return true;
}

bool expectBinaryThenText(const char *label,
                          const QByteArray &payload,
                          const QByteArray &binaryPart,
                          const QByteArray &textPart) {
    StreamChunker chunker;
    chunker.append(payload);
    const QList<StreamChunk> chunks = chunker.takeAvailableChunks();

    if (chunks.size() != 2) {
        std::cerr << label << ": expected exactly 2 chunks, got " << chunks.size() << "\n";
        return false;
    }
    if (chunks.at(0).kind != StreamChunkKind::Binary || chunks.at(0).payload != binaryPart) {
        std::cerr << label << ": first chunk mismatch\n";
        return false;
    }
    if (chunks.at(1).kind == StreamChunkKind::Binary || chunks.at(1).payload != textPart) {
        std::cerr << label << ": second chunk mismatch\n";
        return false;
    }
    return true;
}

}

int main() {
    const QByteArray printableLineWithNumericPrefix =
        "001591857A l#@!7LD'\"pk&!qY'^W\"pb#!qY(1\"qY'g\\#7:1tqY0j[rUfs[q=k.\"qtC7#\"pbGBr:V,g!T3r]!2*0l6%O(Z!5QYd!!!F`!!)-+rrN0!!!**!!<E3\"!!<6%!!*#u!!*&u!WN\n";

    const QByteArray printableLineWithEmbeddedDollar =
        "002565447A q#@!{!!!'#gY`+j!!*(2&-<$#\"#-qZ)#suJU&YM)U&Y/nzz!(fXO;GC4Y;FOqa6m-YaE\\8Im1GppJ3$9ap7VQ[M3Zr!TG;W`<ATf_:DfTDBATN&KBl7EsF_#&:|Uj7^G%G]\n";

    const QByteArray shortPrintableLineWithNumericPrefix =
        "002792103A 5#@!{!<`}\n";

    const QByteArray prefixedNumericLinePayload =
        "D002794160A N#@!{i26}\n";
    const QByteArray prefixedNumericLineText =
        "002794160A N#@!{i26}\n";

    const QByteArray binaryBreamFrame =
        QByteArray::fromHex("BCB201E406090000E80300003200000000000000FFFFFFFFFFFFFFFFFFFFFFFF86AF");

    if (!expectSingleTextChunk("numeric-prefix", printableLineWithNumericPrefix)) {
        return EXIT_FAILURE;
    }
    if (!expectSingleTextChunk("embedded-dollar", printableLineWithEmbeddedDollar)) {
        return EXIT_FAILURE;
    }
    if (!expectSingleTextChunk("short-numeric-prefix", shortPrintableLineWithNumericPrefix)) {
        return EXIT_FAILURE;
    }
    if (!expectBinaryThenText("prefixed-numeric-line",
                              prefixedNumericLinePayload,
                              QByteArray("D"),
                              prefixedNumericLineText)) {
        return EXIT_FAILURE;
    }
    if (!expectSingleBinaryChunk("bream-binary", binaryBreamFrame)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
