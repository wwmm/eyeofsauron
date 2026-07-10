pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import eos.ui

Kirigami.OverlaySheet {
    id: control

    property var backend: null
    property var model: null
    property string backendName

    signal sourceNameChanged(string value)

    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: root.height * 0.75

    ListView {
        id: sourcesListView

        Layout.fillWidth: true
        clip: true
        model: control.model
        delegate: listDelegate
        reuseItems: true

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: sourcesListView.count === 0
            text: i18n("No Source Available")
        }
    }

    Component {
        id: listDelegate

        Controls.ItemDelegate {
            id: listItemDelegate

            required property int index
            required property string sourceType
            required property string sourceName
            required property string sourceSubtitle
            required property string sourceIcon

            property int wrapMode: Text.WrapAnywhere
            property int elide: Text.ElideRight
            property bool selected: listItemDelegate.highlighted || listItemDelegate.down
            property color color: selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor

            highlighted: sourcesListView.currentIndex === index
            hoverEnabled: true
            width: parent ? parent.width : implicitWidth
            onClicked: {
                sourcesListView.currentIndex = index;
            }
            onHighlightedChanged: {
                if (highlighted) {
                    control.backend.selectSource(listItemDelegate.index);
                    control.sourceNameChanged(listItemDelegate.sourceName);
                }
            }

            Kirigami.PromptDialog {
                id: deleteDialog

                title: i18n("Remove Source")
                subtitle: i18n("Are you sure you want to remove this source?")
                standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
                onAccepted: {
                    control.model.removeSource(listItemDelegate.index);
                }
            }

            contentItem: Kirigami.ActionToolBar {
                actions: [
                    Kirigami.Action {
                        text: listItemDelegate.sourceName
                        enabled: false
                        displayHint: Kirigami.DisplayHint.KeepVisible

                        displayComponent: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                source: listItemDelegate.sourceIcon
                            }

                            ColumnLayout {
                                spacing: Kirigami.Units.smallSpacing

                                Controls.Label {
                                    wrapMode: Text.WordWrap
                                    text: listItemDelegate.sourceName
                                }

                                Controls.Label {
                                    wrapMode: Text.WordWrap
                                    text: listItemDelegate.sourceSubtitle
                                    visible: !Common.isEmpty(listItemDelegate.sourceSubtitle)
                                }
                            }
                        }
                    },
                    Kirigami.Action {
                        text: i18n("Remove this Source")
                        icon.name: "delete"
                        visible: listItemDelegate.sourceType === "media_file"
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            deleteDialog.open();
                        }
                    }
                ]

                anchors {
                    left: parent.left
                    leftMargin: Kirigami.Units.smallSpacing
                    right: parent.right
                    rightMargin: Kirigami.Units.smallSpacing
                }
            }
        }
    }

    FileDialog {
        id: fileDialog

        fileMode: FileDialog.OpenFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        nameFilters: control.backendName === "tracker" ? ["Video files (*.*)"] : ["Audio files (*.*)"]
        onAccepted: {
            control.backend.append(fileDialog.selectedFile);
        }
    }

    header: Kirigami.ActionToolBar {
        alignment: Qt.AlignCenter
        actions: [
            Kirigami.Action {
                text: control.backendName === "tracker" ? i18n("Add Video File") : i18n("Add Audio File")
                icon.name: control.backendName === "tracker" ? "video-symbolic" : "emblem-music-symbolic"
                onTriggered: {
                    fileDialog.open();
                }
            }
        ]

        anchors {
            left: parent.left
            leftMargin: Kirigami.Units.smallSpacing
            right: parent.right
            rightMargin: Kirigami.Units.smallSpacing
        }
    }
}
