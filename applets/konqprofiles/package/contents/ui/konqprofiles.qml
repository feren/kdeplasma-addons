/*****************************************************************************
*   Copyright (C) 2011, 2012 by Shaun Reich <shaun.reich@kdemail.net>        *
*                                                                            *
*   This program is free software; you can redistribute it and/or            *
*   modify it under the terms of the GNU General Public License as           *
*   published by the Free Software Foundation; either version 2 of           *
*   the License, or (at your option) any later version.                      *
*                                                                            *
*   This program is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*   GNU General Public License for more details.                             *
*                                                                            *
*   You should have received a copy of the GNU General Public License        *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*****************************************************************************/

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents

Item {
    id: konqProfiles

    property int minimumWidth: 200
    property int minimumHeight: 300

    PlasmaCore.DataSource {
        id: profilesSource
        engine: "org.kde.konqprofiles"
        onSourceAdded: connectSource(source)
        onSourceRemoved: disconnectSource(source)

        Component.onCompleted: connectedSources = sources
    }

    Component.onCompleted: {
        plasmoid.popupIcon = "konqueror";
        plasmoid.aspectRatioMode = IgnoreAspectRatio;
    }


    PlasmaComponents.Label {
        id: header
        text: i18n("Konqueror Profiles")
        anchors { top: parent.top; left: parent.left; right: parent.right }
        horizontalAlignment: Text.AlignHCenter
    }

    PlasmaCore.Svg {
        id: lineSvg
        imagePath: "widgets/line"
    }

    PlasmaCore.SvgItem {
        id: separator

        anchors { top: header.bottom; left: parent.left; right: parent.right }
        anchors { topMargin: 3 }

        svg: lineSvg
        elementId: "horizontal-line"
        height: lineSvg.elementSize("horizontal-line").height
    }


    ListView {
        id: profileView

        anchors {
            top : scrollBar.top
            topMargin: 10
            bottom: konqProfiles.bottom
            left: parent.left
            leftMargin: 10
            right: scrollBar.left
            rightMargin: 10
        }

        model: PlasmaCore.DataModel {
            dataSource: profilesSource
        }

        delegate: profileViewDelegate
        highlight: profileViewHighlighter
        highlightMoveDuration: 250
        highlightMoveSpeed: 1
        clip: true

        Component.onCompleted: currentIndex = -1
    }

    PlasmaComponents.ScrollBar {
        id: scrollBar

        anchors {
            top: separator.bottom
            right: parent.right
            bottom: parent.bottom
        }

        orientation: Qt.Vertical
        flickableItem: profileView
        stepSize: 40
        scrollButtonInterval: 50
    }

    //we use this to compute a fixed height for the items, and also to implement
    //the said constant below (itemHeight)
    Text {
        id: heightMetric
        visible: false
        text: "Arbitrary String"
    }

    property int itemHeight: heightMetric.height * 2
    Component {
        id: profileViewDelegate

        Item {
            height: itemHeight
            anchors { left: parent.left; leftMargin: 10; right: parent.right }

            Text {
                id: text
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                text: model.prettyName
            }

            MouseArea {
                height: itemHeight
                anchors { left: parent.left; right: parent.right }
                hoverEnabled: true

                onClicked: {
                    var service = profilesSource.serviceForSource(model["DataEngineSource"])
                    var operation = service.operationDescription("open")
                    var job = service.startOperationCall(operation)
                }

                onEntered: {
                    profileView.currentIndex = index
                    profileView.highlightItem.opacity = 1
                }

                onExited: {
                    profileView.highlightItem.opacity = 0
                }
            }
        }
    }

    Component {
        id: profileViewHighlighter

        PlasmaCore.FrameSvgItem {
            imagePath: "widgets/viewitem"
            prefix: "hover"
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }
    }
}