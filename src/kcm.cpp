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

#include "kcm.h"
#include "config-kcm.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QStandardPaths>
#include <QtGlobal>
#include <QProcess>

#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>

#include <KLocalizedString>

#include <kauthaction.h>
#include <kauthexecutejob.h>

#include <KNewStuff3/KNSCore/Engine>
#include <KNewStuff3/KNSCore/EntryInternal>

#include <kio/job.h>
#include <KIO/CopyJob>

K_PLUGIN_FACTORY_WITH_JSON(KCMPlymouthFactory, "kcm_plymouth.json", registerPlugin<KCMPlymouth>();)

KCMPlymouth::KCMPlymouth(QObject* parent, const QVariantList& args)
    : KQuickAddons::ConfigModule(parent, args)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,14,0))
    qmlRegisterType<QStandardItemModel>();
    qmlRegisterType<KCMPlymouth>();
#else
    qmlRegisterAnonymousType<QStandardItemModel>("KCMPlymouth", 1);
    qmlRegisterAnonymousType<KCMPlymouth>("KCMPlymouth", 1);
#endif
    KAboutData* about = new KAboutData(QStringLiteral("kcm_plymouth"), i18n("Boot Splash Screen"),
                                       QStringLiteral(PROJECT_VERSION), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"));
    setAboutData(about);
    setButtons(Apply);
    setAuthActionName(QStringLiteral("org.kde.kcontrol.kcmplymouth.save"));
    setNeedsAuthorization(true);

    m_model = new QStandardItemModel(this);

    m_model->setItemRoleNames({
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {DescriptionRole, QByteArrayLiteral("description")},
        {PluginNameRole, QByteArrayLiteral("pluginName")},
        {ScreenhotRole, QByteArrayLiteral("screenshot")},
        {UninstallableRole, QByteArrayLiteral("uninstallable")}
    });
}

KCMPlymouth::~KCMPlymouth()
{
    delete m_newStuffDialog.data();
}

void KCMPlymouth::reloadModel()
{
    m_model->clear();

    QDir dir(QStringLiteral(PLYMOUTH_THEMES_DIR));
    if (!dir.exists()) {
        return;
    }

    KConfigGroup installedCg(KSharedConfig::openConfig(QStringLiteral("kplymouththemeinstallerrc")), "DownloadedThemes");

    dir.setFilter(QDir::NoDotAndDotDot|QDir::Dirs);

    const auto list = dir.entryInfoList();
    for (const QFileInfo &fileInfo : list) {
        const QString pluginName = fileInfo.fileName();
        QDir themeDir(fileInfo.filePath());

        KConfig file(themeDir.filePath(pluginName + QLatin1String(".plymouth")), KConfig::SimpleConfig);
        KConfigGroup grp = file.group("Plymouth Theme");

        QString displayName = grp.readEntry("Name", QString());
        if (displayName.isEmpty()) {
            displayName = pluginName;
        }

        QStandardItem *row = new QStandardItem(displayName);
        row->setData(pluginName, PluginNameRole);
        row->setData(grp.readEntry("Description", QString()), DescriptionRole);
        row->setData(installedCg.entryMap().contains(fileInfo.fileName()), UninstallableRole);

        //the theme has a preview
        if (QFile::exists(themeDir.path() + QStringLiteral("/preview.png"))) {
            row->setData(QString(themeDir.path() + QStringLiteral("/preview.png")), ScreenhotRole);
        //fetch it downloaded from kns
        } else {
            const QString fileName = installedCg.readEntry(fileInfo.fileName(), QString());
            if (fileName.isEmpty()) {
                row->setData(QString(), ScreenhotRole);
            } else {
                row->setData(fileName + QStringLiteral(".png"), ScreenhotRole);
            }
        }

        m_model->appendRow(row);
    }

    emit selectedPluginIndexChanged();
}

void KCMPlymouth::getNewStuff(QQuickItem *ctx)
{
    if (!m_newStuffDialog) {
        m_newStuffDialog = new KNS3::DownloadDialog( QLatin1String("plymouth.knsrc") );
        m_newStuffDialog->setWindowTitle(i18n("Download New Boot Splash Screens"));
        m_newStuffDialog->setWindowModality(Qt::WindowModal);
        m_newStuffDialog->winId(); // so it creates the windowHandle();
        connect(m_newStuffDialog.data(), &KNS3::DownloadDialog::accepted, this, &KCMPlymouth::reloadModel);
        connect(m_newStuffDialog.data(), &KNS3::DownloadDialog::finished, m_newStuffDialog.data(),  &KNS3::DownloadDialog::deleteLater);

        connect(m_newStuffDialog->engine(), &KNSCore::Engine::signalEntryChanged, this, [=](const KNSCore::EntryInternal &entry){
            if (!entry.isValid() || entry.status() != KNS3::Entry::Installed) {
                return;
            }

            KIO::file_copy(QUrl(entry.previewUrl(KNSCore::EntryInternal::PreviewBig1)), QUrl::fromLocalFile(QString(entry.installedFiles().constFirst() + QStringLiteral(".png"))), -1,  KIO::Overwrite | KIO::HideProgressInfo);
        });
    }

    if (ctx && ctx->window()) {
        m_newStuffDialog->windowHandle()->setTransientParent(ctx->window());
    }

    m_newStuffDialog->show();
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

    m_selectedPlugin = plugin;
    emit selectedPluginChanged();
    emit selectedPluginIndexChanged();

    setNeedsSave(true);
}

bool KCMPlymouth::busy() const
{
    return m_busy;
}

void KCMPlymouth::setBusy(const bool &busy)
{
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged();
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
    reloadModel();

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral(PLYMOUTH_CONFIG_PATH)), "Daemon");

    setSelectedPlugin(cg.readEntry("Theme"));

    setNeedsSave(false);
}


void KCMPlymouth::save()
{
    setBusy(true);
    QVariantMap helperargs;
    helperargs[QStringLiteral("theme")] = m_selectedPlugin;

    KAuth::Action action(authActionName());
    action.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplymouth"));
    action.setArguments(helperargs);
    action.setTimeout(60000);

    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        if (job->error() == KAuth::ActionReply::UserCancelledError) {
            emit showErrorMessage(i18n("Unable to authenticate/execute the action: %1 (%2)", job->error(), job->errorString()));
        }
        load();
    }
    setBusy(false);
}

void KCMPlymouth::uninstall(const QString &plugin)
{
    QVariantMap helperargs;
    helperargs[QStringLiteral("theme")] = plugin;

    //KAuth::Action action(authActionName());
    KAuth::Action action(QStringLiteral("org.kde.kcontrol.kcmplymouth.uninstall"));
    action.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplymouth"));
    action.setArguments(helperargs);

    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        emit showErrorMessage(i18n("Unable to authenticate/execute the action: %1 (%2)", job->error(), job->errorString()));
    } else {
        KConfigGroup installedCg(KSharedConfig::openConfig(QStringLiteral("kplymouththemeinstallerrc")), "DownloadedThemes");
        installedCg.deleteEntry(plugin);
        emit showSuccessMessage(i18n("Theme uninstalled successfully."));
        load();
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
