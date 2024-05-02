import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: soundWave

    title: i18n("Sound Wave")
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
