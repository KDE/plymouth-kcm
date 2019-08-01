/*
 *  Copyright (C) 2017 Marco Martin <mart@kde.org>
 *  Copyright (c) 2014 Vishesh Handa <me@vhanda.in>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _KCM_PLYMOUTH_H
#define _KCM_PLYMOUTH_H

#include <QDir>

#include <KNewStuff3/KNS3/DownloadDialog>

#include <KQuickAddons/ConfigModule>

class QQuickView;
class QStandardItemModel;

class KCMPlymouth : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel *themesModel READ themesModel CONSTANT)
    Q_PROPERTY(QString selectedPlugin READ selectedPlugin WRITE setSelectedPlugin NOTIFY selectedPluginChanged)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole +1,
        ScreenhotRole,
        UninstallableRole
    };
    explicit KCMPlymouth(QObject* parent, const QVariantList& args);
    ~KCMPlymouth() Q_DECL_OVERRIDE;

    QStandardItemModel *themesModel();

    QString selectedPlugin() const;
    void setSelectedPlugin(const QString &plugin);

    Q_INVOKABLE int selectedPluginIndex() const;
    Q_INVOKABLE void getNewStuff();
    Q_INVOKABLE void uninstall(const QString &plugin);

public Q_SLOTS:
    void load() Q_DECL_OVERRIDE;
    void save() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void selectedPluginChanged();

private:
    QStandardItemModel *m_model;
    QString m_selectedPlugin;
    QPointer<KNS3::DownloadDialog> m_newStuffDialog;
};

#endif
