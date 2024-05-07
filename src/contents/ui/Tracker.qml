import EoSTrackerBackend
import EosTrackerSourceModel
import QtCharts
import QtMultimedia
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates
import org.kde.kirigamiaddons.formcard as FormCard

Kirigami.ScrollablePage {
    id: tracker

    title: i18n("Object Tracker")
    actions: [
        Kirigami.Action {
            icon.name: "media-playback-start-symbolic"
            text: i18nc("@action:button", "Play")
            onTriggered: EoSTrackerBackend.start()
        },
        Kirigami.Action {
            icon.name: "media-playback-stop-symbolic"
            text: i18nc("@action:button", "Stop")
            onTriggered: EoSTrackerBackend.stop()
        }
    ]

    RowLayout {
        anchors.fill: parent

        VideoOutput {
            id: videoOutput

            implicitWidth: EoSTrackerBackend.frameWidth
            implicitHeight: EoSTrackerBackend.frameHeight
            // Layout.maximumWidth: EoSTrackerBackend.frameWidth
            // Layout.maximumHeight: EoSTrackerBackend.frameHeight
            // fillMode: VideoOutput.Stretch
            Component.onCompleted: {
                EoSTrackerBackend.videoSink = videoOutput.videoSink;
            }

            MouseArea {
                id: mouseArea

                property real x0: 0
                property real y0: 0

                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                preventStealing: true
                onClicked: (event) => {
                    if (event.button == Qt.RightButton)
                        console.log("mouse clicked");

                }
                onReleased: (event) => {
                    if (event.button == Qt.LeftButton) {
                        let width = event.x - x0;
                        let height = event.y - y0;
                        if (Math.abs(width) === 0 || Math.abs(height) === 0)
                            return ;

                        if (width < 0) {
                            x0 = width + x0;
                            width *= -1;
                        }
                        if (height < 0) {
                            y0 = height + y0;
                            height *= -1;
                        }
                        EoSTrackerBackend.onNewRoi(x0, y0, width, height);
                        event.accepted = true;
                    }
                }
                onPositionChanged: (event) => {
                    if (event.buttons & Qt.LeftButton) {
                        let x = x0;
                        let y = y0;
                        let width = event.x - x;
                        let height = event.y - y;
                        if (Math.abs(width) === 0 || Math.abs(height) === 0)
                            return ;

                        if (width < 0) {
                            x = width + x;
                            width *= -1;
                        }
                        if (height < 0) {
                            y = height + y;
                            height *= -1;
                        }
                        EoSTrackerBackend.onNewRoiSelection(x, y, width, height);
                    }
                }
                onPressed: (event) => {
                    if (event.button == Qt.LeftButton) {
                        x0 = event.x;
                        y0 = event.y;
                        event.accepted = true;
                    }
                }
            }

        }

        ChartView {
            title: "Line Chart"
            antialiasing: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: 480
            implicitWidth: 640

            LineSeries {
                name: "Line"

                XYPoint {
                    x: 0
                    y: 0
                }

                XYPoint {
                    x: 1.1
                    y: 2.1
                }

                XYPoint {
                    x: 1.9
                    y: 3.3
                }

                XYPoint {
                    x: 2.1
                    y: 2.1
                }

                XYPoint {
                    x: 2.9
                    y: 4.9
                }

                XYPoint {
                    x: 3.4
                    y: 3
                }

                XYPoint {
                    x: 4.1
                    y: 3.3
                }

            }

        }

    }

    footer: Kirigami.ActionToolBar {
        actions: [
            Kirigami.Action {
                // comboBoxDelegate: Delegates.RoundedItemDelegate {
                //     implicitWidth: Kirigami.Units.gridUnit * 16
                //     text: value
                //     highlighted: sourceIndex.currentIndex === index
                //     icon.name: sourceIcon
                // }
                // displayComponent: FormCard.FormComboBoxDelegate {
                //     id: sourceIndex
                //     text: i18n("Source")
                //     displayMode: FormCard.FormComboBoxDelegate.ComboBox
                //     currentIndex: EoSTrackerBackend.sourceIndex
                //     editable: false
                //     textRole: "value"
                //     onActivated: (idx) => {
                //         if (idx !== EoSTrackerBackend.sourceIndex)
                //             EoSTrackerBackend.sourceIndex = idx;

                //     }
                //     model: EosTrackerSourceModel
                // }
                displayComponent: EoSComboBox {
                    id: sourceIndex

                    text: i18n("Source")
                    textRole: "value"
                    iconRole: "sourceIcon"
                    editable: false
                    currentIndex: EoSTrackerBackend.sourceIndex
                    model: EosTrackerSourceModel
                }

            },
            Kirigami.Action {

                displayComponent: EoSSpinBox {
                    id: nPoints

                    label: i18n("Chart")
                    unit: i18n("points")
                    decimals: 0
                    stepSize: 1
                    from: 2
                    to: 1000
                    value: 100
                }

            },
            Kirigami.Action {
                text: i18n("Save Chart")
                icon.name: "folder-chart-symbolic"
            },
            Kirigami.Action {
                text: i18n("Save Table")
                icon.name: "folder-table-symbolic"
            }
        ]
    }

}
