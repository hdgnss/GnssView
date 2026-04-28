#pragma once

#include <QObject>
#include <QVariantMap>

namespace hdgnss {

class ITransport : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool open READ isOpen NOTIFY openChanged)

public:
    explicit ITransport(QString transportName, QObject *parent = nullptr);
    ~ITransport() override = default;

    virtual bool openWithSettings(const QVariantMap &settings) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool sendData(const QByteArray &payload) = 0;
    virtual QStringList capabilities() const = 0;

    QString name() const;
    QString status() const;
    virtual QString sessionBaseName() const;
    virtual QString sessionQualifier() const;

signals:
    void dataReceived(const QByteArray &payload);
    void errorOccurred(const QString &message);
    void statusChanged();
    void openChanged();

protected:
    void setStatus(const QString &statusText);
    void emitOpenChanged();

private:
    QString m_name;
    QString m_status;
};

}  // namespace hdgnss
