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
#include <QMimeDatabase>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>

#include "kzip.h"
#include "ktar.h"
#include <KArchive>
#include <KConfigGroup>
#include <KSharedConfig>
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

    QString themefile = args.first();
    themefile.replace(QStringLiteral("//"), QChar('/'));
    if (parser.isSet(QStringLiteral("install")) && !QFile::exists(themefile)) {
        qWarning() << "Specified theme file does not exists";
        return 0;
    }

    QVariantMap helperargs;
    helperargs[QStringLiteral("themearchive")] = themefile;

    //support uninstalling from an archive
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(themefile);
    bool isArchive = false;
    if (parser.isSet(QStringLiteral("uninstall"))) {
        QScopedPointer<KArchive> archive;
        if (mimeType.inherits(QStringLiteral("application/zip"))) {
            archive.reset(new KZip(themefile));
        } else if (mimeType.inherits(QStringLiteral("application/tar"))
            || mimeType.inherits(QStringLiteral("application/x-gzip"))
            || mimeType.inherits(QStringLiteral("application/x-bzip"))
            || mimeType.inherits(QStringLiteral("application/x-lzma"))
            || mimeType.inherits(QStringLiteral("application/x-xz"))
            || mimeType.inherits(QStringLiteral("application/x-bzip-compressed-tar"))
            || mimeType.inherits(QStringLiteral("application/x-compressed-tar"))) {
            archive.reset(new KTar(themefile));
        }
        if (archive) {
            isArchive = true;
            bool success = archive->open(QIODevice::ReadOnly);
            if (!success) {
                qCritical() << "Cannot open archive file '" << themefile << "'";
                exit(-1);
            }
            const KArchiveDirectory *dir = archive->directory();
            //if there is more than an item in the file,
            //plugin is a subdirectory with the same name as the file
            if (dir->entries().count() > 1) {
                helperargs[QStringLiteral("theme")] = QFileInfo(archive->fileName()).baseName();
            } else {
                helperargs[QStringLiteral("theme")] = dir->entries().first();
            }
        } else {
            helperargs[QStringLiteral("theme")] = themefile;
        }
    }

    KAuth::Action action(parser.isSet(QStringLiteral("install")) ? QStringLiteral("org.kde.kcontrol.kcmplymouth.install") : QStringLiteral("org.kde.kcontrol.kcmplymouth.uninstall"));
    action.setHelperId("org.kde.kcontrol.kcmplymouth");
    action.setArguments(helperargs);

    KAuth::ExecuteJob *job = action.execute();
    bool rc = job->exec();
    if (!rc) {
        qWarning() << i18n("Unable to authenticate/execute the action: %1, %2", job->error(), job->errorString());
        return -1;
    }

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kplymouththemeinstallerrc")), "DownloadedThemes");
    if (parser.isSet(QStringLiteral("install"))) {
        cg.writeEntry(job->data().value(QStringLiteral("plugin")).toString(), themefile);
    } else {
        cg.deleteEntry(job->data().value(QStringLiteral("plugin")).toString());
        if (isArchive) {
            //remove archive
            QFile(themefile).remove();
            //remove screenshot
            QFile::remove(QString(themefile + QStringLiteral(".png")));
        }
    }

    return app.exec();
}
