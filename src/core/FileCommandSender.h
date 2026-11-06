#pragma once

#include <functional>
#include <optional>

#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

namespace hdgnss {

class IProtocolPlugin;
class TransportViewModel;

class FileCommandSender : public QObject {
    Q_OBJECT

public:
    using DecoderLookupFn = std::function<IProtocolPlugin *(const QString &protocolName)>;

    explicit FileCommandSender(TransportViewModel *transportViewModel,
                               DecoderLookupFn decoderLookup,
                               QObject *parent = nullptr);
    ~FileCommandSender() override;

    bool sendFile(const QString &filePath,
                  const QString &decoderName,
                  const QString &target,
                  int packetIntervalMs);
    void stop();

signals:
    void statusMessage(const QString &message);
    void packetTransmitted(const QString &transportName, const QByteArray &packet);

private:
    struct FileSendJob {
        QString filePath;
        QString decoderName;
        QString resolvedTarget;
        QList<QByteArray> packets;
        int packetIntervalMs = 0;
        int nextPacketIndex = 0;
    };

    bool queueFileBytesForSend(const QByteArray &fileBytes,
                               const QString &sourceLabel,
                               const QString &decoderName,
                               const QString &target,
                               int packetIntervalMs);
    bool enqueuePacketJob(QList<QByteArray> packets,
                          const QString &sourceLabel,
                          const QString &decoderName,
                          const QString &target,
                          int packetIntervalMs);
    void downloadFileCommand(const QUrl &url,
                             const QString &decoderName,
                             const QString &target,
                             int packetIntervalMs);
    void handleFileDownloadFinished(QNetworkReply *reply,
                                    const QUrl &sourceUrl,
                                    const QString &decoderName,
                                    const QString &target,
                                    int packetIntervalMs);
    void startNextFileSendJob();
    void sendNextFilePacket();

    TransportViewModel *m_transportViewModel = nullptr;
    DecoderLookupFn m_decoderLookup;
    QList<FileSendJob> m_fileSendQueue;
    std::optional<FileSendJob> m_activeFileSendJob;
    QNetworkAccessManager m_fileDownloadManager;
    QTimer m_fileSendTimer;
    quint64 m_generation = 0;
};

}  // namespace hdgnss
