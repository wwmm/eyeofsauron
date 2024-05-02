import QtCharts
import QtMultimedia
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: tracker

    title: i18n("Object Tracker")
    actions: [
        Kirigami.Action {
            // onTriggered: CppModelEnvVars.append("", "")

            icon.name: "media-playback-start-symbolic"
            text: i18nc("@action:button", "Play")
        },
        Kirigami.Action {
            icon.name: "media-playback-pause-symbolic"
            text: i18nc("@action:button", "Pause")
        },
        Kirigami.Action {
            icon.name: "media-playback-stop-symbolic"
            text: i18nc("@action:button", "Stop")
        }
    ]

    RowLayout {
        anchors.fill: parent

        VideoOutput {
            id: videoOutput

            implicitWidth: 640
            implicitHeight: 480
            Layout.maximumWidth: 640
            Layout.maximumHeight: 480
            fillMode: VideoOutput.Stretch
        }

        MediaPlayer {
            id: player

            // source: "file:///home/wallace/Videos/viewpca.webm"
            // source: "qrc:/images/offline.png"
            videoOutput: videoOutput
            autoPlay: true
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
                text: i18n("Points")

                displayComponent: EoSSpinBox {
                    id: nPoints

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
