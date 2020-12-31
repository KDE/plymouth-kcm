/*
 *  Copyright (C) 2016 Marco Martin <mart@kde.org>
 *  Copyright (C) 1998 Luca Montecchiani <m.luca@usa.net>
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

#include "helper.h"
#include "config-kcm.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>

#include "ktar.h"
#include "kzip.h"
#include <KArchive>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

ActionReply PlymouthHelper::save(const QVariantMap &args)
{
    const QString theme = args.value(QStringLiteral("theme")).toString();
    ActionReply reply;

    if (theme.isEmpty()) {
        reply = ActionReply::BackendError;
        reply.setErrorDescription(i18n("No theme specified in helper parameters."));
        return reply;
    }

    {
        KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral(PLYMOUTH_CONFIG_PATH)), "Daemon");
        cg.writeEntry("Theme", theme);
    }
    QFile configFile(QStringLiteral(PLYMOUTH_CONFIG_PATH));
    configFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);

    // Special case: Ubuntu derivatives, which work different from everybody else
    if (QFile::exists(QStringLiteral("/usr/sbin/update-alternatives"))) {
        // find the .plymouth file in the theme
        QDir dir(QStringLiteral(PLYMOUTH_THEMES_DIR) + theme);
        QStringList themeFile = dir.entryList(QStringList() << QStringLiteral("*.plymouth"));
        if (themeFile.count() != 1) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Theme corrupted: .plymouth file not found inside theme."));
            return reply;
        }
        int ret = 0;
        QProcess checkProcess;
        QByteArray data;
        qDebug() << "Running update-alternatives --list default.plymouth now";
        checkProcess.start(QStringLiteral("update-alternatives"), {QStringLiteral("--list"), QStringLiteral("default.plymouth")});
        if (!checkProcess.waitForStarted()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Cannot start update-alternatives."));
            return reply;
        }
        if (!checkProcess.waitForFinished()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("update-alternatives failed to run."));
            return reply;
        } else {
            data = checkProcess.readAllStandardOutput();
        }
        ret = checkProcess.exitCode();

        if (ret != 0) {
            reply = ActionReply(ActionReply::HelperErrorReply());
            reply.setErrorCode(static_cast<ActionReply::Error>(ret));
            reply.setErrorDescription(i18n("update-alternatives returned with error condition %1.", ret));
            return reply;
        }
        QString installFile = dir.path() + QLatin1Char('/') + themeFile.first();
        if (!data.contains(installFile.toUtf8())) {
            qDebug() << "Plymouth file not found in update-alternatives. So install it";
            QProcess installProcess;
            installProcess.start(QStringLiteral("update-alternatives"),
                                 QStringList() << QStringLiteral("--install") << QStringLiteral("/usr/share/plymouth/themes/default.plymouth")
                                               << QStringLiteral("default.plymouth") << installFile << QStringLiteral("100"));

            if (!installProcess.waitForStarted()) {
                reply = ActionReply::BackendError;
                reply.setErrorDescription(i18n("Cannot start update-alternatives."));
                return reply;
            }
            if (!installProcess.waitForFinished()) {
                reply = ActionReply::BackendError;
                reply.setErrorDescription(i18n("update-alternatives failed to run."));
                return reply;
            }
            ret = installProcess.exitCode();

            if (ret != 0) {
                reply = ActionReply(ActionReply::HelperErrorReply());
                reply.setErrorCode(static_cast<ActionReply::Error>(ret));
                reply.setErrorDescription(i18n("update-alternatives returned with error condition %1.", ret));
                return reply;
            }
        } else {
            qDebug() << "Running update-alternatives --set  now";
            QProcess process;
            process.start(QStringLiteral("update-alternatives"), QStringList() << QStringLiteral("--set") << QStringLiteral("default.plymouth") << installFile);
            if (!process.waitForStarted()) {
                reply = ActionReply::BackendError;
                reply.setErrorDescription(i18n("Cannot start update-alternatives."));
                return reply;
            }
            if (!process.waitForFinished()) {
                reply = ActionReply::BackendError;
                reply.setErrorDescription(i18n("update-alternatives failed to run."));
                return reply;
            }
            ret = process.exitCode();

            if (ret != 0) {
                reply = ActionReply(ActionReply::HelperErrorReply());
                reply.setErrorCode(static_cast<ActionReply::Error>(ret));
                reply.setErrorDescription(i18n("update-alternatives returned with error condition %1.", ret));
                return reply;
            }
        }
    }

    int ret = 0;

    QProcess process;
    qDebug() << "Running update-initramfs -u  now";
    process.start(QStringLiteral("/usr/sbin/update-initramfs"), QStringList() << QStringLiteral("-u"));
    if (!process.waitForStarted()) {
        reply = ActionReply::BackendError;
        reply.setErrorDescription(i18n("Cannot start initramfs."));
        return reply;
    }
    if (!process.waitForFinished(60000)) {
        reply = ActionReply::BackendError;
        reply.setErrorDescription(i18n("Initramfs failed to run."));
        return reply;
    }
    ret = process.exitCode();

    if (ret == 0) {
        return ActionReply::SuccessReply();
    } else {
        reply = ActionReply(ActionReply::HelperErrorReply());
        reply.setErrorCode(static_cast<ActionReply::Error>(ret));
        reply.setErrorDescription(i18n("Initramfs returned with error condition %1.", ret));
        return reply;
    }
}

ActionReply PlymouthHelper::install(const QVariantMap &args)
{
    const QString themearchive = args.value(QStringLiteral("themearchive")).toString();
    ActionReply reply;

    if (themearchive.isEmpty()) {
        return ActionReply::BackendError;
    }

    QDir basedir(QStringLiteral(PLYMOUTH_THEMES_DIR));
    if (!basedir.exists()) {
        return ActionReply::BackendError;
    }

    // this is weird but a decompression is not a single name, so take the path instead
    QString installpath = QStringLiteral(PLYMOUTH_THEMES_DIR);
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(themearchive);
    qWarning() << "Postinstallation: uncompress the file";

    // FIXME: check for overwriting, malicious archive entries (../foo) etc.
    // FIXME: KArchive should provide "safe mode" for this!
    QScopedPointer<KArchive> archive;

    if (mimeType.inherits(QStringLiteral("application/zip"))) {
        archive.reset(new KZip(themearchive));
        // clang-format off
    } else if (mimeType.inherits(QStringLiteral("application/tar"))
                || mimeType.inherits(QStringLiteral("application/x-gzip"))
                || mimeType.inherits(QStringLiteral("application/x-bzip"))
                || mimeType.inherits(QStringLiteral("application/x-lzma"))
                || mimeType.inherits(QStringLiteral("application/x-xz"))
                || mimeType.inherits(QStringLiteral("application/x-bzip-compressed-tar"))
                || mimeType.inherits(QStringLiteral("application/x-compressed-tar"))) {
        archive.reset(new KTar(themearchive));
        // clang-format on
    } else {
        qCritical() << "Could not determine type of archive file '" << themearchive << "'";
        return ActionReply::BackendError;
    }

    bool success = archive->open(QIODevice::ReadOnly);
    if (!success) {
        qCritical() << "Cannot open archive file '" << themearchive << "'";
        return ActionReply::BackendError;
    }

    QString themeName;
    QString themePath;
    const KArchiveDirectory *dir = archive->directory();
    // if there is more than an item in the file,
    // put contents in a subdirectory with the same name as the file
    if (dir->entries().count() > 1) {
        installpath += QLatin1Char('/') + QFileInfo(archive->fileName()).baseName();
        themeName = QFileInfo(archive->fileName()).baseName();
        themePath = installpath;
    } else {
        themeName = dir->entries().first();
        themePath = installpath + dir->entries().first();
    }
    dir->copyTo(installpath);

    const QStringList themeFileList = dir->entries().filter(QRegularExpression(QStringLiteral("\\.plymouth$")));

    archive->close();

    // Special case: Ubuntu derivatives, which work different from everybody else
    if (QFile::exists(QStringLiteral("/usr/sbin/update-alternatives"))) {
        // find the .plymouth file in the theme
        QDir dir(themePath);
        QStringList themeFile = dir.entryList(QStringList() << QStringLiteral("*.plymouth"));
        if (themeFile.count() != 1) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Theme corrupted: .plymouth file not found inside theme."));
            return reply;
        }
        int ret = 0;
        QProcess process;
        process.start(QStringLiteral("update-alternatives"),
                      QStringList() << QStringLiteral("--install") << QStringLiteral("/usr/share/plymouth/themes/default.plymouth")
                                    << QStringLiteral("default.plymouth") << themePath + QLatin1Char('/') + themeFile.first() << QStringLiteral("100"));
        if (!process.waitForStarted()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Cannot start update-alternatives."));
            return reply;
        }
        if (!process.waitForFinished()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("update-alternatives failed to run."));
            return reply;
        }
        ret = process.exitCode();

        if (ret != 0) {
            reply = ActionReply(ActionReply::HelperErrorReply());
            reply.setErrorCode(static_cast<ActionReply::Error>(ret));
            reply.setErrorDescription(i18n("update-alternatives returned with error condition %1.", ret));
            return reply;
        }
    }

    QVariantMap map;
    map[QStringLiteral("plugin")] = themeName;
    map[QStringLiteral("path")] = themePath;
    reply = ActionReply::SuccessReply();
    reply.setData(map);
    return reply;
}

ActionReply PlymouthHelper::uninstall(const QVariantMap &args)
{
    const QString theme = args.value(QStringLiteral("theme")).toString();
    ActionReply reply;

    if (theme.isEmpty()) {
        qWarning() << "No theme specified.";
        return ActionReply::BackendError;
    }

    QDir dir(QStringLiteral(PLYMOUTH_THEMES_DIR));
    if (!dir.exists()) {
        reply = ActionReply::BackendError;
        reply.setErrorDescription(i18n("Theme folder %1 does not exist.", QStringLiteral(PLYMOUTH_THEMES_DIR)));
        return reply;
    }

    if (!dir.cd(theme)) {
        reply = ActionReply::BackendError;
        reply.setErrorDescription(i18n("Theme %1 does not exist.", theme));
        return reply;
    }

    // Special case: Ubuntu derivatives, which work different from everybody else
    if (QFile::exists(QStringLiteral("/usr/sbin/update-alternatives"))) {
        // find the .plymouth file in the theme
        QStringList themeFile = dir.entryList(QStringList() << QStringLiteral("*.plymouth"));
        if (themeFile.count() != 1) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Theme corrupted: .plymouth file not found inside theme."));
            return reply;
        }
        int ret = 0;
        QProcess process;

        process.start(QStringLiteral("update-alternatives"),
                      QStringList() << QStringLiteral("--remove") << QStringLiteral("default.plymouth") << dir.path() + QLatin1Char('/') + themeFile.first());
        if (!process.waitForStarted()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("Cannot start update-alternatives."));
            return reply;
        }
        if (!process.waitForFinished()) {
            reply = ActionReply::BackendError;
            reply.setErrorDescription(i18n("update-alternatives failed to run."));
            return reply;
        }
        ret = process.exitCode();

        if (ret != 0) {
            reply = ActionReply(ActionReply::HelperErrorReply());
            reply.setErrorCode(static_cast<ActionReply::Error>(ret));
            reply.setErrorDescription(i18n("update-alternatives returned with error condition %1.", ret));
            return reply;
        }
    }

    if (dir.removeRecursively()) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::BackendError;
    }
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplymouth", PlymouthHelper)
