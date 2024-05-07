import QtCore
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.OverlaySheet {
    id: control

    property var backend: null
    property var model: null

    showCloseButton: false
    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: root.height * 0.75

    ListView {
        id: sourcesListView

        Layout.fillWidth: true
        clip: true
        model: control.model
        delegate: listDelegate

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

            property string sourceType: model.sourceType
            property string sourceName: model.name
            property string sourceSubtitle: model.subtitle
            property string sourceIcon: model.icon
            property int wrapMode: Text.WrapAnywhere
            property int elide: Text.ElideRight
            property bool selected: listItemDelegate.highlighted || listItemDelegate.down
            property color color: selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor

            highlighted: ListView.isCurrentItem
            hoverEnabled: true
            width: parent ? parent.width : implicitWidth
            onClicked: {
                sourcesListView.currentIndex = index;
                backend.selectSource(index);
            }

            contentItem: Kirigami.ActionToolBar {
                actions: [
                    Kirigami.Action {
                        text: sourceName
                        enabled: false
                        displayHint: Kirigami.DisplayHint.KeepVisible

                        displayComponent: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Icon {
                                source: sourceIcon
                            }

                            ColumnLayout {
                                spacing: Kirigami.Units.smallSpacing

                                Controls.Label {
                                    wrapMode: Text.WordWrap
                                    text: sourceName
                                }

                                Controls.Label {
                                    wrapMode: Text.WordWrap
                                    text: sourceSubtitle
                                }

                            }

                        }

                    },
                    Kirigami.Action {
                        text: i18n("Remove this Source")
                        icon.name: "delete"
                        visible: sourceType === "video_file"
                        displayHint: Kirigami.DisplayHint.AlwaysHide
                        onTriggered: {
                            console.log(model.name);
                            control.model.removeSource(index);
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
        nameFilters: ["Video files (*.*)"]
        onAccepted: {
            backend.append(fileDialog.selectedFile);
        }
    }

    header: Kirigami.ActionToolBar {
        alignment: Qt.AlignCenter
        actions: [
            Kirigami.Action {
                text: i18n("Add Video File")
                icon.name: "video-symbolic"
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
