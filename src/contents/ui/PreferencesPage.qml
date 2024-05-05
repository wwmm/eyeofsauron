import CfgWindow
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    FormCard.FormHeader {
        title: i18n("General")
    }

    FormCard.FormCard {
        EoSSwitch {
            id: showTrayIcon

            label: i18n("Show the Tray Icon")
            isChecked: CfgWindow.showTrayIcon
            onCheckedChanged: {
                if (isChecked !== CfgWindow.showTrayIcon)
                    CfgWindow.showTrayIcon = isChecked;

            }
        }

    }

    FormCard.FormHeader {
        title: i18n("Camera")
    }

    FormCard.FormCard {
    }

}
