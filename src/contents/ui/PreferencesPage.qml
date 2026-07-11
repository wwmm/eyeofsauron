import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    FormCard.FormHeader {
        title: i18n("General")
    }

    FormCard.FormCard {
        EoSSwitch {
            id: showTrayIcon

            label: i18n("Show the Tray Icon")
            isChecked: DbMain.showTrayIcon
            onCheckedChanged: {
                if (isChecked !== DbMain.showTrayIcon)
                    DbMain.showTrayIcon = isChecked;
            }
        }

        EoSSwitch {
            id: darkChartTheme

            label: i18n("Dark Chart Theme")
            isChecked: DbMain.darkChartTheme
            onCheckedChanged: {
                if (isChecked !== DbMain.darkChartTheme)
                    DbMain.darkChartTheme = isChecked;
            }
        }

        EoSSwitch {
            id: chartsUseOpenGL

            label: i18n("Charts Use OpenGL Acceleration")
            isChecked: DbMain.chartsUseOpenGL
            onCheckedChanged: {
                if (isChecked !== DbMain.chartsUseOpenGL)
                    DbMain.chartsUseOpenGL = isChecked;
            }
        }

        EoSSpinBox {
            label: i18n("Table File Precision")
            decimals: 0
            stepSize: 1
            from: 0
            to: 10
            value: DbMain.tableFilePrecision
            onValueModified: v => {
                DbMain.tableFilePrecision = v;
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
            currentIndex: DbMain.trackingAlgorithm
            editable: false
            model: ["KCF", "MOSSE", "TLD", "MIL"]
            onActivated: idx => {
                if (idx !== DbMain.trackingAlgorithm)
                    DbMain.trackingAlgorithm = idx;
            }
        }

        FormCard.FormComboBoxDelegate {
            id: imageScalingAlgorithm

            text: i18n("Image Scaling Algorithm")
            displayMode: FormCard.FormComboBoxDelegate.ComboBox
            currentIndex: DbMain.imageScalingAlgorithm
            editable: false
            model: [i18n("Fast"), i18n("Smooth")]
            onActivated: idx => {
                if (idx !== DbMain.imageScalingAlgorithm)
                    DbMain.imageScalingAlgorithm = idx;
            }
        }

        EoSSwitch {
            id: showDateTime

            label: i18n("Show Date and Time")
            isChecked: DbMain.showDateTime
            onCheckedChanged: {
                if (isChecked !== DbMain.showDateTime)
                    DbMain.showDateTime = isChecked;
            }
        }

        EoSSwitch {
            id: showFps

            label: i18n("Show FPS")
            isChecked: DbMain.showFps
            onCheckedChanged: {
                if (isChecked !== DbMain.showFps)
                    DbMain.showFps = isChecked;
            }
        }

        EoSSpinBox {
            label: i18n("Video Width")
            unit: i18n("px")
            decimals: 0
            stepSize: 1
            from: 40
            to: 1920
            value: DbMain.videoWidth
            onValueModified: v => {
                DbMain.videoWidth = v;
            }
        }

        EoSSpinBox {
            label: i18n("Video Height")
            unit: i18n("px")
            decimals: 0
            stepSize: 1
            from: 30
            to: 1080
            value: DbMain.videoHeight
            onValueModified: v => {
                DbMain.videoHeight = v;
            }
        }
    }
}
