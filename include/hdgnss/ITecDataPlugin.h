#pragma once

#include <QDateTime>
#include <QtPlugin>

#include "hdgnss/TecTypes.h"

namespace hdgnss {

class ITecDataPlugin {
public:
    virtual ~ITecDataPlugin() = default;

    virtual QString sourceId() const = 0;
    virtual QString sourceName() const = 0;
    virtual qint64 downloadIntervalSeconds() const = 0;
    virtual void requestForObservationTime(const QDateTime &observationTimeUtc) = 0;
};

}  // namespace hdgnss

#define HDGNSS_TEC_DATA_PLUGIN_IID "com.hdgnss.ITecDataPlugin/1.0"
Q_DECLARE_INTERFACE(hdgnss::ITecDataPlugin, HDGNSS_TEC_DATA_PLUGIN_IID)
