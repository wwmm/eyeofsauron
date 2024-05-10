import EoSTrackerBackend
import EoSdb
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

    title: i18n("Tracker")
    actions: [
        Kirigami.Action {
            text: i18n("Video Source")
            icon.name: "camera-web-symbolic"
            onTriggered: sourceMenu.open()
        },
        Kirigami.Action {
            icon.name: "media-playback-start-symbolic"
            text: i18nc("@action:button", "Play")
            onTriggered: EoSTrackerBackend.start()
        },
        Kirigami.Action {
            icon.name: "media-playback-pause-symbolic"
            text: i18nc("@action:button", "Pause")
            onTriggered: EoSTrackerBackend.pause()
        },
        Kirigami.Action {
            icon.name: "media-playback-stop-symbolic"
            text: i18nc("@action:button", "Stop")
            onTriggered: EoSTrackerBackend.stop()
        }
    ]

    Connections {
        function onUpdateChart() {
            for (let n = 0; n < chart.count; n += 2) {
                if (n + 1 < chart.count)
                    EoSTrackerBackend.updateSeries(chart.series(n), chart.series(n + 1), n);

            }
        }

        target: EoSTrackerBackend
    }

    SourceMenu {
        id: sourceMenu

        backend: EoSTrackerBackend
        model: EosTrackerSourceModel
    }

    RowLayout {
        anchors.fill: parent

        VideoOutput {
            id: videoOutput

            implicitWidth: EoSTrackerBackend.frameWidth
            implicitHeight: EoSTrackerBackend.frameHeight
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
                    if (event.button == Qt.RightButton) {
                        let seriesIndex = EoSTrackerBackend.removeRoi(event.x, event.y);
                        if (seriesIndex !== -1) {
                            chart.removeSeries(chart.series(seriesIndex)); // x
                            chart.removeSeries(chart.series(seriesIndex)); // y
                        }
                    }
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
                        EoSTrackerBackend.createNewRoi(x0, y0, width, height);
                        EoSTrackerBackend.drawRoiSelection(false);
                        chart.addSeries("x");
                        chart.addSeries("y");
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
                        EoSTrackerBackend.newRoiSelection(x, y, width, height);
                    }
                }
                onPressed: (event) => {
                    if (event.button == Qt.LeftButton) {
                        x0 = event.x;
                        y0 = event.y;
                        event.accepted = true;
                        EoSTrackerBackend.drawRoiSelection(true);
                    }
                }
            }

        }

        ChartView {
            id: chart

            function addSeries(axisName) {
                let label = i18n("Object-" + Math.floor(chart.count / 2) + "-" + axisName);
                let series = createSeries(ChartView.SeriesTypeLine, label, axisTime, axisPosition);
                series.useOpenGL = true;
            }

            title: i18n("Position")
            antialiasing: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: 480
            implicitWidth: 640
            theme: EoSdb.darkChartTheme === true ? ChartView.ChartThemeDark : ChartView.ChartThemeLight

            ValueAxis {
                id: axisPosition

                labelFormat: "%.1f"
                min: 0
                max: 1024
            }

            ValueAxis {
                id: axisTime

                labelFormat: "%.1f"
                min: 0
                max: 10
            }

        }

    }

    footer: Kirigami.ActionToolBar {
        actions: [
            Kirigami.Action {

                displayComponent: EoSSpinBox {
                    id: nPoints

                    label: i18n("History")
                    unit: i18n("points")
                    decimals: 0
                    stepSize: 1
                    from: 2
                    to: 1000
                    value: EoSdb.chartDataPoints
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
