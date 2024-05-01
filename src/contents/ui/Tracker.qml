import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: tracker

    title: i18n("Tracker")
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
}
