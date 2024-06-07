import QtCharts
import QtQuick

MouseArea {
    property ChartView chart
    property Rectangle zoomRect
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
