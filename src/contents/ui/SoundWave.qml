import EoSdb
import QtCharts
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: soundWave

    title: i18n("Sound Wave")
    actions: [
        Kirigami.Action {
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
                    title: i18n("WaveForm")

                    ValueAxis {
                        id: axisTime

                        labelFormat: "%.1f"
                        // min: EoSTrackerBackend.xAxisMin
                        // max: EoSTrackerBackend.xAxisMax
                        titleText: i18n("Time [s]")
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

                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.textColor
                    text: {
                        return `x = ${mouseAreaWaveForm.mouseX.toFixed(1)} \t y = ${mouseAreaWaveForm.mouseY.toFixed(1)}`;
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
                    title: i18n("Fourier Transform")

                    ValueAxis {
                        id: axisFrequency

                        labelFormat: "%.1f"
                        // min: EoSTrackerBackend.xAxisMin
                        // max: EoSTrackerBackend.xAxisMax
                        titleText: i18n("Frequency [Hz]")
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

                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.textColor
                    text: {
                        return `x = ${mouseAreaFFT.mouseX.toFixed(1)} \t y = ${mouseAreaFFT.mouseY.toFixed(1)}`;
                    }
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
