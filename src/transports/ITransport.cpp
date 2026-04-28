#include "hdgnss/ITransport.h"

namespace hdgnss {

ITransport::ITransport(QString transportName, QObject *parent)
    : QObject(parent), m_name(std::move(transportName)) {}

QString ITransport::name() const {
    return m_name;
}

QString ITransport::status() const {
    return m_status;
}

QString ITransport::sessionBaseName() const {
    return m_name.trimmed().toLower();
}

QString ITransport::sessionQualifier() const {
    return {};
}

void ITransport::setStatus(const QString &statusText) {
    if (m_status == statusText) {
        return;
    }
    m_status = statusText;
    emit statusChanged();
}

void ITransport::emitOpenChanged() {
    emit openChanged();
}

}  // namespace hdgnss
