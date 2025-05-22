/**
 *  OSM
 *  Copyright (C) 2022  Pavel Smokotnin

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.13
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2

Item {

    ColumnLayout {
        anchors.fill: parent

        RowLayout {

            Button {
                checkable: true
                text: qsTr("Server")
                checked: remoteServer.active
                Material.background: parent.Material.background
                onCheckedChanged: {
                    remoteServer.active = checked;
                }
            }

            Button {
                checkable: true
                text: qsTr("Generator")
                checked: remoteServer.generatorEnable
                Material.background: parent.Material.background
                onCheckedChanged: {
                    remoteServer.generatorEnable = checked;
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("enable for remote")
            }

            Label {
                Layout.fillWidth: true
                text: remoteServer.lastConnected ? "Last connected client: <b>" + remoteServer.lastConnected + "</b>" : ""
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.RichText
            }

            Button {
                text: qsTr("About remote control")
                Layout.preferredWidth: 200
                Material.background: parent.Material.background

                onClicked:  {
                    Qt.openUrlExternally("https://opensoundmeter.com/api");
                }
            }
        }

        RowLayout {
            Button {
                checkable: true
                text: qsTr("Client")
                Material.background: parent.Material.background
                checked: remoteClient.active
                onCheckedChanged: {
                    remoteClient.active = checked;
                }
            }

            Button {
                font.family: "Osm"
                font.bold: false
                text: "\ue808"

                Material.background: parent.Material.background
                onClicked: remoteClient.reset()
            }

            Button {
                checkable: true
                text: qsTr("Rest Api")
                Material.background: parent.Material.background
                checked: restApi.active
                onCheckedChanged: {
                    restApi.active = checked
                }
            }
            TextField {
                id: portField
                placeholderText: restApi.port
                ToolTip.visible: hovered
                ToolTip.text: qsTr("rest api port 1 - 65535")
                Layout.preferredWidth: 50
                enabled: !restApi.active
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {}

                selectByMouse: true

                onFocusChanged: {
                    if (focus) {
                        selectAll()
                    } else {
                        let val = parseInt(text);
                        if (isNaN(val) || val < 0 || val > 65535) {
                            restApi.port = 49008
                            text = ""
                            placeholderText: 49008
                        } else {
                            restApi.port = val
                        }
                    }
                }

                onTextChanged: {
                    if (text.trim().length === 0) {
                        placeholderText: 49008
                    }
                }

                Keys.onEscapePressed: {
                    focus = false
                }
                Keys.onReturnPressed: {
                    focus = false
                }
            }

            CheckBox {
                text: qsTr("enable api on startup")
                checked: restApi.startup
                onToggled: {
                    if (checked !==  restApi.startup) {
                        restApi.startup = checked
                    }
                }
            }

            CheckBox {
                text: qsTr("enable server on startup")
                checked: remoteServer.startup
                onToggled: {
                    if (checked !== remoteServer.startup) {
                        remoteServer.startup = checked
                    }
                }
            }
        }
    }
}
