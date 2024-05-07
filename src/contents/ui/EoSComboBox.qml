/*
    Based on:

    https://github.com/KDE/kirigami-addons/blob/master/src/formcard/AbstractFormDelegate.qml
    https://github.com/KDE/kirigami-addons/blob/master/src/formcard/FormComboBoxDelegate.qml
*/

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates
import org.kde.kirigamiaddons.formcard as FormCard

T.ItemDelegate {
    id: controlRoot

    /**
     * @brief This property holds the value of the current item in the combobox.
     */
    property alias currentValue: combobox.currentValue
    /**
     * @brief This property holds the text of the current item in the combobox.
     *
     * @see displayText
     */
    property alias currentText: combobox.currentText
    /**
     * @brief This property holds the model providing data for the combobox.
     *
     * @see displayText
     * @see QtQuick.Controls.ComboBox.model
     * @see <a href="https://doc.qt.io/qt-6/qtquick-modelviewsdata-modelview.html">Models and Views in QtQuick</a>
     */
    property var model
    /**
     * @brief This property holds the `textRole` of the internal combobox.
     *
     * @see QtQuick.Controls.ComboBox.textRole
     */
    property alias textRole: combobox.textRole
    /**
     * @brief This property holds the `valueRole` of the internal combobox.
     *
     * @see QtQuick.Controls.ComboBox.valueRole
     */
    property alias valueRole: combobox.valueRole
    /**
     * @brief This property holds the `iconRole`
     */
    property string iconRole: combobox.textRole
    /**
     * @brief This property holds the `currentIndex` of the internal combobox.
     *
     * default: `-1` when the ::model has no data, `0` otherwise
     *
     * @see QtQuick.Controls.ComboBox.currentIndex
     */
    property alias currentIndex: combobox.currentIndex
    /**
     * @brief This property holds the `highlightedIndex` of the internal combobox.
     *
     * @see QtQuick.Controls.ComboBox.highlightedIndex
     */
    property alias highlightedIndex: combobox.highlightedIndex
    /**
     * @brief This property holds the `displayText` of the internal combobox.
     *
     * This can be used to slightly modify the text to be displayed in the combobox, for instance, by adding a string with the ::currentText.
     *
     * @see QtQuick.Controls.ComboBox.displayText
     */
    property alias displayText: combobox.displayText
    /**
     * @brief This property holds the `editable` property of the internal combobox.
     *
     * This turns the combobox editable, allowing the user to specify
     * existing values or add new ones.
     *
     * Use this only when ::displayMode is set to
     * FormComboBoxDelegate.ComboBox.
     *
     * @see QtQuick.Controls.ComboBox.editable
     */
    property alias editable: combobox.editable
    /**
     * @brief This property holds the `editText` property of the internal combobox.
     *
     * @see QtQuick.Controls.ComboBox.editText
     */
    property alias editText: combobox.editText
    /**
     * @brief The delegate component to use as entries in the combobox display mode.
     */
    property Component comboBoxDelegate
    property real __indicatorMargin: controlRoot.indicator && controlRoot.indicator.visible && controlRoot.indicator.width > 0 ? controlRoot.spacing + indicator.width + controlRoot.spacing : 0

    /**
     * @brief This signal is emitted when the item at @p index is activated
     * by the user.
     */
    signal activated(int index)
    /**
     * @brief This signal is emitted when the Return or Enter key is pressed
     * while an editable combo box is focused.
     *
     * @see editable
     */
    signal accepted()

    function indexOfValue(value) {
        return combobox.indexOfValue(value);
    }

    horizontalPadding: parent._internal_formcard_margins ? parent._internal_formcard_margins : Kirigami.Units.gridUnit
    verticalPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    leftPadding: horizontalPadding + (!controlRoot.mirrored ? 0 : __indicatorMargin)
    rightPadding: horizontalPadding + (controlRoot.mirrored ? 0 : __indicatorMargin)
    Layout.fillWidth: true
    Accessible.onPressAction: controlRoot.clicked()

    // use connections instead of onClicked on root, so that users can supply
    // their own behaviour.
    Connections {
        function onClicked() {
            combobox.popup.open();
        }

        target: controlRoot
    }

    comboBoxDelegate: Delegates.RoundedItemDelegate {
        implicitWidth: Kirigami.Units.gridUnit * 16
        text: controlRoot.textRole ? (Array.isArray(controlRoot.model) ? modelData[controlRoot.textRole] : model[controlRoot.textRole]) : modelData
        highlighted: controlRoot.currentIndex === index
        icon.name: model[controlRoot.iconRole]
    }

    contentItem: RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        Controls.Label {
            Layout.fillWidth: true
            text: controlRoot.text
            elide: Text.ElideRight
            color: controlRoot.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            Accessible.ignored: true
        }

        Controls.ComboBox {
            id: combobox

            focusPolicy: Qt.NoFocus // provided by parent
            model: controlRoot.model
            delegate: controlRoot.comboBoxDelegate
            currentIndex: controlRoot.currentIndex
            onActivated: (index) => {
                return controlRoot.activated(index);
            }
            onAccepted: controlRoot.accepted()
            popup.contentItem.clip: true
        }

    }

    background: FormCard.FormDelegateBackground {
        control: controlRoot
    }

}
