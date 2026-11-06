#include "ProtocolInfoStore.h"

#include <QMetaType>

namespace hdgnss {

bool ProtocolInfoStore::registerPanels(const QList<ProtocolInfoPanel> &panels) {
    bool changed = false;
    for (const ProtocolInfoPanel &panel : panels) {
        if (panel.id.isEmpty() || panel.fields.isEmpty()) {
            continue;
        }

        bool exists = false;
        for (const PanelState &state : m_panels) {
            if (state.definition.id == panel.id) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }

        PanelState state;
        state.definition = panel;
        state.cacheDirty = true;
        m_panels.append(std::move(state));
        changed = true;
    }

    if (changed) {
        m_tabsDirty = true;
    }
    return changed;
}

void ProtocolInfoStore::clear() {
    m_panels.clear();
    m_cachedTabs.clear();
    m_tabsDirty = true;
}

void ProtocolInfoStore::clearValues() {
    for (PanelState &panel : m_panels) {
        if (!panel.values.isEmpty()) {
            panel.values.clear();
            panel.cacheDirty = true;
        }
    }
}

bool ProtocolInfoStore::applyMessage(const ProtocolMessage &message) {
    const QVariantMap &fields = message.fields;
    const auto panelTargetIt = fields.constFind(QStringLiteral("infoPanelId"));
    const QString panelTargetId = panelTargetIt == fields.cend()
        ? QString()
        : panelTargetIt.value().toString();

    bool anyPanelChanged = false;
    for (PanelState &panel : m_panels) {
        if (!panelTargetId.isEmpty() && panel.definition.id != panelTargetId) {
            continue;
        }
        if (panelTargetId.isEmpty()
            && !panel.definition.protocol.isEmpty()
            && panel.definition.protocol != message.protocol) {
            continue;
        }

        bool matchedAnyField = false;
        bool panelChanged = false;
        for (const ProtocolInfoField &field : panel.definition.fields) {
            const auto valueIt = fields.constFind(field.valueKey);
            if (valueIt == fields.cend()) {
                continue;
            }
            if (!matchedAnyField && panel.definition.replaceOnMessage && !panel.values.isEmpty()) {
                panel.values.clear();
                panelChanged = true;
            }
            matchedAnyField = true;
            const QVariant newValue = valueIt.value();
            if (panel.values.value(field.id) != newValue) {
                panel.values.insert(field.id, newValue);
                panelChanged = true;
            }
        }
        if (panelChanged) {
            panel.cacheDirty = true;
            anyPanelChanged = true;
        }
    }
    return anyPanelChanged;
}

QVariantList ProtocolInfoStore::tabs() const {
    if (m_tabsDirty) {
        QVariantList nextTabs;
        nextTabs.reserve(m_panels.size());
        for (const PanelState &panel : m_panels) {
            nextTabs.append(QVariantMap{
                {QStringLiteral("id"), panel.definition.id},
                {QStringLiteral("tabTitle"), panel.definition.tabTitle}
            });
        }
        m_cachedTabs = std::move(nextTabs);
        m_tabsDirty = false;
    }
    return m_cachedTabs;
}

QVariantMap ProtocolInfoStore::panel(const QString &panelId) const {
    for (const PanelState &state : m_panels) {
        if (state.definition.id == panelId) {
            if (state.cacheDirty) {
                state.cachedMap = buildPanelMap(state);
                state.cacheDirty = false;
            }
            return state.cachedMap;
        }
    }
    return {};
}

QVariantMap ProtocolInfoStore::buildPanelMap(const PanelState &panel) {
    return {
        {QStringLiteral("id"), panel.definition.id},
        {QStringLiteral("protocol"), panel.definition.protocol},
        {QStringLiteral("tabTitle"), panel.definition.tabTitle},
        {QStringLiteral("title"), panel.definition.title},
        {QStringLiteral("items"), buildPanelItems(panel)}
    };
}

QVariantList ProtocolInfoStore::buildPanelItems(const PanelState &panel) {
    QVariantList items;
    items.reserve(panel.definition.fields.size());
    for (const ProtocolInfoField &field : panel.definition.fields) {
        const QVariant rawValue = panel.values.value(field.id);
        items.append(QVariantMap{
            {QStringLiteral("id"), field.id},
            {QStringLiteral("label"), field.label},
            {QStringLiteral("group"), field.group},
            {QStringLiteral("semanticRole"), field.semanticRole},
            {QStringLiteral("value"), rawValue},
            {QStringLiteral("valueText"), formatFieldValue(field, rawValue)},
            {QStringLiteral("emphasize"), field.emphasize}
        });
    }
    return items;
}

QString ProtocolInfoStore::formatFieldValue(const ProtocolInfoField &field, const QVariant &value) {
    if (!value.isValid() || value.isNull()) {
        return field.fallbackText;
    }

    QString rendered;
    if (value.metaType().id() == QMetaType::Bool) {
        rendered = value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    } else if (field.precision >= 0 && value.canConvert<double>()) {
        rendered = QString::number(value.toDouble(), 'f', field.precision);
    } else {
        rendered = value.toString();
    }

    if (rendered.isEmpty()) {
        return field.fallbackText;
    }
    if (!field.unit.isEmpty()) {
        rendered += QStringLiteral(" ") + field.unit;
    }
    return rendered;
}

}  // namespace hdgnss
