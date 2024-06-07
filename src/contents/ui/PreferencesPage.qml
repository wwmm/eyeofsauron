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

    FormCard.FormHeader {
        title: i18n("Tracker")
    }

    FormCard.FormCard {
        FormCard.FormComboBoxDelegate {
            id: trackingAlgorithm

            text: i18n("Tracking Algorithm")
            displayMode: FormCard.FormComboBoxDelegate.ComboBox
            currentIndex: EoSdb.trackingAlgorithm
            editable: false
            model: ["KCF", "MOSSE", "TLD", "MIL"]
            onActivated: (idx) => {
                if (idx !== EoSdb.trackingAlgorithm)
                    EoSdb.trackingAlgorithm = idx;

            }
        }

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

        EoSSpinBox {
            label: i18n("Video Width")
            unit: i18n("px")
            decimals: 0
            stepSize: 1
            from: 640
            to: 1920
            value: EoSdb.videoWidth
            onValueModified: (v) => {
                EoSdb.videoWidth = v;
            }
        }

        EoSSpinBox {
            label: i18n("Video Height")
            unit: i18n("px")
            decimals: 0
            stepSize: 1
            from: 480
            to: 1080
            value: EoSdb.videoHeight
            onValueModified: (v) => {
                EoSdb.videoHeight = v;
            }
        }

    }

}
