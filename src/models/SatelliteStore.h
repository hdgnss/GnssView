#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

#include "src/protocols/GnssTypes.h"

namespace hdgnss {

// Single source of truth for the satellites shown by the sky view and the
// signal spectrum. Updates are applied as a key-based diff (remove / change /
// append) and reported through fine-grained signals so list models can emit
// precise row notifications instead of resetting.
//
// The about-to-* / done signal pairs are emitted synchronously around each
// mutation so any number of models can wrap them in begin/end calls.
class SatelliteStore : public QObject {
    Q_OBJECT

public:
    explicit SatelliteStore(QObject *parent = nullptr);

    // Replaces the content with `satellites` (entries without a visible signal
    // are ignored). Existing rows keep their position; new rows are appended in
    // input order.
    void setSatellites(const QList<SatelliteInfo> &satellites);

    int size() const;
    const SatelliteInfo &at(int row) const;
    const QList<SatelliteInfo> &items() const;
    int revision() const;

signals:
    void aboutToInsert(int row);
    void inserted();
    void aboutToRemove(int row);
    void removed();
    void changed(int row);
    // Emitted once per setSatellites() call that modified the store.
    void revisionChanged();

private:
    QList<SatelliteInfo> m_items;
    int m_revision = 0;
};

}  // namespace hdgnss
