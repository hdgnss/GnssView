#include "FileCommandSender.h"

#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QThreadPool>

#include "src/core/FilePacketizer.h"
#include "src/core/TransportViewModel.h"
#include "src/protocols/IProtocolPlugin.h"

namespace hdgnss {

namespace {

constexpr qint64 kAsyncFileLoadThresholdBytes = 64 * 1024;

bool hasHttpFileScheme(const QString &payload) {
    const QString trimmed = payload.trimmed();
    return trimmed.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        || trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

bool parseHttpFileUrl(const QString &payload, QUrl *urlOut) {
    const QString trimmed = payload.trimmed();
    const QUrl url(trimmed);
    if (!url.isValid() || url.host().isEmpty()) {
        return false;
    }
    if (urlOut) {
        *urlOut = url;
    }
    return true;
}

}  // namespace

FileCommandSender::FileCommandSender(TransportViewModel *transportViewModel,
                                     DecoderLookupFn decoderLookup,
                                     QObject *parent)
    : QObject(parent)
    , m_transportViewModel(transportViewModel)
    , m_decoderLookup(std::move(decoderLookup)) {
    m_fileSendTimer.setSingleShot(true);
    connect(&m_fileSendTimer, &QTimer::timeout, this, &FileCommandSender::sendNextFilePacket);
}

FileCommandSender::~FileCommandSender() {
    stop();
}

void FileCommandSender::stop() {
    ++m_generation;
    m_fileSendTimer.stop();
    m_activeFileSendJob.reset();
    m_fileSendQueue.clear();
}

bool FileCommandSender::sendFile(const QString &filePath,
                                 const QString &decoderName,
                                 const QString &target,
                                 int packetIntervalMs) {
    const QString source = filePath.trimmed();
    QUrl fileUrl;
    if (hasHttpFileScheme(source)) {
        if (!parseHttpFileUrl(source, &fileUrl)) {
            emit statusMessage(QStringLiteral("Invalid file URL: %1").arg(source));
            return false;
        }
        downloadFileCommand(fileUrl, decoderName, target, packetIntervalMs);
        return true;
    }

    const QFileInfo fileInfo(source);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        emit statusMessage(QStringLiteral("Failed to open file: %1").arg(source));
        return false;
    }

    if (fileInfo.size() > kAsyncFileLoadThresholdBytes) {
        const QString trimmedDecoder = decoderName.trimmed();
        const bool useDefaultPacketizer = trimmedDecoder.isEmpty()
            || trimmedDecoder.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0;
        const quint64 currentGen = m_generation;
        QPointer<FileCommandSender> weakSelf(this);
        QThreadPool::globalInstance()->start([weakSelf, currentGen, source, decoderName, target, packetIntervalMs, useDefaultPacketizer]() {
            QFile asyncFile(source);
            if (!asyncFile.open(QIODevice::ReadOnly)) {
                if (weakSelf) {
                    QMetaObject::invokeMethod(weakSelf.data(), [weakSelf, currentGen, source]() {
                        if (weakSelf && weakSelf->m_generation == currentGen) {
                            emit weakSelf->statusMessage(QStringLiteral("Failed to open file: %1").arg(source));
                        }
                    }, Qt::QueuedConnection);
                }
                return;
            }
            const QByteArray fileBytes = asyncFile.readAll();
            if (useDefaultPacketizer) {
                QString packetizeError;
                const QList<QByteArray> packets = FilePacketizer::packetize(fileBytes, &packetizeError);
                const bool fileEmpty = fileBytes.isEmpty();
                if (weakSelf) {
                    QMetaObject::invokeMethod(weakSelf.data(),
                        [weakSelf, currentGen, source, decoderName, target, packetIntervalMs,
                         fileEmpty, packets, packetizeError]() {
                            if (!weakSelf || weakSelf->m_generation != currentGen) {
                                return;
                            }
                            if (fileEmpty) {
                                emit weakSelf->statusMessage(QStringLiteral("File is empty: %1").arg(source));
                                return;
                            }
                            if (packets.isEmpty()) {
                                emit weakSelf->statusMessage(
                                    QStringLiteral("Failed to packetize file %1: %2")
                                        .arg(source,
                                             packetizeError.isEmpty() ? QStringLiteral("No payload to send") : packetizeError));
                                return;
                            }
                            weakSelf->enqueuePacketJob(packets, source, decoderName, target, packetIntervalMs);
                        },
                        Qt::QueuedConnection);
                }
                return;
            }
            if (weakSelf) {
                QMetaObject::invokeMethod(weakSelf.data(),
                    [weakSelf, currentGen, fileBytes, source, decoderName, target, packetIntervalMs]() {
                        if (weakSelf && weakSelf->m_generation == currentGen) {
                            weakSelf->queueFileBytesForSend(fileBytes, source, decoderName, target, packetIntervalMs);
                        }
                    },
                    Qt::QueuedConnection);
            }
        });
        return true;
    }

    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        emit statusMessage(QStringLiteral("Failed to open file: %1").arg(source));
        return false;
    }

    const QByteArray fileBytes = file.readAll();
    return queueFileBytesForSend(fileBytes, source, decoderName, target, packetIntervalMs);
}

bool FileCommandSender::queueFileBytesForSend(const QByteArray &fileBytes,
                                              const QString &sourceLabel,
                                              const QString &decoderName,
                                              const QString &target,
                                              int packetIntervalMs) {
    if (fileBytes.isEmpty()) {
        emit statusMessage(QStringLiteral("File is empty: %1").arg(sourceLabel));
        return false;
    }

    QList<QByteArray> packets;
    const QString trimmedDecoder = decoderName.trimmed();
    if (!trimmedDecoder.isEmpty() && trimmedDecoder.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0) {
        IProtocolPlugin *decoderPlugin = m_decoderLookup ? m_decoderLookup(trimmedDecoder) : nullptr;
        if (!decoderPlugin) {
            emit statusMessage(QStringLiteral("File decoder not available: %1").arg(trimmedDecoder));
            return false;
        }
        QString packetizeError;
        packets = decoderPlugin->packetizeFile(fileBytes, &packetizeError);
        if (packets.isEmpty()) {
            emit statusMessage(QStringLiteral("Failed to packetize file %1: %2")
                                   .arg(sourceLabel,
                                        packetizeError.isEmpty() ? QStringLiteral("No payload to send") : packetizeError));
            return false;
        }
    } else {
        QString packetizeError;
        packets = FilePacketizer::packetize(fileBytes, &packetizeError);
        if (packets.isEmpty()) {
            emit statusMessage(QStringLiteral("Failed to packetize file %1: %2")
                                   .arg(sourceLabel,
                                        packetizeError.isEmpty() ? QStringLiteral("No payload to send") : packetizeError));
            return false;
        }
    }

    return enqueuePacketJob(std::move(packets), sourceLabel, decoderName, target, packetIntervalMs);
}

bool FileCommandSender::enqueuePacketJob(QList<QByteArray> packets,
                                         const QString &sourceLabel,
                                         const QString &decoderName,
                                         const QString &target,
                                         int packetIntervalMs) {
    const QString resolvedTarget = (target == QStringLiteral("Active") || target == QStringLiteral("Default"))
        ? (m_transportViewModel ? m_transportViewModel->activeTransport() : target)
        : target;

    FileSendJob job;
    job.filePath = sourceLabel;
    job.decoderName = decoderName;
    job.resolvedTarget = resolvedTarget;
    job.packets = std::move(packets);
    job.packetIntervalMs = qMax(0, packetIntervalMs);

    const bool alreadyBusy = m_activeFileSendJob.has_value() || !m_fileSendQueue.isEmpty() || m_fileSendTimer.isActive();
    const int packetCount = job.packets.size();
    m_fileSendQueue.append(std::move(job));

    if (alreadyBusy) {
        emit statusMessage(QStringLiteral("Queued file send: %1 (%2 packet(s), %3 ms interval)")
                               .arg(sourceLabel)
                               .arg(packetCount)
                               .arg(qMax(0, packetIntervalMs)));
    } else {
        startNextFileSendJob();
    }
    return true;
}

void FileCommandSender::downloadFileCommand(const QUrl &url,
                                            const QString &decoderName,
                                            const QString &target,
                                            int packetIntervalMs) {
    const QString sourceLabel = url.toDisplayString();
    emit statusMessage(QStringLiteral("Downloading file: %1").arg(sourceLabel));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "GnssView");
    request.setTransferTimeout(30000);

    const quint64 currentGen = m_generation;
    QNetworkReply *reply = m_fileDownloadManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, currentGen, reply, url, decoderName, target, packetIntervalMs]() {
        if (m_generation != currentGen) {
            reply->deleteLater();
            return;
        }
        handleFileDownloadFinished(reply, url, decoderName, target, packetIntervalMs);
    });
}

void FileCommandSender::handleFileDownloadFinished(QNetworkReply *reply,
                                                   const QUrl &sourceUrl,
                                                   const QString &decoderName,
                                                   const QString &target,
                                                   int packetIntervalMs) {
    reply->deleteLater();

    const QString sourceLabel = sourceUrl.toDisplayString();
    if (reply->error() != QNetworkReply::NoError) {
        emit statusMessage(QStringLiteral("Failed to download file %1: %2")
                               .arg(sourceLabel, reply->errorString()));
        return;
    }

    const QByteArray fileBytes = reply->readAll();
    emit statusMessage(QStringLiteral("Downloaded file: %1 (%2 bytes)")
                           .arg(sourceLabel)
                           .arg(fileBytes.size()));
    queueFileBytesForSend(fileBytes, sourceLabel, decoderName, target, packetIntervalMs);
}

void FileCommandSender::startNextFileSendJob() {
    if (m_activeFileSendJob.has_value() || m_fileSendQueue.isEmpty()) {
        return;
    }

    m_activeFileSendJob = m_fileSendQueue.takeFirst();
    const FileSendJob &job = *m_activeFileSendJob;
    emit statusMessage(QStringLiteral("Sending %1 packet(s) from %2 via %3 with %4 ms interval")
                           .arg(job.packets.size())
                           .arg(job.filePath)
                           .arg(job.resolvedTarget)
                           .arg(job.packetIntervalMs));
    sendNextFilePacket();
}

void FileCommandSender::sendNextFilePacket() {
    if (!m_activeFileSendJob.has_value()) {
        startNextFileSendJob();
        return;
    }

    FileSendJob &job = *m_activeFileSendJob;
    if (job.nextPacketIndex >= job.packets.size()) {
        emit statusMessage(QStringLiteral("Sent %1 packet(s) from %2 via %3")
                               .arg(job.packets.size())
                               .arg(job.filePath)
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    const QByteArray &packet = job.packets.at(job.nextPacketIndex);
    if (!m_transportViewModel || !m_transportViewModel->send(packet, job.resolvedTarget)) {
        emit statusMessage(QStringLiteral("Failed to send file packet %1/%2 through %3")
                               .arg(job.nextPacketIndex + 1)
                               .arg(job.packets.size())
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    emit packetTransmitted(job.resolvedTarget, packet);
    ++job.nextPacketIndex;

    if (job.nextPacketIndex >= job.packets.size()) {
        emit statusMessage(QStringLiteral("Sent %1 packet(s) from %2 via %3")
                               .arg(job.packets.size())
                               .arg(job.filePath)
                               .arg(job.resolvedTarget));
        m_activeFileSendJob.reset();
        startNextFileSendJob();
        return;
    }

    m_fileSendTimer.start(job.packetIntervalMs);
}

}  // namespace hdgnss
