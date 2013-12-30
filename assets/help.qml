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
	Page {
        titleBar: TitleBar {
	        title: qsTr("Help")
            dismissAction: ActionItem {
                title: "Back"
                onTriggered: {
                    mySheet.close();
                }
            }
	    }
	    
	    ScrollView {
	        scrollViewProperties.pinchToZoomEnabled: false
	        Container {    
		        	        
		        Header {
		            title: qsTr("Tips")
		
		        }
		        TextArea {
		            text: qsTr("#1 Connecting to the GoPro Camera:\nTo connect to the GoPro, please enter the settings page, and enter the Wifi name (also known as SSID) of your camera, and also your password, that you defined when installing your camera with the vendor software. Then check connection, you should see wifi OK (means the SSID is OK), and also connection OK (means your password is OK). Then you are ready to use this App.")
	                backgroundVisible: false
		            textFormat: TextFormat.Html
		            editable: false
		
		        }
		        TextArea {
		            text: qsTr("#2 Sometimes, the Wifi connection can be unstable, or maybe your phone may try to catch other networks while in range. If you have issue, please check that your BlackBerry 10 phone is properly connected to the GoPro wifi network.")
		            backgroundVisible: false
		            textFormat: TextFormat.Html
		            editable: false
		
		        }
                TextArea {
                    text: qsTr("#3 Wifi is very power hungry ! Be careful not to drain your battery of both your phone and camera by playing with this App :) !")
                    backgroundVisible: false
                    textFormat: TextFormat.Html
                    editable: false
                
                }
                
                Header {
                    title: qsTr("Credits")
                
                }
                TextArea {
                    text: qsTr("LightGuru was designed and programmed by Gael Jaffrain.\nWebsite: <a href=\"http://gaeljaffrain.com\">gaeljaffrain.com</a>\n\nGoPro (TM) is a registered trademark by Woodman Labs, Inc. They design this great camera, and I do not have any link or affiliation with Woodman Labs. If you need support or have questions regarding the camera itself, please go to http://gopro.com/")
                    backgroundVisible: false
                    textFormat: TextFormat.Html
                    editable: false
                
                }
		    }
	
	    }
	}
}