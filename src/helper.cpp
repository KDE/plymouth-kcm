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

#include <QFile>
#include <QDebug>
#include <QProcess>


ActionReply PlymouthHelper::save(const QVariantMap &args)
{
    QString theme = args.value(QStringLiteral("theme")).toString();

    int ret = 0;
    
    qWarning()<<"KAUTH HELPER CALLED WITH" << theme;

    if (ret == 0) {
        return ActionReply::SuccessReply();
    } else {
        ActionReply reply(ActionReply::HelperErrorReply());
        reply.setErrorCode(static_cast<ActionReply::Error>(ret));
        return reply;
    }
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplymouth", PlymouthHelper)
