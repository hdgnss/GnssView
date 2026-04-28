#include "CommandButtonModel.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSet>

namespace hdgnss {

CommandButtonModel::CommandButtonModel(QObject *parent)
    : QAbstractListModel(parent) {
    const QString path = defaultStoragePath();
    if (QFile::exists(path) && loadFromFile(path)) {
        return;
    }
    setDefaults();
}

int CommandButtonModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_buttons.size();
}

QVariant CommandButtonModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_buttons.size()) {
        return {};
    }
    const CommandButtonDefinition &button = m_buttons.at(index.row());
    switch (role) {
    case NameRole: return button.name;
    case GroupRole: return button.group;
    case PayloadRole: return button.payload;
    case ContentTypeRole: return button.contentType;
    case TransportTargetRole: return button.transportTarget;
    case BuildProtocolRole: return button.buildProtocol;
    case PluginMessageRole: return button.pluginMessage;
    case FileDecoderRole: return button.fileDecoder;
    case FilePacketIntervalMsRole: return button.filePacketIntervalMs;
    default: return {};
    }
}

QHash<int, QByteArray> CommandButtonModel::roleNames() const {
    return {
        {NameRole, "name"},
        {GroupRole, "group"},
        {PayloadRole, "payload"},
        {ContentTypeRole, "contentType"},
        {TransportTargetRole, "transportTarget"},
        {BuildProtocolRole, "buildProtocol"},
        {PluginMessageRole, "pluginMessage"},
        {FileDecoderRole, "fileDecoder"},
        {FilePacketIntervalMsRole, "filePacketIntervalMs"}
    };
}

QStringList CommandButtonModel::groups() const {
    QSet<QString> groupSet;
    for (const CommandButtonDefinition &button : m_buttons) {
        groupSet.insert(button.group);
    }
    QStringList list = groupSet.values();
    std::sort(list.begin(), list.end());
    return list;
}

QVariantList CommandButtonModel::buttonsForGroup(const QString &group) const {
    QVariantList list;
    for (int i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons.at(i).group == group) {
            QVariantMap item = toVariant(m_buttons.at(i));
            item.insert(QStringLiteral("index"), i);
            list.append(item);
        }
    }
    return list;
}

QVariantMap CommandButtonModel::get(int row) const {
    if (row < 0 || row >= m_buttons.size()) {
        return {};
    }
    return toVariant(m_buttons.at(row));
}

void CommandButtonModel::addButton(const QVariantMap &definition) {
    const int row = m_buttons.size();
    beginInsertRows({}, row, row);
    m_buttons.append(fromVariant(definition));
    endInsertRows();
    emitStructureChanged();
}

void CommandButtonModel::addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget) {
    addButton(name, group, payload, contentType, transportTarget, QStringLiteral("None"), 0);
}

void CommandButtonModel::addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder) {
    addButton(name, group, payload, contentType, transportTarget, fileDecoder, 0);
}

void CommandButtonModel::addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder, int filePacketIntervalMs) {
    addButton({
        {QStringLiteral("name"), name},
        {QStringLiteral("group"), group},
        {QStringLiteral("payload"), payload},
        {QStringLiteral("contentType"), contentType},
        {QStringLiteral("transportTarget"), transportTarget},
        {QStringLiteral("buildProtocol"), QString()},
        {QStringLiteral("pluginMessage"), QString()},
        {QStringLiteral("fileDecoder"), fileDecoder},
        {QStringLiteral("filePacketIntervalMs"), qMax(0, filePacketIntervalMs)}
    });
}

void CommandButtonModel::updateButton(int row, const QVariantMap &definition) {
    if (row < 0 || row >= m_buttons.size()) {
        return;
    }
    m_buttons[row] = fromVariant(definition);
    emit dataChanged(index(row, 0), index(row, 0));
    emitStructureChanged();
}

void CommandButtonModel::updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget) {
    updateButton(rowId, name, group, payload, contentType, transportTarget, QStringLiteral("None"), 0);
}

void CommandButtonModel::updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder) {
    updateButton(rowId, name, group, payload, contentType, transportTarget, fileDecoder, 0);
}

void CommandButtonModel::updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder, int filePacketIntervalMs) {
    bool ok = false;
    const int row = rowId.toInt(&ok);
    if (!ok) {
        return;
    }
    updateButton(row, {
        {QStringLiteral("name"), name},
        {QStringLiteral("group"), group},
        {QStringLiteral("payload"), payload},
        {QStringLiteral("contentType"), contentType},
        {QStringLiteral("transportTarget"), transportTarget},
        {QStringLiteral("buildProtocol"), QString()},
        {QStringLiteral("pluginMessage"), QString()},
        {QStringLiteral("fileDecoder"), fileDecoder},
        {QStringLiteral("filePacketIntervalMs"), qMax(0, filePacketIntervalMs)}
    });
}

void CommandButtonModel::removeButton(int row) {
    if (row < 0 || row >= m_buttons.size()) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_buttons.removeAt(row);
    endRemoveRows();
    emitStructureChanged();
}

void CommandButtonModel::removeButton(const QString &rowId) {
    bool ok = false;
    const int row = rowId.toInt(&ok);
    if (ok) {
        removeButton(row);
    }
}

bool CommandButtonModel::loadFromFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return false;
    }
    QList<CommandButtonDefinition> next;
    for (const QJsonValue &value : doc.array()) {
        next.append(fromVariant(value.toObject().toVariantMap()));
    }
    beginResetModel();
    m_buttons = next;
    endResetModel();
    emitStructureChanged();
    return true;
}

bool CommandButtonModel::importJson() {
    const QString startPath = QDir(defaultDocumentsPath()).filePath(QStringLiteral("command-buttons.json"));
    const QString path = QFileDialog::getOpenFileName(nullptr,
                                                      QStringLiteral("Import Command Buttons"),
                                                      startPath,
                                                      QStringLiteral("JSON Files (*.json);;All Files (*)"));
    if (path.isEmpty()) {
        return false;
    }
    return loadFromFile(path);
}

bool CommandButtonModel::saveToFile(const QString &path) const {
    const QFileInfo info(path);
    if (info.dir().exists() || QDir().mkpath(info.dir().absolutePath())) {
        // directory ready
    } else {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QJsonArray array;
    for (const CommandButtonDefinition &button : m_buttons) {
        array.append(QJsonObject::fromVariantMap(toExportVariant(button)));
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

bool CommandButtonModel::exportJson() const {
    const QString startPath = QDir(defaultDocumentsPath()).filePath(QStringLiteral("command-buttons.json"));
    const QString path = QFileDialog::getSaveFileName(nullptr,
                                                      QStringLiteral("Export Command Buttons"),
                                                      startPath,
                                                      QStringLiteral("JSON Files (*.json);;All Files (*)"));
    if (path.isEmpty()) {
        return false;
    }
    return saveToFile(path);
}

void CommandButtonModel::importTemplates(const QList<CommandTemplate> &templates) {
    for (const CommandTemplate &templ : templates) {
        addButton({
            {QStringLiteral("name"), templ.name},
            {QStringLiteral("group"), templ.group},
            {QStringLiteral("payload"), templ.payload},
            {QStringLiteral("contentType"), templ.contentType},
            {QStringLiteral("transportTarget"), QStringLiteral("Default")},
            {QStringLiteral("buildProtocol"), QString()},
            {QStringLiteral("pluginMessage"), QString()},
            {QStringLiteral("fileDecoder"), QStringLiteral("None")},
            {QStringLiteral("filePacketIntervalMs"), 0}
        });
    }
}

void CommandButtonModel::setDefaults() {
    beginResetModel();
    m_buttons.clear();
    endResetModel();
    emitStructureChanged();
}

void CommandButtonModel::emitStructureChanged() {
    emit groupsChanged();
    persist();
}

QString CommandButtonModel::defaultStoragePath() const {
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(basePath);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("command-buttons.json"));
}

QString CommandButtonModel::defaultDocumentsPath() const {
    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documentsPath.isEmpty()) {
        return documentsPath;
    }
    return QFileInfo(defaultStoragePath()).absolutePath();
}

void CommandButtonModel::persist() const {
    saveToFile(defaultStoragePath());
}

CommandButtonDefinition CommandButtonModel::fromVariant(const QVariantMap &map) {
    QString contentType = map.value(QStringLiteral("contentType"), QStringLiteral("ASCII")).toString();
    if (contentType.compare(QStringLiteral("Binary Template"), Qt::CaseInsensitive) == 0) {
        contentType = QStringLiteral("HEX");
    }
    QString buildProtocol = map.value(QStringLiteral("buildProtocol")).toString();
    if (contentType.compare(QStringLiteral("Plugin"), Qt::CaseInsensitive) == 0) {
        contentType = QStringLiteral("Plugin");
    }
    return {
        map.value(QStringLiteral("name")).toString(),
        map.value(QStringLiteral("group"), QStringLiteral("General")).toString(),
        map.value(QStringLiteral("payload")).toString(),
        contentType,
        QStringLiteral("Default"),
        buildProtocol,
        map.value(QStringLiteral("pluginMessage")).toString(),
        map.value(QStringLiteral("fileDecoder"), QStringLiteral("None")).toString(),
        qMax(0, map.value(QStringLiteral("filePacketIntervalMs"), 0).toInt())
    };
}

QVariantMap CommandButtonModel::toVariant(const CommandButtonDefinition &button) {
    return {
        {QStringLiteral("name"), button.name},
        {QStringLiteral("group"), button.group},
        {QStringLiteral("payload"), button.payload},
        {QStringLiteral("contentType"), button.contentType},
        {QStringLiteral("transportTarget"), QStringLiteral("Default")},
        {QStringLiteral("buildProtocol"), button.buildProtocol},
        {QStringLiteral("pluginMessage"), button.pluginMessage},
        {QStringLiteral("fileDecoder"), button.fileDecoder},
        {QStringLiteral("filePacketIntervalMs"), button.filePacketIntervalMs}
    };
}

QVariantMap CommandButtonModel::toExportVariant(const CommandButtonDefinition &button) {
    QVariantMap map = {
        {QStringLiteral("name"), button.name},
        {QStringLiteral("group"), button.group},
        {QStringLiteral("payload"), button.payload},
        {QStringLiteral("contentType"), button.contentType}
    };

    if (button.contentType.compare(QStringLiteral("Plugin"), Qt::CaseInsensitive) == 0) {
        if (!button.buildProtocol.isEmpty()) {
            map.insert(QStringLiteral("buildProtocol"), button.buildProtocol);
        }
        if (!button.pluginMessage.isEmpty()) {
            map.insert(QStringLiteral("pluginMessage"), button.pluginMessage);
        }
    }

    if (button.contentType.compare(QStringLiteral("FILE"), Qt::CaseInsensitive) == 0) {
        map.insert(QStringLiteral("fileDecoder"), button.fileDecoder.isEmpty()
                                                ? QStringLiteral("None")
                                                : button.fileDecoder);
        map.insert(QStringLiteral("filePacketIntervalMs"), qMax(0, button.filePacketIntervalMs));
    }

    return map;
}

}  // namespace hdgnss
