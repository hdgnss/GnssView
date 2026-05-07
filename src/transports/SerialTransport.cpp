#include "SerialTransport.h"

#include <QSerialPortInfo>

namespace hdgnss {

namespace {
QSerialPort::DataBits dataBitsFromInt(int value) {
    switch (value) {
    case 5: return QSerialPort::Data5;
    case 6: return QSerialPort::Data6;
    case 7: return QSerialPort::Data7;
    default: return QSerialPort::Data8;
    }
}

QSerialPort::StopBits stopBitsFromInt(int value) {
    return value == 2 ? QSerialPort::TwoStop : QSerialPort::OneStop;
}

QSerialPort::Parity parityFromString(const QString &value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == "even") return QSerialPort::EvenParity;
    if (normalized == "odd") return QSerialPort::OddParity;
    if (normalized == "mark") return QSerialPort::MarkParity;
    if (normalized == "space") return QSerialPort::SpaceParity;
    return QSerialPort::NoParity;
}

QSerialPort::FlowControl flowControlFromString(const QString &value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == "hardware" || normalized == "rts/cts" || normalized == "rtscts") return QSerialPort::HardwareControl;
    if (normalized == "software" || normalized == "xon/xoff" || normalized == "xonxoff") return QSerialPort::SoftwareControl;
    return QSerialPort::NoFlowControl;
}
}  // namespace

SerialTransport::SerialTransport(QObject *parent)
    : ITransport(QStringLiteral("UART"), parent) {
    connect(&m_serial, &QSerialPort::readyRead, this, [this]() {
        emit dataReceived(m_serial.readAll());
    });
    connect(&m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError) {
            return;
        }
        const QString message = m_serial.errorString();
        setStatus(message);
        emit errorOccurred(message);
    });
}

bool SerialTransport::openWithSettings(const QVariantMap &settings) {
    close();

    const QString portName = settings.value(QStringLiteral("port")).toString().trimmed();
    if (portName.isEmpty()) {
        const QString message = QStringLiteral("UART open failed: no serial port selected");
        setStatus(message);
        emit errorOccurred(message);
        emitOpenChanged();
        return false;
    }

    m_serial.setPortName(portName);
    m_serial.setBaudRate(settings.value(QStringLiteral("baud"), 115200).toInt());
    m_serial.setDataBits(dataBitsFromInt(settings.value(QStringLiteral("dataBits"), 8).toInt()));
    m_serial.setStopBits(stopBitsFromInt(settings.value(QStringLiteral("stopBits"), 1).toInt()));
    m_serial.setParity(parityFromString(settings.value(QStringLiteral("parity"), QStringLiteral("None")).toString()));
    m_serial.setFlowControl(flowControlFromString(settings.value(QStringLiteral("flowControl"), QStringLiteral("None")).toString()));

    if (!m_serial.open(QIODevice::ReadWrite)) {
        const QString message = QStringLiteral("UART open failed: %1").arg(m_serial.errorString());
        setStatus(message);
        emit errorOccurred(message);
        emitOpenChanged();
        return false;
    }

    setStatus(QStringLiteral("Connected %1 @ %2").arg(m_serial.portName()).arg(m_serial.baudRate()));
    emitOpenChanged();
    return true;
}

void SerialTransport::close() {
    if (m_serial.isOpen()) {
        m_serial.close();
    }
    setStatus(QStringLiteral("Closed"));
    emitOpenChanged();
}

bool SerialTransport::isOpen() const {
    return m_serial.isOpen();
}

bool SerialTransport::sendData(const QByteArray &payload) {
    if (!m_serial.isOpen()) {
        setStatus(QStringLiteral("UART not connected"));
        return false;
    }
    return m_serial.write(payload) == payload.size();
}

QStringList SerialTransport::capabilities() const {
    return {
        QStringLiteral("rx"),
        QStringLiteral("tx"),
        QStringLiteral("serial"),
        QStringLiteral("baud-config")
    };
}

QString SerialTransport::sessionQualifier() const {
    return m_serial.portName();
}

QString SerialTransport::portName() const {
    return m_serial.portName();
}

}  // namespace hdgnss
