#pragma once

#include <QSerialPort>

#include "ITransport.h"

namespace hdgnss {

class SerialTransport : public ITransport {
    Q_OBJECT

public:
    explicit SerialTransport(QObject *parent = nullptr);

    bool openWithSettings(const QVariantMap &settings) override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &payload) override;
    QStringList capabilities() const override;
    QString sessionQualifier() const override;
    QString portName() const;

private:
    QSerialPort m_serial;
};

}  // namespace hdgnss
