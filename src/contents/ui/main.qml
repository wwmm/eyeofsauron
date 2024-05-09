import AboutEoS
import EoSdb
import Qt.labs.platform
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

Kirigami.ApplicationWindow {
    id: root

    width: EoSdb.width
    height: EoSdb.height
    pageStack.initialPage: tracker
    pageStack.globalToolBar.style: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.Titles : Kirigami.ApplicationHeaderStyle.Auto
    title: i18nc("@title:window", "Eye Of Sauron")
    onWidthChanged: {
        EoSdb.width = applicationWindow().width;
    }
    onHeightChanged: {
        EoSdb.height = applicationWindow().height;
    }
    onVisibleChanged: {
        if (!root.visible)
            EoSdb.save();

    }

    Tracker {
        id: tracker

        visible: true
    }

    SoundWave {
        id: soundWave

        visible: false
    }

    Kirigami.Dialog {
        id: aboutDialog

        Kirigami.AboutPage {
            implicitWidth: Kirigami.Units.gridUnit * 24
            implicitHeight: Kirigami.Units.gridUnit * 21
            aboutData: AboutEoS
        }

    }

    Kirigami.OverlayDrawer {
        id: progressBottomDrawer

        edge: Qt.BottomEdge
        modal: false
        parent: applicationWindow().overlay
        drawerOpen: false

        contentItem: RowLayout {
            Controls.ProgressBar {
                from: 0
                to: 100
                indeterminate: true
                Layout.fillWidth: true
            }

        }

    }

    SystemTrayIcon {
        id: tray

        visible: EoSdb.showTrayIcon
        icon.name: "eyeofsauron"
        onActivated: {
            if (!root.visible) {
                root.show();
                root.raise();
                root.requestActivate();
            } else {
                root.hide();
            }
        }

        menu: Menu {
            visible: false

            MenuItem {
                text: i18n("Quit")
                onTriggered: Qt.quit()
            }

        }

    }

    Component {
        id: preferencesPage

        PreferencesPage {
        }

    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer

        drawerOpen: true
        showHeaderWhenCollapsed: true
        collapsible: true
        modal: Kirigami.Settings.isMobile ? true : false
        actions: [
            Kirigami.Action {
                text: tracker.title
                icon.name: "camera-web-symbolic"
                checked: tracker.visible
                onTriggered: {
                    if (!tracker.visible) {
                        while (pageStack.depth > 0)pageStack.pop()
                        pageStack.push(tracker);
                    }
                }
            },
            Kirigami.Action {
                text: soundWave.title
                icon.name: "waveform-symbolic"
                checked: soundWave.visible
                onTriggered: {
                    if (!soundWave.visible) {
                        while (pageStack.depth > 0)pageStack.pop()
                        pageStack.push(soundWave);
                    }
                }
            }
        ]

        header: Kirigami.AbstractApplicationHeader {

            contentItem: RowLayout {
                anchors {
                    left: parent.left
                    leftMargin: Kirigami.Units.smallSpacing
                    right: parent.right
                    rightMargin: Kirigami.Units.smallSpacing
                }

                Kirigami.Icon {
                    Layout.alignment: Qt.AlignHCenter
                    source: "image-red-eye-symbolic"
                    visible: !globalDrawer.collapsed
                }

                Kirigami.ActionToolBar {
                    actions: [
                        Kirigami.Action {
                            text: i18n("Preferences")
                            icon.name: "gtk-preferences"
                            displayHint: Kirigami.DisplayHint.AlwaysHide
                            onTriggered: {
                                root.pageStack.layers.push(preferencesPage);
                            }
                        },
                        Kirigami.Action {
                            text: i18n("About Eye Of Sauron")
                            icon.name: "eyeofsauron"
                            displayHint: Kirigami.DisplayHint.AlwaysHide
                            onTriggered: {
                                aboutDialog.open();
                            }
                        },
                        Kirigami.Action {
                            text: i18n("Quit")
                            icon.name: "gtk-quit"
                            displayHint: Kirigami.DisplayHint.AlwaysHide
                            onTriggered: {
                                Qt.quit();
                            }
                        }
                    ]
                }

            }

        }

    }

}
