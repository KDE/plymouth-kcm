/* This file is part of the KDE Project
   Copyright (c) 2016 Marco Martin <mart@kde.org>
   Copyright (c) 2014 Vishesh Handa <me@vhanda.in>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcm.h"
#include "config-kcm.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>
#include <QQuickView>

#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>
#include <QStandardItemModel>

#include <KLocalizedString>
#include <KMessageBox>

#include <kauthaction.h>
#include <kauthexecutejob.h>

K_PLUGIN_FACTORY_WITH_JSON(KCMPlymouthFactory, "kcm_plymouth.json", registerPlugin<KCMPlymouth>();)

KCMPlymouth::KCMPlymouth(QObject* parent, const QVariantList& args)
    : KQuickAddons::ConfigModule(parent, args)
{
    //This flag seems to be needed in order for QQuickWidget to work
    //see https://bugreports.qt-project.org/browse/QTBUG-40765
    //also, it seems to work only if set in the kcm, not in the systemsettings' main
    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    qmlRegisterType<QStandardItemModel>();
    qmlRegisterType<KCMPlymouth>();
    KAboutData* about = new KAboutData(QStringLiteral("kcm_plymouth"), i18n("Configure Plymouth splash screen"),
                                       QStringLiteral("0.1"), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"));
    setAboutData(about);
    setButtons(Apply | Default);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[ScreenhotRole] = "screenshot";
    m_model->setItemRoleNames(roles);

    //setAuthActionName("org.kde.kcontrol.kcmplymouth.save");
    //setNeedsAuthorization(true);
}

KCMPlymouth::~KCMPlymouth()
{
}

void KCMPlymouth::getNewStuff()
{
    if (!m_newStuffDialog) {
        m_newStuffDialog = new KNS3::DownloadDialog( QLatin1String("plymouth.knsrc") );
        m_newStuffDialog.data()->setWindowTitle(i18n("Download New Splash Screens"));
        connect(m_newStuffDialog.data(), &KNS3::DownloadDialog::accepted, this,  &KCMPlymouth::load);
    }
    m_newStuffDialog.data()->show();
}

QStandardItemModel *KCMPlymouth::themesModel()
{
    return m_model;
}

QString KCMPlymouth::selectedPlugin() const
{
    return m_selectedPlugin;
}

void KCMPlymouth::setSelectedPlugin(const QString &plugin)
{
    if (m_selectedPlugin == plugin) {
        return;
    }

    const bool firstTime = m_selectedPlugin.isNull();
    m_selectedPlugin = plugin;
    emit selectedPluginChanged();

    if (!firstTime) {
        setNeedsSave(true);
    }
}

int KCMPlymouth::selectedPluginIndex() const
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (m_model->data(m_model->index(i, 0), PluginNameRole).toString() == m_selectedPlugin) {
            return i;
        }
    }
    return -1;
}

void KCMPlymouth::load()
{
    m_model->clear();
    QDir dir(PLYMOUTH_THEMES_DIR);
    if (!dir.exists()) {
        return;
    }

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral(PLYMOUTH_CONFIG_PATH)), "Daemon");
    m_selectedPlugin = cg.readEntry("Theme");

    dir.setFilter(QDir::NoDotAndDotDot|QDir::Dirs);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QStandardItem* row = new QStandardItem(fileInfo.fileName());
        row->setData(fileInfo.fileName(), PluginNameRole);
        QDir themeDir(fileInfo.filePath());
        //heuristically search for a logo or a background as there are no previews
        themeDir.setNameFilters(QStringList()<<QStringLiteral("*logo*.png"));
        if (!themeDir.entryInfoList().isEmpty()) {
            row->setData(themeDir.entryInfoList().first().filePath(), ScreenhotRole);
        } else {
            themeDir.setNameFilters(QStringList()<<QStringLiteral("*background*.png"));
            if (!themeDir.entryInfoList().isEmpty()) {
                row->setData(themeDir.entryInfoList().first().filePath(), ScreenhotRole);
            }
        }

        m_model->appendRow(row);
    }
}


void KCMPlymouth::save()
{
    QVariantMap helperargs;
    helperargs[QStringLiteral("theme")] = m_selectedPlugin;

    //KAuth::Action action(authActionName());
    KAuth::Action action(QStringLiteral("org.kde.kcontrol.kcmplymouth.save"));
    action.setHelperId("org.kde.kcontrol.kcmplymouth");
    action.setArguments(helperargs);
    qWarning()<<"Action: "<<action.helperId()<<action.details();
    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        KMessageBox::error(0, i18n("Unable to authenticate/execute the action: %1, %2", job->error(), job->errorString()));
    }
}

void KCMPlymouth::defaults()
{/*TODO
    if (!) {
        return;
    }
*/

}

#include "kcm.moc"
