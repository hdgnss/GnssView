#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "src/protocols/GnssTypes.h"
#include "src/protocols/IProtocolPlugin.h"

namespace hdgnss {

class ProtocolInfoStore {
public:
    bool registerPanels(const QList<ProtocolInfoPanel> &panels);
    void clear();
    void clearValues();
    bool applyMessage(const ProtocolMessage &message);

    [[nodiscard]] QVariantList tabs() const;
    [[nodiscard]] QVariantMap panel(const QString &panelId) const;

private:
    struct PanelState {
        ProtocolInfoPanel definition;
        QHash<QString, QVariant> values;
        mutable QVariantMap cachedMap;
        mutable bool cacheDirty = true;
    };

    static QVariantMap buildPanelMap(const PanelState &panel);
    static QVariantList buildPanelItems(const PanelState &panel);
    static QString formatFieldValue(const ProtocolInfoField &field, const QVariant &value);

    QList<PanelState> m_panels;
    mutable QVariantList m_cachedTabs;
    mutable bool m_tabsDirty = true;
};

}  // namespace hdgnss
