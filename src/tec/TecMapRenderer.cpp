#include "TecMapRenderer.h"

#include <algorithm>
#include <cmath>

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QVector>

namespace hdgnss {

namespace {

constexpr double kWorldMinY = -91.296;
constexpr double kWorldHeight = 182.592;
constexpr double kPi = 3.14159265358979323846;
constexpr double kDefaultLonStepDegrees = 5.0;
constexpr double kDefaultLatStepDegrees = 2.5;

constexpr double kRobinsonX[] = {
    1.0, 0.9986, 0.9954, 0.99, 0.9822, 0.973, 0.96, 0.9427, 0.9216,
    0.8962, 0.8679, 0.835, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213,
    0.5722, 0.5322
};
constexpr double kRobinsonY[] = {
    0.0, 0.062, 0.124, 0.186, 0.248, 0.31, 0.372, 0.434, 0.4958,
    0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394,
    0.9761, 1.0
};

struct TecColorStop {
    double position;
    QColor color;
};

double clamp(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

QPointF robinsonProject(double lonDeg, double latDeg) {
    if (!std::isfinite(lonDeg) || !std::isfinite(latDeg)) {
        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
    }

    const double absLat = clamp(std::abs(latDeg), 0.0, 90.0);
    const int bandIndex = std::min<int>(std::size(kRobinsonX) - 2, static_cast<int>(std::floor(absLat / 5.0)));
    const double bandStart = bandIndex * 5.0;
    const double ratio = (absLat - bandStart) / 5.0;
    const double xFactor = lerp(kRobinsonX[bandIndex], kRobinsonX[bandIndex + 1], ratio);
    const double yFactor = lerp(kRobinsonY[bandIndex], kRobinsonY[bandIndex + 1], ratio);
    const double x = lonDeg * xFactor;
    const double y = (latDeg >= 0.0 ? -1.0 : 1.0) * 90.0 * 1.0144 * yFactor;
    return {x, y};
}

QPointF projectedCanvasPoint(int width, int height, double lonDeg, double latDeg) {
    const QPointF projected = robinsonProject(lonDeg, latDeg);
    if (!std::isfinite(projected.x()) || !std::isfinite(projected.y()) || width <= 0 || height <= 0) {
        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
    }

    return {
        (projected.x() + 180.0) / 360.0 * width,
        (projected.y() - kWorldMinY) / kWorldHeight * height
    };
}

double percentile(QList<double> values, double ratio) {
    if (values.isEmpty()) {
        return 0.0;
    }
    if (values.size() == 1) {
        return values.first();
    }

    std::sort(values.begin(), values.end());
    const double position = clamp(ratio, 0.0, 1.0) * static_cast<double>(values.size() - 1);
    const int lower = static_cast<int>(std::floor(position));
    const int upper = static_cast<int>(std::ceil(position));
    if (lower == upper) {
        return values.at(lower);
    }
    const double weight = position - lower;
    return values.at(lower) * (1.0 - weight) + values.at(upper) * weight;
}

QColor interpolateColor(const QColor &from, const QColor &to, double t) {
    return QColor::fromRgbF(
        lerp(from.redF(), to.redF(), t),
        lerp(from.greenF(), to.greenF(), t),
        lerp(from.blueF(), to.blueF(), t),
        lerp(from.alphaF(), to.alphaF(), t));
}

QColor tecColor(double value, double minTec, double maxTec) {
    static const QVector<TecColorStop> kStops = {
        {0.00, QColor(33, 60, 125, 192)},
        {0.18, QColor(43, 110, 204, 192)},
        {0.36, QColor(63, 198, 255, 192)},
        {0.54, QColor(119, 228, 141, 192)},
        {0.72, QColor(255, 214, 102, 192)},
        {0.86, QColor(255, 140, 66, 192)},
        {1.00, QColor(213, 38, 61, 192)}
    };

    if (maxTec - minTec < 1e-6) {
        return kStops.last().color;
    }

    const double normalized = clamp((value - minTec) / (maxTec - minTec), 0.0, 1.0);
    for (int i = 1; i < kStops.size(); ++i) {
        if (normalized <= kStops.at(i).position) {
            const TecColorStop &left = kStops.at(i - 1);
            const TecColorStop &right = kStops.at(i);
            const double section = (normalized - left.position) / (right.position - left.position);
            return interpolateColor(left.color, right.color, section);
        }
    }
    return kStops.last().color;
}

QPainterPath cellPath(int width,
                      int height,
                      double lonMin,
                      double lonMax,
                      double latMin,
                      double latMax) {
    const double stepDegrees = std::max(0.5, (lonMax - lonMin) / 4.0);
    QPainterPath path;
    bool firstPoint = true;

    auto addPoint = [&](double lon, double lat) {
        const QPointF point = projectedCanvasPoint(width, height, lon, lat);
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
            return;
        }
        if (firstPoint) {
            path.moveTo(point);
            firstPoint = false;
        } else {
            path.lineTo(point);
        }
    };

    for (double lon = lonMin; lon <= lonMax + 0.001; lon += stepDegrees) {
        addPoint(std::min(lon, lonMax), latMin);
    }
    addPoint(lonMax, latMin);
    for (double lon = lonMax; lon >= lonMin - 0.001; lon -= stepDegrees) {
        addPoint(std::max(lon, lonMin), latMax);
    }
    addPoint(lonMin, latMax);

    if (!firstPoint) {
        path.closeSubpath();
    }
    return path;
}

}  // namespace

QSize TecMapRenderer::defaultOverlaySize() {
    return QSize(964, 488);
}

QImage TecMapRenderer::render(const TecGridData &dataset, const QSize &size) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    if (!dataset.isValid() || size.isEmpty()) {
        return image;
    }

    QList<double> tecValues;
    tecValues.reserve(dataset.samples.size());
    for (const TecSample &sample : dataset.samples) {
        tecValues.append(sample.tec);
    }

    double minTec = percentile(tecValues, 0.02);
    double maxTec = percentile(tecValues, 0.98);
    if (maxTec - minTec < 1e-6) {
        minTec = dataset.minTec;
        maxTec = dataset.maxTec;
    }
    if (maxTec - minTec < 1e-6) {
        maxTec = minTec + 1.0;
    }

    const double lonHalfStep = (dataset.longitudeStepDegrees > 0.0 ? dataset.longitudeStepDegrees : kDefaultLonStepDegrees) * 0.5;
    const double latHalfStep = (dataset.latitudeStepDegrees > 0.0 ? dataset.latitudeStepDegrees : kDefaultLatStepDegrees) * 0.5;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setPen(Qt::NoPen);

    for (const TecSample &sample : dataset.samples) {
        const double lonMin = clamp(sample.longitude - lonHalfStep, -180.0, 180.0);
        const double lonMax = clamp(sample.longitude + lonHalfStep, -180.0, 180.0);
        const double latMin = clamp(sample.latitude - latHalfStep, -90.0, 90.0);
        const double latMax = clamp(sample.latitude + latHalfStep, -90.0, 90.0);
        if (lonMax <= lonMin || latMax <= latMin) {
            continue;
        }

        const QColor color = tecColor(sample.tec, minTec, maxTec);
        painter.setBrush(QBrush(color));
        painter.drawPath(cellPath(image.width(), image.height(), lonMin, lonMax, latMin, latMax));
    }

    return image;
}

}  // namespace hdgnss
