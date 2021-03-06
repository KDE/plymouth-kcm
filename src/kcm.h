/*
 *  SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef _KCM_PLYMOUTH_H
#define _KCM_PLYMOUTH_H

#include <KNewStuff3/KNSCore/EntryWrapper>
#include <QDir>

#include <KQuickAddons/ConfigModule>

class QQuickItem;
class QStandardItemModel;

class KCMPlymouth : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel *themesModel READ themesModel CONSTANT)
    Q_PROPERTY(QString selectedPlugin READ selectedPlugin WRITE setSelectedPlugin NOTIFY selectedPluginChanged)
    Q_PROPERTY(int selectedPluginIndex READ selectedPluginIndex NOTIFY selectedPluginIndexChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)

public:
    enum Roles {
        DescriptionRole = Qt::UserRole + 1,
        PluginNameRole,
        ScreenhotRole,
        UninstallableRole,
    };
    explicit KCMPlymouth(QObject *parent, const QVariantList &args);
    ~KCMPlymouth() Q_DECL_OVERRIDE;

    QStandardItemModel *themesModel();

    QString selectedPlugin() const;
    void setSelectedPlugin(const QString &plugin);

    int selectedPluginIndex() const;

    bool busy() const;
    void setBusy(const bool &busy);

    Q_INVOKABLE void reloadModel();
    Q_INVOKABLE void onChangedEntriesChanged(const QQmlListReference &changedEntries);
    Q_INVOKABLE void uninstall(const QString &plugin);

public Q_SLOTS:
    void load() Q_DECL_OVERRIDE;
    void save() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void selectedPluginChanged();
    void selectedPluginIndexChanged();

    void busyChanged();

    void showSuccessMessage(const QString &message);
    void showErrorMessage(const QString &message);

private:
    QStandardItemModel *m_model;
    QString m_selectedPlugin;
    bool m_busy = false;
};

#endif
