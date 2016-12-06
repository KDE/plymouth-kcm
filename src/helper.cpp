/*
 *  tzone.cpp
 *
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

/*

 A helper that's run using KAuth and does the system modifications.

*/

#include "helper.h"
#include "config-kcm.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QProcess>

#include <KConfigGroup>
#include <KSharedConfig>


ActionReply PlymouthHelper::save(const QVariantMap &args)
{
    const QString theme = args.value(QStringLiteral("theme")).toString();

    if (theme.isEmpty()) {
        return ActionReply::BackendError;
    }
    qWarning()<<"KAUTH HELPER CALLED SAVE WITH" << theme;

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral(PLYMOUTH_CONFIG_PATH)), "Daemon");
    cg.writeEntry("Theme", theme);


    int ret = 0;

    QProcess process;
    process.start("update-initramfs", QStringList() << "-u");
    if (!process.waitForStarted()) {
        return ActionReply::BackendError;
    }
    if (!process.waitForFinished()) {
        return ActionReply::BackendError;
    }
    ret = process.exitCode();

    if (ret == 0) {
        return ActionReply::SuccessReply();
    } else {
        ActionReply reply(ActionReply::HelperErrorReply());
        reply.setErrorCode(static_cast<ActionReply::Error>(ret));
        return reply;
    }
}

ActionReply PlymouthHelper::install(const QVariantMap &args)
{
    const QString theme = args.value(QStringLiteral("theme")).toString();
    const QString tmpPath = args.value(QStringLiteral("tmpdir")).toString();

    if (theme.isEmpty()) {
        return ActionReply::BackendError;
    }
    qWarning()<<"KAUTH HELPER CALLED INSTALL WITH" << theme;

    QDir dir(PLYMOUTH_THEMES_DIR);
    if (!dir.exists()) {
        return ActionReply::BackendError;
    }

    QDir tmpdir(tmpPath);
    if (!tmpdir.exists()) {
        return ActionReply::BackendError;
    }

    if (tmpdir.rename(tmpdir.path(), dir.path() + QChar('/') + theme)) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::BackendError;
    }
}

ActionReply PlymouthHelper::uninstall(const QVariantMap &args)
{
    const QString theme = args.value(QStringLiteral("theme")).toString();

    if (theme.isEmpty()) {
        return ActionReply::BackendError;
    }
    qWarning()<<"KAUTH HELPER CALLED INSTALL WITH" << theme;

    QDir dir(PLYMOUTH_THEMES_DIR);
    if (!dir.exists()) {
        return ActionReply::BackendError;
    }

    dir.cd(theme);
    if (!dir.exists()) {
        return ActionReply::BackendError;
    }
    if (dir.removeRecursively()) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::BackendError;
    }
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplymouth", PlymouthHelper)
