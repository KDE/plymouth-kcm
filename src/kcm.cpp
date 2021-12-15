/*
 *  SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "kcm.h"
#include "config-kcm.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include <QtGlobal>

#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>

#include <KLocalizedString>

#include <kauthaction.h>
#include <kauthexecutejob.h>

#include <KNewStuff3/KNSCore/EntryInternal>

#include <KIO/CopyJob>
#include <kio/job.h>

K_PLUGIN_FACTORY_WITH_JSON(KCMPlymouthFactory, "kcm_plymouth.json", registerPlugin<KCMPlymouth>();)

KCMPlymouth::KCMPlymouth(QObject *parent, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, args)
    , m_model(new QStandardItemModel(this))
{
    qmlRegisterAnonymousType<QStandardItemModel>("KCMPlymouth", 1);
    qmlRegisterAnonymousType<KCMPlymouth>("KCMPlymouth", 1);
    KAboutData *about =
        new KAboutData(QStringLiteral("kcm_plymouth"), i18n("Boot Splash Screen"), QStringLiteral(PROJECT_VERSION), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"));
    setAboutData(about);
    setButtons(Apply);
    setAuthActionName(QStringLiteral("org.kde.kcontrol.kcmplymouth.save"));
    setNeedsAuthorization(true);

    m_model->setItemRoleNames({{Qt::DisplayRole, QByteArrayLiteral("display")},
                               {DescriptionRole, QByteArrayLiteral("description")},
                               {PluginNameRole, QByteArrayLiteral("pluginName")},
                               {ScreenhotRole, QByteArrayLiteral("screenshot")},
                               {UninstallableRole, QByteArrayLiteral("uninstallable")}});
}

KCMPlymouth::~KCMPlymouth()
{
}

void KCMPlymouth::reloadModel()
{
    m_model->clear();

    QDir dir(QStringLiteral(PLYMOUTH_THEMES_DIR));
    if (!dir.exists()) {
        return;
    }

    KConfigGroup installedCg(KSharedConfig::openConfig(QStringLiteral("kplymouththemeinstallerrc")), "DownloadedThemes");

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);

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

        // the theme has a preview
        if (QFile::exists(themeDir.path() + QStringLiteral("/preview.png"))) {
            row->setData(QString(themeDir.path() + QStringLiteral("/preview.png")), ScreenhotRole);
            // fetch it downloaded from kns
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

    Q_EMIT selectedPluginIndexChanged();
}

void KCMPlymouth::onChangedEntriesChanged(const QQmlListReference &changedEntries)
{
    static QStringList alreadyCopiedThumbnails;
    for (int i = 0; i < changedEntries.count(); ++i) {
        auto entry = qobject_cast<KNSCore::EntryWrapper *>(changedEntries.at(i))->entry();
        if (entry.isValid() && entry.status() == KNS3::Entry::Installed && !alreadyCopiedThumbnails.contains(entry.uniqueId())) {
            alreadyCopiedThumbnails.append(entry.uniqueId());
            KIO::file_copy(QUrl(entry.previewUrl(KNSCore::EntryInternal::PreviewBig1)),
                           QUrl::fromLocalFile(QString(entry.installedFiles().constFirst() + QStringLiteral(".png"))),
                           -1,
                           KIO::Overwrite | KIO::HideProgressInfo);
        }
    }
    reloadModel();
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
    Q_EMIT selectedPluginChanged();
    Q_EMIT selectedPluginIndexChanged();

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
    Q_EMIT busyChanged();
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
            Q_EMIT showErrorMessage(i18n("Unable to authenticate/execute the action: %1 (%2)", job->error(), job->errorString()));
        }
        load();
    }
    setBusy(false);
}

void KCMPlymouth::uninstall(const QString &plugin)
{
    QVariantMap helperargs;
    helperargs[QStringLiteral("theme")] = plugin;

    // KAuth::Action action(authActionName());
    KAuth::Action action(QStringLiteral("org.kde.kcontrol.kcmplymouth.uninstall"));
    action.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplymouth"));
    action.setArguments(helperargs);

    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        Q_EMIT showErrorMessage(i18n("Unable to authenticate/execute the action: %1 (%2)", job->error(), job->errorString()));
    } else {
        KConfigGroup installedCg(KSharedConfig::openConfig(QStringLiteral("kplymouththemeinstallerrc")), "DownloadedThemes");
        installedCg.deleteEntry(plugin);
        Q_EMIT showSuccessMessage(i18n("Theme uninstalled successfully."));
        load();
    }
}

void KCMPlymouth::defaults()
{ /*TODO
     if (!) {
         return;
     }
 */
}

#include "kcm.moc"
