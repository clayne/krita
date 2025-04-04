/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12
import org.krita.flake.text 1.0

CollapsibleGroupProperty {
    propertyName: i18nc("@title:group", "Text Align");
    propertyType: TextPropertyBase.Paragraph;
    toolTip: i18nc("@info:tooltip",
                   "Text Align sets the alignment for the given block of characters.");
    searchTerms: i18nc("comma separated search terms for the text-anchor property, matching is case-insensitive",
                       "text-align, justification, text-anchor");

    property int textAlignAll: 1;
    property int textAlignLast: 0;
    property int textAnchor: 0;

    onPropertiesUpdated: {
        blockSignals = true;
        textAlignAll = properties.textAlignAll;
        textAlignLast = properties.textAlignLast;
        textAnchor = properties.textAnchor;
        visible = properties.textAlignAllState !== KoSvgTextPropertiesModel.PropertyUnset ||
                properties.textAlignLastState !== KoSvgTextPropertiesModel.PropertyUnset ||
                properties.textAnchorState !== KoSvgTextPropertiesModel.PropertyUnset;
        blockSignals = false;
    }

    onTextAlignAllChanged: {
        textAlignAllCmb.currentIndex = textAlignAllCmb.indexOfValue(textAlignAll);
        if (!blockSignals) {
            properties.textAlignAll = textAlignAll;
        }
    }

    onTextAlignLastChanged: {
        textAlignLastCmb.currentIndex = textAlignLastCmb.indexOfValue(textAlignLast);
        if (!blockSignals) {
            properties.textAlignLast = textAlignLast;
        }
    }

    onTextAnchorChanged: {
        textAnchorCmb.currentIndex = textAnchorCmb.indexOfValue(textAnchor);
        if (!blockSignals) {
            properties.textAnchor = textAnchor;
        }
    }

    titleItem: RowLayout {
        width: parent.width;
        height: childrenRect.height;
        Label {
            id: propertyTitle;
            text: propertyName;
            verticalAlignment: Text.AlignVCenter
            color: sysPalette.text;
            elide: Text.ElideRight;
            Layout.maximumWidth: contentWidth;
        }
    }

    onEnableProperty: properties.textAnchorState = KoSvgTextPropertiesModel.PropertySet;

    contentItem: GridLayout {
        columns: 3
        anchors.left: parent.left
        anchors.right: parent.right
        columnSpacing: columnSpacing;

        RevertPropertyButton {
            revertState: properties.textAlignAllState;
            onClicked: properties.textAlignAllState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            text: i18nc("@label:listbox", "Text Align:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter;
            elide: Text.ElideRight;
            Layout.maximumWidth: implicitWidth;
            font.italic: properties.textAlignAllState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        SqueezedComboBox {
            id: textAlignAllCmb;
            model: [
                {text: i18nc("@label:inlistbox", "Left"), value: KoSvgText.AlignLeft, icon: "qrc:///16_light_format-justify-left.svg"},
                {text: i18nc("@label:inlistbox", "Start"), value: KoSvgText.AlignStart, icon: "qrc:///16_light_format-justify-left.svg"},
                {text: i18nc("@label:inlistbox", "Center"), value: KoSvgText.AlignCenter, icon: "qrc:///16_light_format-justify-center.svg"},
                {text: i18nc("@label:inlistbox", "End"), value: KoSvgText.AlignEnd, icon: "qrc:///16_light_format-justify-right.svg"},
                {text: i18nc("@label:inlistbox", "Right"), value: KoSvgText.AlignRight, icon: "qrc:///16_light_format-justify-right.svg"},
                {text: i18nc("@label:inlistbox", "Justified"), value: KoSvgText.AlignJustify, icon: "qrc:///16_light_format-justify-fill.svg"}
            ]
            Layout.fillWidth: true
            textRole: "text";
            valueRole: "value";
            iconRole: "icon";
            iconSize: 16;
            onActivated: textAlignAll = currentValue;
            wheelEnabled: true;
        }

        RevertPropertyButton {
            revertState: properties.textAlignLastState;
            onClicked: properties.textAlignLastState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            text: i18nc("@label:listbox", "Align Last:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter;
            elide: Text.ElideRight;
            Layout.preferredWidth: implicitWidth;
            font.italic: properties.textAlignLastState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        ComboBox {
            id: textAlignLastCmb;
            model: [
                {text: i18nc("@label:inlistbox", "Auto"), value: KoSvgText.AlignLastAuto},
                {text: i18nc("@label:inlistbox", "Left"), value: KoSvgText.AlignLeft},
                {text: i18nc("@label:inlistbox", "Start"), value: KoSvgText.AlignStart},
                {text: i18nc("@label:inlistbox", "Center"), value: KoSvgText.AlignCenter},
                {text: i18nc("@label:inlistbox", "End"), value: KoSvgText.AlignEnd},
                {text: i18nc("@label:inlistbox", "Right"), value: KoSvgText.AlignRight}
            ]
            Layout.fillWidth: true
            textRole: "text";
            valueRole: "value";
            onActivated: textAlignLast = currentValue;
            wheelEnabled: true;
        }


        RevertPropertyButton {
            revertState: properties.textAnchorState;
            onClicked: properties.textAnchorState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            text: i18nc("@label:listbox", "Text Anchor:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter;
            elide: Text.ElideRight;
            Layout.preferredWidth: implicitWidth;
            font.italic: properties.textAnchorState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        SqueezedComboBox {
            id: textAnchorCmb;
            model: [
                {text: i18nc("@label:inlistbox", "Start"), value: KoSvgText.AnchorStart, icon: "qrc:///16_light_format-justify-left.svg"},
                {text: i18nc("@label:inlistbox", "Middle"), value: KoSvgText.AnchorMiddle, icon: "qrc:///16_light_format-justify-center.svg"},
                {text: i18nc("@label:inlistbox", "End"), value: KoSvgText.AnchorEnd, icon: "qrc:///16_light_format-justify-right.svg"}]
            Layout.fillWidth: true
            textRole: "text";
            valueRole: "value";
            iconRole: "icon";
            iconSize: 16;
            onActivated: textAnchor = currentValue;
            wheelEnabled: true;
        }

    }
}

