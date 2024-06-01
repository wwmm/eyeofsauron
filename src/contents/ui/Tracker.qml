import EoSTrackerBackend
import EoSdb
import EosTrackerSourceModel
import QtCharts
import QtCore
import QtMultimedia
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
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
                EoSTrackerBackend.updateSeries(chart.series(n), chart.series(n + 1), Math.floor(n / 2));
            }
        }

        target: EoSTrackerBackend
    }

    SourceMenu {
        id: sourceMenu

        backend: EoSTrackerBackend
        model: EosTrackerSourceModel
    }

    FileDialog {
        id: fileDialogSaveChart

        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        nameFilters: ["PNG files (*.png)"]
        onAccepted: {
            chart.grabToImage(function(result) {
                result.saveToFile(fileDialogSaveChart.selectedFile);
            });
        }
    }

    FileDialog {
        id: fileDialogSaveTable

        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: ["TXT Table files (*.tsv)"]
        onAccepted: {
            EoSTrackerBackend.saveTable(fileDialogSaveTable.selectedFile);
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            VideoOutput {
                id: videoOutput

                implicitWidth: EoSTrackerBackend.frameWidth
                implicitHeight: EoSTrackerBackend.frameHeight
                Component.onCompleted: {
                    EoSTrackerBackend.videoSink = videoOutput.videoSink;
                }
                Component.onDestruction: EoSTrackerBackend.stop() // So we do not write to an invalid videoSink

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
                            chart.addSeries("x");
                            chart.addSeries("y");
                            EoSTrackerBackend.createNewRoi(x0, y0, width, height);
                            EoSTrackerBackend.drawRoiSelection(false);
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

            ColumnLayout {
                ChartView {
                    id: chart

                    property real rangeMargin: 0.01

                    function addSeries(axisName) {
                        let name = i18n(axisName + Math.floor(chart.count / 2));
                        let series = createSeries(ChartView.SeriesTypeLine, name, axisTime, axisPosition);
                        series.useOpenGL = true;
                        if (axisName === "x")
                            series.visible = Qt.binding(function() {
                            return actionViewXdata.showChart;
                        });
                        else if (axisName === "y")
                            series.visible = Qt.binding(function() {
                            return actionViewYdata.showChart;
                        });
                    }

                    antialiasing: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    implicitHeight: 480
                    implicitWidth: 640
                    theme: EoSdb.darkChartTheme === true ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                    localizeNumbers: true

                    ValueAxis {
                        id: axisPosition

                        labelFormat: "%.1f"
                        min: EoSTrackerBackend.yAxisMin * (1 - chart.rangeMargin)
                        max: EoSTrackerBackend.yAxisMax * (1 + chart.rangeMargin)
                        titleText: i18n("Position [px]")
                    }

                    ValueAxis {
                        id: axisTime

                        labelFormat: "%.1f"
                        min: EoSTrackerBackend.xAxisMin
                        max: EoSTrackerBackend.xAxisMax
                        titleText: i18n("Time [s]")
                    }

                    Rectangle {
                        id: zoomRect

                        color: EoSdb.darkChartTheme === true ? "aquamarine" : "crimson"
                        opacity: 0.25
                        visible: false
                        width: 0
                        height: 0
                    }

                    MouseArea {
                        // console.log(chart.mapToValue(Qt.point(event.x, event.y)));

                        id: chartMouseArea

                        property real mouseX: 0
                        property real mouseY: 0
                        property real x0: 0
                        property real y0: 0

                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        preventStealing: true
                        hoverEnabled: true
                        onPressed: (event) => {
                            if (event.button == Qt.LeftButton) {
                                x0 = event.x;
                                y0 = event.y;
                                zoomRect.width = 0;
                                zoomRect.height = 0;
                                zoomRect.visible = true;
                                event.accepted = true;
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
                                chart.zoomIn(zoomRect);
                                zoomRect.visible = false;
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
                                zoomRect.x = x;
                                zoomRect.y = y;
                                zoomRect.width = width;
                                zoomRect.height = height;
                            } else if (chart.count > 0) {
                                let p = chart.mapToValue(Qt.point(event.x, event.y));
                                mouseX = p.x;
                                mouseY = p.y;
                            }
                        }
                        onExited: {
                            mouseX = 0;
                            mouseY = 0;
                        }
                    }

                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.textColor
                    text: {
                        return `t = ${chartMouseArea.mouseX.toFixed(1)} \t p = ${chartMouseArea.mouseY.toFixed(1)}`;
                    }
                }

            }

        }

        RowLayout {
            visible: EoSTrackerBackend.showPlayerSlider

            Text {
                horizontalAlignment: Text.AlignRight
                color: Kirigami.Theme.textColor
                text: {
                    var m = Math.floor(EoSTrackerBackend.playerPosition / 60000);
                    var ms = (EoSTrackerBackend.playerPosition / 1000 - m * 60).toFixed(1);
                    return `${m}:${ms.padStart(4, 0)}`;
                }
            }

            Controls.Slider {
                id: playerSlider

                Layout.fillWidth: true
                to: 1
                value: EoSTrackerBackend.playerPosition / EoSTrackerBackend.playerDuration
                onMoved: EoSTrackerBackend.setPlayerPosition(value * EoSTrackerBackend.playerDuration)
            }

            Text {
                horizontalAlignment: Text.AlignRight
                color: Kirigami.Theme.textColor
                text: {
                    var m = Math.floor(EoSTrackerBackend.playerDuration / 60000);
                    var ms = (EoSTrackerBackend.playerDuration / 1000 - m * 60).toFixed(1);
                    return `${m}:${ms.padStart(4, 0)}`;
                }
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
                    onValueModified: (v) => {
                        EoSdb.chartDataPoints = v;
                    }
                }

            },
            Kirigami.Action {
                id: actionViewXdata

                property bool showChart: true

                displayComponent: Controls.CheckBox {
                    text: i18n("x")
                    checked: EoSTrackerBackend.xDataVisible
                    onCheckedChanged: {
                        actionViewXdata.showChart = checked;
                        if (EoSTrackerBackend.xDataVisible !== checked)
                            EoSTrackerBackend.xDataVisible = checked;

                    }
                }

            },
            Kirigami.Action {
                id: actionViewYdata

                property bool showChart: true

                displayComponent: Controls.CheckBox {
                    text: i18n("y")
                    checked: EoSTrackerBackend.yDataVisible
                    onCheckedChanged: {
                        actionViewYdata.showChart = checked;
                        if (EoSTrackerBackend.yDataVisible !== checked)
                            EoSTrackerBackend.yDataVisible = checked;

                    }
                }

            },
            Kirigami.Action {
                text: i18n("Save Chart")
                icon.name: "folder-chart-symbolic"
                onTriggered: {
                    fileDialogSaveChart.open();
                }
            },
            Kirigami.Action {
                text: i18n("Save Table")
                icon.name: "folder-table-symbolic"
                onTriggered: {
                    fileDialogSaveTable.open();
                }
            },
            Kirigami.Action {
                text: i18n("Reset Zoom")
                icon.name: "edit-reset-symbolic"
                onTriggered: {
                    chart.zoomReset();
                }
            },
            Kirigami.Action {
                text: i18n("Remove Trackers")
                icon.name: "delete-symbolic"
                onTriggered: {
                    EoSTrackerBackend.removeAllTrackers();
                    chart.removeAllSeries();
                }
            }
        ]
    }

}
