#pragma once

#include <QImage>
#include <QSize>

#include "src/tec/TecTypes.h"

namespace hdgnss {

class TecMapRenderer {
public:
    static QSize defaultOverlaySize();
    static QImage render(const TecGridData &dataset, const QSize &size = defaultOverlaySize());
};

}  // namespace hdgnss
