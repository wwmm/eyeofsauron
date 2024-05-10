import EoSdb
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    FormCard.FormHeader {
        title: i18n("General")
    }

    FormCard.FormCard {
        EoSSwitch {
            id: showTrayIcon

            label: i18n("Show the Tray Icon")
            isChecked: EoSdb.showTrayIcon
            onCheckedChanged: {
                if (isChecked !== EoSdb.showTrayIcon)
                    EoSdb.showTrayIcon = isChecked;

            }
        }

    }

    FormCard.FormHeader {
        title: i18n("Tracker")
    }

    FormCard.FormCard {
        EoSSwitch {
            id: showDateTime

            label: i18n("Show Date and Time")
            isChecked: EoSdb.showDateTime
            onCheckedChanged: {
                if (isChecked !== EoSdb.showDateTime)
                    EoSdb.showDateTime = isChecked;

            }
        }

        EoSSwitch {
            id: showFps

            label: i18n("Show FPS")
            isChecked: EoSdb.showFps
            onCheckedChanged: {
                if (isChecked !== EoSdb.showFps)
                    EoSdb.showFps = isChecked;

            }
        }

        EoSSwitch {
            id: darkChartTheme

            label: i18n("Dark Chart Theme")
            isChecked: EoSdb.darkChartTheme
            onCheckedChanged: {
                if (isChecked !== EoSdb.darkChartTheme)
                    EoSdb.darkChartTheme = isChecked;

            }
        }

    }

}
