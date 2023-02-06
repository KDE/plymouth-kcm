/*
 *  SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kcm 1.3 as KCM
import org.kde.newstuff 1.91 as NewStuff

KCM.GridViewKCM {
    KCM.ConfigModule.quickHelp: i18n("This module lets you choose the Plymouth boot splash screen.")

    actions.main: NewStuff.Action {
        id: newStuffButton
        text: i18nc("@action:button as in, get new Plymouth boot splash screens", "Get New…")
        enabled: !kcm.busy
        icon.name: "get-hot-new-stuff"
        configFile: "plymouth.knsrc"
        onChangedEntriesChanged: kcm.onChangedEntriesChanged(newStuffButton.changedEntries);
    }

    view.model: kcm.themesModel
    view.currentIndex: kcm.selectedPluginIndex
    view.enabled: !kcm.busy

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        toolTip: model.description

        thumbnailAvailable: !!model.screenshot
        thumbnail: Image {
            anchors.fill: parent
            source: model.screenshot
            sourceSize: Qt.size(delegate.GridView.view.cellWidth * Screen.devicePixelRatio,
 delegate.GridView.view.cellHeight * Screen.devicePixelRatio)
        }

        actions: [
            Kirigami.Action {
                iconName: "edit-delete"
                tooltip: i18n("Uninstall")
                enabled: model.uninstallable
                onTriggered: kcm.uninstall(model.pluginName)
            }
        ]
        onClicked: {
            kcm.selectedPlugin = model.pluginName;
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    footer: ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true
            showCloseButton: true
        }

        QtControls.ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            visible: kcm.busy
            indeterminate: true
        }
    }

    Connections {
        target: kcm
        function onShowSuccessMessage() {
            infoLabel.type = Kirigami.MessageType.Positive;
            infoLabel.text = message;
            infoLabel.visible = true;
        }
        function onShowErrorMessage() {
            infoLabel.type = Kirigami.MessageType.Error;
            infoLabel.text = message;
            infoLabel.visible = true;
        }
    }
}
