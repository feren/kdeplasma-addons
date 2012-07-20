/*
 * Copyright 2012  Luís Gabriel Lima <lampih@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.qtextracomponents 0.1 as QtExtraComponents

Item {
   id: root

   property int minimumWidth: 364
   property int minimumHeight: 265

   anchors.fill: parent

   PlasmaCore.FrameSvgItem {
       id: panel
       anchors {
           top: parent.top
           left: parent.left
           right: parent.right
       }
       height: 56 // XXX
       imagePath: "widgets/frame"
       prefix: "plain"

       QtExtraComponents.QIconItem {
           icon: "weather-many-clouds" // XXX
           height: parent.height // XXX
           width: height
       }

       Text {
           id: locationLabel
           anchors {
               top: parent.top
               left: parent.left
               topMargin: 5
               leftMargin: 76
           }
           font.bold: true
           text: "Porto Alegre, Brazil"

           Component.onCompleted: font.pointSize = Math.floor(font.pointSize * 1.4);
       }

       Text {
           id: conditionLabel
           anchors {
               top: locationLabel.bottom
               left: locationLabel.left
               topMargin: 5
           }
           text: "grey cloud"
       }

       Text {
           id: tempLabel
           anchors {
               right: parent.right
               top: locationLabel.top
               rightMargin: 5
           }
           font: locationLabel.font
           text: "22°C"
       }

       Text {
           id: forecastTempsLabel
           anchors {
               right: tempLabel.right
               top: conditionLabel.top
           }
           font.pointSize: 8 // XXX
           text: "H: 27°C L: 21°C"
       }
   }

   PlasmaComponents.TabBar {
       anchors {
           top: panel.bottom
           topMargin: 5
           horizontalCenter: parent.horizontalCenter
       }
       width: 160 // XXX
       height: 30 // XXX

       PlasmaComponents.TabButton {
           text: i18n("%1 Days").arg(3)
       }
       PlasmaComponents.TabButton {
           text: i18n("Details")
       }
   }
}