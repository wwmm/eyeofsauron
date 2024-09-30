import EoSSoundBackend
import EoSdb
import EosSoundSourceModel
import QtCharts
import QtCore
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: soundWave

    title: i18n("Sound Wave")
    actions: [
        Kirigami.Action {
            text: i18n("Audio Source")
            icon.name: "emblem-music-symbolic"
            onTriggered: sourceMenu.open()
        },
        Kirigami.Action {
            icon.name: "media-playback-start-symbolic"
            text: i18nc("@action:button", "Play")
            onTriggered: EoSSoundBackend.start()
        },
        Kirigami.Action {
            icon.name: "media-playback-pause-symbolic"
            text: i18nc("@action:button", "Pause")
            onTriggered: EoSSoundBackend.pause()
        },
        Kirigami.Action {
            icon.name: "media-playback-stop-symbolic"
            text: i18nc("@action:button", "Stop")
            onTriggered: EoSSoundBackend.stop()
        }
    ]

    SourceMenu {
        id: sourceMenu

        backend: EoSSoundBackend
        model: EosSoundSourceModel
        backendName: "sound_wave"
    }

    Connections {
        function onUpdateChart() {
            EoSSoundBackend.updateSeriesWaveform(chartWaveForm.series(0));
            EoSSoundBackend.updateSeriesFFT(chartFFT.series(0));
        }

        target: EoSSoundBackend
    }

    FileDialog {
        id: fileDialogSaveChart

        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        nameFilters: ["PNG files (*.png)"]
        onAccepted: {
            chartWaveForm.grabToImage(function(result) {
                result.saveToFile(fileDialogSaveChart.selectedFile.toString().replace(".png", "_waveform.png"));
            });
            chartFFT.grabToImage(function(result) {
                result.saveToFile(fileDialogSaveChart.selectedFile.toString().replace(".png", "_fft.png"));
            });
        }
    }

    FileDialog {
        id: fileDialogSaveTable

        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        nameFilters: ["TXT Table files (*.tsv)"]
        onAccepted: {
            EoSSoundBackend.saveTable(fileDialogSaveTable.selectedFile);
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            ColumnLayout {
                ChartView {
                    id: chartWaveForm

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    implicitHeight: 480
                    implicitWidth: 640
                    antialiasing: true
                    theme: EoSdb.darkChartTheme === true ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                    localizeNumbers: true

                    ValueAxis {
                        id: axisTime

                        labelFormat: "%.2e"
                        min: EoSSoundBackend.xAxisMinWave
                        max: EoSSoundBackend.xAxisMaxWave
                        titleText: i18n("Time [s]")
                    }

                    ValueAxis {
                        id: axisWaveform

                        labelFormat: "%.1e"
                        min: EoSSoundBackend.yAxisMinWave
                        max: EoSSoundBackend.yAxisMaxWave
                        titleText: i18n("Amplitude")
                    }

                    Rectangle {
                        id: zoomRectWaveForm

                        color: EoSdb.darkChartTheme === true ? "aquamarine" : "crimson"
                        opacity: 0.25
                        visible: false
                        width: 0
                        height: 0
                    }

                    ChartMouseArea {
                        id: mouseAreaWaveForm

                        chart: chartWaveForm
                        zoomRect: zoomRectWaveForm
                    }

                    LineSeries {
                        id: chartWaveFormLineSeries

                        name: i18n("Waveform")
                        axisX: axisTime
                        axisY: axisWaveform
                        useOpenGL: EoSdb.chartsUseOpenGL
                    }

                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.textColor
                    text: {
                        return `x = ${mouseAreaWaveForm.mouseX.toExponential(5)} \t y = ${mouseAreaWaveForm.mouseY.toExponential(5)}`;
                    }
                }

            }

            ColumnLayout {
                ChartView {
                    id: chartFFT

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    implicitHeight: 480
                    implicitWidth: 640
                    antialiasing: true
                    theme: EoSdb.darkChartTheme === true ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                    localizeNumbers: true

                    LogValueAxis {
                        id: axisFreq

                        labelFormat: "%.1e"
                        min: EoSSoundBackend.xAxisMinFFT
                        max: EoSSoundBackend.xAxisMaxFFT
                        titleText: i18n("Frequency [Hz]")
                        base: 10
                    }

                    ValueAxis {
                        id: axisFFT

                        labelFormat: "%.1e"
                        min: EoSSoundBackend.yAxisMinFFT
                        max: EoSSoundBackend.yAxisMaxFFT
                        titleText: i18n("Amplitude²")
                    }

                    Rectangle {
                        id: zoomRectFFT

                        color: EoSdb.darkChartTheme === true ? "aquamarine" : "crimson"
                        opacity: 0.25
                        visible: false
                        width: 0
                        height: 0
                    }

                    ChartMouseArea {
                        id: mouseAreaFFT

                        chart: chartFFT
                        zoomRect: zoomRectFFT
                    }

                    LineSeries {
                        id: chartFFTLineSeries

                        name: i18n("Fourier Transform")
                        axisX: axisFreq
                        axisY: axisFFT
                        useOpenGL: EoSdb.chartsUseOpenGL
                    }

                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.textColor
                    text: {
                        return `x = ${mouseAreaFFT.mouseX.toExponential(5)} \t y = ${mouseAreaFFT.mouseY.toExponential(5)}`;
                    }
                }

            }

        }

        RowLayout {
            visible: EoSSoundBackend.showPlayerSlider

            Text {
                horizontalAlignment: Text.AlignRight
                color: Kirigami.Theme.textColor
                text: {
                    var m = Math.floor(EoSSoundBackend.playerPosition / 60000);
                    var ms = (EoSSoundBackend.playerPosition / 1000 - m * 60).toFixed(1);
                    return `${m}:${ms.padStart(4, 0)}`;
                }
            }

            Controls.ProgressBar {
                id: playerSlider

                value: EoSSoundBackend.playerPosition / EoSSoundBackend.playerDuration
                to: 1
                indeterminate: false
                Layout.fillWidth: true
            }

            Text {
                horizontalAlignment: Text.AlignRight
                color: Kirigami.Theme.textColor
                text: {
                    var m = Math.floor(EoSSoundBackend.playerDuration / 60000);
                    var ms = (EoSSoundBackend.playerDuration / 1000 - m * 60).toFixed(1);
                    return `${m}:${ms.padStart(4, 0)}`;
                }
            }

        }

    }

    footer: Kirigami.ActionToolBar {
        actions: [
            Kirigami.Action {

                displayComponent: EoSSpinBox {
                    label: i18n("Time Window")
                    unit: i18n("s")
                    decimals: 2
                    stepSize: 0.001
                    from: 0.001
                    to: 3600
                    value: EoSdb.chartTimeWindow
                    onValueModified: (v) => {
                        EoSdb.chartTimeWindow = v;
                    }
                }

            },
            Kirigami.Action {
                text: i18n("Save Charts")
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
                    chartWaveForm.zoomReset();
                    chartFFT.zoomReset();
                }
            }
        ]
    }

}
