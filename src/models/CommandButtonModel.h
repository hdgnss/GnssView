#pragma once

#include <QAbstractListModel>
#include <QStringList>

#include "src/protocols/GnssTypes.h"

namespace hdgnss {

struct CommandButtonDefinition {
    QString name;
    QString group;
    QString payload;
    QString contentType;
    QString transportTarget;
    QString buildProtocol;
    QString pluginMessage;
    QString fileDecoder;
    int filePacketIntervalMs = 0;
};

class CommandButtonModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        GroupRole,
        PayloadRole,
        ContentTypeRole,
        TransportTargetRole,
        BuildProtocolRole,
        PluginMessageRole,
        FileDecoderRole,
        FilePacketIntervalMsRole
    };

    explicit CommandButtonModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList groups() const;
    Q_INVOKABLE QVariantList buttonsForGroup(const QString &group) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void addButton(const QVariantMap &definition);
    Q_INVOKABLE void addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget);
    Q_INVOKABLE void addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder);
    Q_INVOKABLE void addButton(const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder, int filePacketIntervalMs);
    Q_INVOKABLE void updateButton(int row, const QVariantMap &definition);
    Q_INVOKABLE void updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget);
    Q_INVOKABLE void updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder);
    Q_INVOKABLE void updateButton(const QString &rowId, const QString &name, const QString &group, const QString &payload, const QString &contentType, const QString &transportTarget, const QString &fileDecoder, int filePacketIntervalMs);
    Q_INVOKABLE void removeButton(int row);
    Q_INVOKABLE void removeButton(const QString &rowId);
    Q_INVOKABLE bool loadFromFile(const QString &path);
    Q_INVOKABLE bool saveToFile(const QString &path) const;
    Q_INVOKABLE bool importJson();
    Q_INVOKABLE bool exportJson() const;
    void importTemplates(const QList<CommandTemplate> &templates);

signals:
    void groupsChanged();

private:
    void setDefaults();
    void emitStructureChanged();
    QString defaultStoragePath() const;
    QString defaultDocumentsPath() const;
    void persist() const;
    static CommandButtonDefinition fromVariant(const QVariantMap &map);
    static QVariantMap toVariant(const CommandButtonDefinition &button);
    static QVariantMap toExportVariant(const CommandButtonDefinition &button);

    QList<CommandButtonDefinition> m_buttons;
};

}  // namespace hdgnss
