pragma ComponentBehavior: Bound
import QtGraphs
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: widgetRoot

    property int colorScheme: GraphsTheme.ColorScheme.Dark
    property int colorTheme: GraphsTheme.Theme.QtGreenNeon
    property int xAxisDecimals: 0
    property int yAxisDecimals: 0
    property bool logarithmicHorizontalAxis: true
    property bool logarithmicVerticalAxis: false
    property bool dynamicXScale: true
    property bool dynamicYScale: true
    property real xMin: 0
    property real xMax: 1
    property real yMin: 0
    property real yMax: 1
    property real yDataOffset: 0
    property string xUnit: ""
    property string yUnit: ""
    property int pointsCount: 0

    readonly property real xMinLog: Math.log10(xMin)
    readonly property real xMaxLog: Math.log10(xMax)
    readonly property real yMinLog: Math.log10(yMin)
    readonly property real yMaxLog: Math.log10(yMax)
    readonly property color backgroundRectColor: Kirigami.Theme.backgroundColor
    readonly property int targetTicks: Math.max(2, Math.floor(width / (Kirigami.Units.gridUnit * 6)))
    readonly property int coordLabelOffset: Kirigami.Units.smallSpacing

    implicitHeight: columnLayout.implicitHeight
    implicitWidth: columnLayout.implicitWidth
    Kirigami.Theme.colorSet: Kirigami.Theme.View

    readonly property list<real> linearTicks: {
        const step = (xMax - xMin) / targetTicks;

        const start = xMin;

        const ticks = [];

        for (let v = start; v <= xMax; v += step) {
            ticks.push(v);
        }

        return ticks;
    }

    readonly property list<real> logTicks: {
        const step = (xMaxLog - xMinLog) / targetTicks;

        const ticks = [];

        for (let p = xMinLog; p <= xMaxLog; p += step) {
            ticks.push(Math.pow(10, p));
        }

        return ticks;
    }

    Component {
        id: splineComponent

        SplineSeries {}
    }

    function addTrackerSeries(axisName: string) {
        let name = axisName + Math.floor(graph.seriesList.length / 2);

        let series = splineComponent.createObject(graph, {
            name: name
        });

        graph.addSeries(series);

        // if (axisName === "x")
        //     series.visible = Qt.binding(function () {
        //         return actionViewXdata.showgraph;
        //     });
        // else if (axisName === "y")
        //     series.visible = Qt.binding(function () {
        //         return actionViewYdata.showChart;
        //     });
    }

    function removeSeries(index: int) {
        if (index >= 0 && index < graph.seriesList.length) {
            graph.removeSeries(index);
        }
    }

    function removeAllSeries() {
        while (graph.seriesList.length > 0) {
            graph.removeSeries(0);
        }
    }

    function getSeriesCount(): int {
        return graph.seriesList.length;
    }

    function getSeries(index: int): SplineSeries {
        if (index >= 0 && index < graph.seriesList.length) {
            return graph.seriesList[index];
        }

        return null;
    }

    function updateData(inputData: list<point>) {
        if (!inputData || inputData.length === 0) {
            return;
        }

        let minX = Number.POSITIVE_INFINITY;
        let maxX = Number.NEGATIVE_INFINITY;
        let minY = Number.POSITIVE_INFINITY;
        let maxY = Number.NEGATIVE_INFINITY;

        let processedData = [];

        for (let n = 0; n < inputData.length; n++) {
            let y = inputData[n].y + yDataOffset;
            let x = inputData[n].x;

            if (!Number.isFinite(x) || !Number.isFinite(y)) {
                continue;
            }

            if ((widgetRoot.logarithmicHorizontalAxis && x <= 0) || (widgetRoot.logarithmicVerticalAxis && y <= 0)) {
                continue;
            }

            const pointX = widgetRoot.logarithmicHorizontalAxis ? Math.log10(x) : x;
            const pointY = widgetRoot.logarithmicVerticalAxis ? Math.log10(y) : y;

            if (!Number.isFinite(pointX) || !Number.isFinite(pointY)) {
                continue;
            }

            minX = Math.min(minX, x);
            maxX = Math.max(maxX, x);
            minY = Math.min(minY, y);
            maxY = Math.max(maxY, y);

            processedData.push(Qt.point(pointX, pointY));
        }

        if (processedData.length === 0) {
            return;
        }

        widgetRoot.pointsCount = processedData.length;

        // Avoid tiny changes triggering expensive relayouts
        const epsilon = 0.01; // 1% threshold
        const xRange = (maxX - minX) || 1;
        const yRange = (maxY - minY) || 1;

        if (dynamicXScale) {
            if (Math.abs(minX - xMin) / xRange > epsilon) {
                xMin = minX;
            }

            if (Math.abs(maxX - xMax) / xRange > epsilon) {
                xMax = maxX;
            }
        } else {
            const newXMin = Math.min(minX, xMin);
            const newXMax = Math.max(maxX, xMax);

            if (Math.abs(newXMin - xMin) / xRange > epsilon) {
                xMin = newXMin;
            }

            if (Math.abs(newXMax - xMax) / xRange > epsilon) {
                xMax = newXMax;
            }
        }

        if (dynamicYScale) {
            if (Math.abs(minY - yMin) / yRange > epsilon) {
                yMin = minY;
            }

            if (Math.abs(maxY - yMax) / yRange > epsilon) {
                yMax = maxY;
            }
        } else {
            const newYMin = Math.min(minY, yMin);
            const newYMax = Math.max(maxY, yMax);

            if (Math.abs(newYMin - yMin) / yRange > epsilon) {
                yMin = newYMin;
            }
            if (Math.abs(newYMax - yMax) / yRange > epsilon) {
                yMax = newYMax;
            }
        }

        // if (splineSeries.visible === true) {
        //     splineSeries.replace(processedData);
        // }
    }

    function mapToValueX(mouseX: real): real {
        if (graph.plotArea.width <= 0) {
            return 0;
        }

        const normalizedX = (mouseX - graph.plotArea.x) / graph.plotArea.width;

        if (logarithmicHorizontalAxis) {
            return Math.pow(10, horizontalAxis.min + normalizedX * (horizontalAxis.max - horizontalAxis.min));
        } else {
            return horizontalAxis.min + normalizedX * (horizontalAxis.max - horizontalAxis.min);
        }
    }

    function mapToValueY(mouseY: real): real {
        if (graph.plotArea.height <= 0) {
            return 0;
        }

        const normalizedY = 1 - (mouseY - graph.plotArea.y) / graph.plotArea.height;

        if (logarithmicVerticalAxis) {
            return Math.pow(10, verticalAxis.min + normalizedY * (verticalAxis.max - verticalAxis.min));
        } else {
            return verticalAxis.min + normalizedY * (verticalAxis.max - verticalAxis.min);
        }
    }

    GraphsTheme {
        id: qtTheme

        colorScheme: widgetRoot.colorScheme
        theme: widgetRoot.colorTheme

        // Component.onCompleted: {
        //     console.log("plot area: " + plotAreaBackgroundColor);
        //     console.log("backgroundColor: " + backgroundColor);
        //     console.log("seriesColors: " + seriesColors);
        //     console.log("labelTextColor: " + labelTextColor);
        //     console.log("labelBackgroundColor: " + labelBackgroundColor);
        //     console.log("borderColors: " + borderColors);
        // }
    }

    ColumnLayout {
        id: columnLayout

        anchors.fill: parent
        spacing: 0

        GraphsView {
            id: graph

            antialiasing: true
            marginBottom: 0
            marginTop: 0
            marginLeft: 0
            marginRight: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            axisX: horizontalAxis

            ValueAxis {
                id: horizontalAxis
                labelFormat: "%.1f"
                min: widgetRoot.logarithmicHorizontalAxis !== true ? widgetRoot.xMin : widgetRoot.xMinLog
                max: widgetRoot.logarithmicHorizontalAxis !== true ? widgetRoot.xMax : widgetRoot.xMaxLog
                // gridVisible: DbGraph.gridVisible
                // subGridVisible: DbGraph.gridVisible
                visible: true
                lineVisible: true
                titleVisible: true
                labelDecimals: 0
            }

            axisY: ValueAxis {
                id: verticalAxis
                labelFormat: "%.1e"
                // gridVisible: DbGraph.gridVisible
                // subGridVisible: DbGraph.gridVisible
                visible: true
                lineVisible: true
                labelsVisible: true
                titleVisible: true
                min: widgetRoot.logarithmicVerticalAxis !== true ? widgetRoot.yMin : widgetRoot.yMinLog
                max: widgetRoot.logarithmicVerticalAxis !== true ? widgetRoot.yMax : widgetRoot.yMaxLog
            }

            theme: qtTheme
        }
    }

    // Coordinate display label
    Controls.Label {
        id: coordinateLabel
        visible: false
        padding: Kirigami.Units.smallSpacing
        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.Theme.textColor
            border.width: 1
            radius: Kirigami.Units.smallSpacing
            opacity: 0.9
        }
        color: Kirigami.Theme.textColor
        font.pointSize: Kirigami.Theme.smallFont.pointSize
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: function (mouse) {
            // Calculating the label x coordinate

            let labelX = mouse.x + widgetRoot.coordLabelOffset;

            if (labelX + coordinateLabel.width > widgetRoot.width) {
                labelX = widgetRoot.width - coordinateLabel.width - widgetRoot.coordLabelOffset;
            } else if (x < 0) {
                labelX = widgetRoot.coordLabelOffset;
            }

            coordinateLabel.x = labelX;

            // Calculating the y coordinate

            let labelY = mouse.y - coordinateLabel.height - widgetRoot.coordLabelOffset;

            if (labelY < 0) {
                labelY = widgetRoot.coordLabelOffset;
            } else if (labelY + coordinateLabel.height > widgetRoot.height) {
                labelY = widgetRoot.height - coordinateLabel.height - widgetRoot.coordLabelOffset;
            }

            coordinateLabel.y = labelY;

            const dataX = widgetRoot.mapToValueX(mouse.x);
            const dataY = widgetRoot.mapToValueY(mouse.y) - widgetRoot.yDataOffset;

            // const newText = `${Number(dataX).toLocaleString(Qt.locale(), 'f', widgetRoot.xAxisDecimals)} ${widgetRoot.xUnit}`;
            const newText = `x: ${Number(dataX).toLocaleString(Qt.locale(), 'f', widgetRoot.xAxisDecimals)} ${widgetRoot.xUnit}, y: ${Number(dataY).toLocaleString(Qt.locale(), 'f', widgetRoot.yAxisDecimals)} ${widgetRoot.yUnit}`;

            if (coordinateLabel.text !== newText) {
                coordinateLabel.text = newText;
            }

            coordinateLabel.visible = true;
        }
        onExited: {
            coordinateLabel.visible = false;
        }
    }
}
