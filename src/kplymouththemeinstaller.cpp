/*
 *  Copyright (C) 2016 Marco Martin <mart@kde.org>
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

#include "config-kcm.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QProcess>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>

#include <klocalizedstring.h>
#include <kauthaction.h>
#include <kauthexecutejob.h>

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    QCoreApplication app(argc, argv);

    const QString description = i18n("Plymouth theme installer");
    const char version[] = "1.0";

    app.setApplicationVersion(version);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("i") << QStringLiteral("install"), i18n("Install a theme.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("u") << QStringLiteral("uninstall"), i18n("Uninstall a theme.")));

    parser.addPositionalArgument("themefile", i18n("The theme to install, must be an existing archive file."));

    parser.process(app);

    if (!parser.isSet(QStringLiteral("install")) && !parser.isSet(QStringLiteral("uninstall"))) {
        qWarning() << "You need to specify either install or uninstall operation..";
        return 0;
    }
    const QStringList args = parser.positionalArguments();

    if (args.isEmpty()) {
        qWarning() << "No theme file specified.";
        return 0;
    }

    const QString themefile = args.first();
    if (parser.isSet(QStringLiteral("install")) && !QFile::exists(themefile)) {
        qWarning() << "Specified theme file does not exists";
        return 0;
    }

    QVariantMap helperargs;
    helperargs[QStringLiteral("themearchive")] = themefile;
    KAuth::Action action(parser.isSet(QStringLiteral("install")) ? QStringLiteral("org.kde.kcontrol.kcmplymouth.install") : QStringLiteral("org.kde.kcontrol.kcmplymouth.uninstall"));
    action.setHelperId("org.kde.kcontrol.kcmplymouth");
    action.setArguments(helperargs);

    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        qWarning() << i18n("Unable to authenticate/execute the action: %1, %2", job->error(), job->errorString());
        return -1;
    }

    qWarning()<< themefile;
    return app.exec();
}
