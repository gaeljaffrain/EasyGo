/*
 * Copyright 2013 Gael Jaffrain
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import bb.cascades 1.0

Sheet {
    id: mySheet
    property string savedPassword:_EasyGoApp.getValueFor(password.objectName, "")
    property string savedWifiName:_EasyGoApp.getValueFor(wifiName.objectName, "")

    
	Page {
        titleBar: TitleBar {
            title: qsTr("Settings")
            dismissAction: ActionItem {
                title: "Cancel"
                onTriggered: {
                    // Restore saved settings, and close the page
                    
                    _EasyGoApp.saveValueFor(wifiName.objectName, savedWifiName);
                    _EasyGoApp.saveValueFor(password.objectName, savedPassword);                            
                    _EasyGoApp.updateGoProSettings();  
                    _EasyGoApp.onCheckConnection();
                    mySheet.close();
                }
            }
            acceptAction: ActionItem {
                title: "Save"
                onTriggered: {
                    // Save settings and then close the page
                    
                    _EasyGoApp.saveValueFor(wifiName.objectName, wifiName.text);                    
                    _EasyGoApp.saveValueFor(password.objectName, password.text);
                    _EasyGoApp.updateGoProSettings();
                    _EasyGoApp.onCheckConnection();
                    mySheet.close();
                }
            }
        }
        
        ScrollView {
            scrollViewProperties.pinchToZoomEnabled: false
        
		    Container {
		        id: main
	
	            //Spacer
	            Container {
	                topPadding: 10.0
	            }
	
	            // Camera Wifi Name
	            Header {
	                title: qsTr("GoPro Wifi Name")
	                leftMargin: 10.0
	            }
	            
	            Container {
	                layout: StackLayout {
	                }
	                horizontalAlignment: HorizontalAlignment.Fill
	                leftPadding: 10.0
	                topPadding: 10.0
	                rightPadding: 10.0
	                bottomPadding: 10.0
	                
	                TextField {
	                    id: wifiName
	                    objectName: "wifiName"
	                    text: savedWifiName
	                }
	            }        
	
				//Spacer
				Container {
	                topPadding: 10.0
	            }
	
	            // Wifi status
	            Header {
	                title: qsTr("Wifi Status")
	                topMargin: 0.0
	
	            }
	            Container {
	                layout: DockLayout {	
	                }
	                leftPadding: 10.0
	                topPadding: 10.0
	                rightPadding: 10.0
	                bottomPadding: 10.0
	
	                horizontalAlignment: HorizontalAlignment.Fill
	                Button {
	                    id: checkWifi
	                    text: "Check again"
	                    onClicked: {
	                        wifiStatusLabel.text = qsTr("waiting update ...");
	                        
	                        // We save wifi name temporary, to check connection with the new password
	                        if (savedWifiName != wifiName.text) {
	                            _EasyGoApp.saveValueFor(wifiName.objectName, wifiName.text);
	                            _EasyGoApp.updateGoProSettings();
	                        }
	                        
	                        if (_EasyGoApp.wifiConnected){
	                            wifiStatusLabel.text = qsTr("Connected");
	                        }
	                        else {
	                            wifiStatusLabel.text = qsTr("Not Connected");
	                        }   
	                    }
	                }
	                Label {
	                    id: wifiStatusLabel
	                    text: qsTr("waiting update ...")
	                    onCreationCompleted: {
	                    	checkWifi.clicked()
	                    }
	                    verticalAlignment: VerticalAlignment.Center
	                    horizontalAlignment: HorizontalAlignment.Right
	
	                }
	            }
	            TextArea {
	                text: qsTr("Note: Please check in BlackBerry 10 settings that the network name, or SSID, is the GoPro camera one.")
	                enabled: true
	                editable: false
	            }
	            
	            //Spacer
	            Container {
	                topPadding: 30.0
	            }
	
		        // Camera Password
	            Header {
	                title: qsTr("GoPro Password")
	            }
		        
		        Container {
		            layout: StackLayout {	
		            }
		            horizontalAlignment: HorizontalAlignment.Fill
		            leftPadding: 10.0
		            topPadding: 10.0
		            rightPadding: 10.0
		            bottomPadding: 10.0
	
	                TextField {
		                id: password
		                objectName: "password"
	                    inputMode: TextFieldInputMode.Password
	                    text: savedPassword
	                }
		        }
		        
	            //Spacer
	            Container {
	                topPadding: 10.0
	            }
	            
		        // Camera Connection Test
		        Header {
	            	title: qsTr("GoPro Connection")
	            }
	            Container {
	                layout: DockLayout {	
	                }
	                leftPadding: 10.0
	                topPadding: 10.0
	                rightPadding: 10.0
	                bottomPadding: 10.0
	                
	                horizontalAlignment: HorizontalAlignment.Fill
	                Button {
	                    id: checkGoPro
	                    text: "Check again"
	                    enabled: !_EasyGoApp.goProBusy
	                    onClicked: {
	                        
	                        // We save password temporary, to check connection with the new password
	                        if (savedPassword != password.text) {
	                            _EasyGoApp.saveValueFor(password.objectName, password.text);
	                            _EasyGoApp.updateGoProSettings(); 	//Force C++ to read settings from QSettings and update member variables
	                        }
	                        
	                        _EasyGoApp.onCheckConnection();                        
	                    }
	                }
	                Label {
	                    id: goProStatusLabel
	                    text: _EasyGoApp.goProBusy ? "waiting update" : (_EasyGoApp.goProConnected ? "Connected" : "Not Connected")
	                    verticalAlignment: VerticalAlignment.Center
	                    horizontalAlignment: HorizontalAlignment.Right
	                }
	            }
	        }
	    }
    }
}
