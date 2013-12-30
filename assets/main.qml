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

import bb.cascades 1.2
import bb.multimedia 1.2
import my.timer 1.0


TabbedPane {
	id: tabbedPane
    showTabsOnActionBar: false
    
    // Application Menu
    Menu.definition: MenuDefinition {
        // Add a Help action
        helpAction: HelpActionItem {
            onTriggered: {
                var newHelpPage = pageHelp.createObject();
                newHelpPage.open();
            }
        }
        
        // Add a Settings action
        settingsAction: SettingsActionItem {
            onTriggered: {
                var newSettingsPage = pageSettings.createObject();
                newSettingsPage.open();
            }
        }
        
        actions: [
            // BBM integration
            ActionItem {
                title: "Share on BBM"
                imageSource: "asset:///images/ic_bbm.png"
                enabled: _EasyGoApp.allowed;
                
                onTriggered: {
                    _EasyGoApp.sendInvite();
                }
            }    
        ]
    }
    
    attachedObjects: [
        // Help page
        ComponentDefinition {
            id: pageHelp
            source: "help.qml"
        },
        
        // Settings page
        ComponentDefinition {
            id: pageSettings
            source: "settings.qml"
        },
        QTimer {
            id: timer
            interval: 1000  
            onTimeout :{
                if (_EasyGoApp.goProPowered && !_EasyGoApp.goProBusy && tabbedPane.activeTab == controlTab) {
                    _EasyGoApp.onCameraGetStatus(); 
                }    
            }
        }
    ]

	//Core Controls Tab
	Tab {
	    id: controlTab
	    title: "Control"
        content: Page {
            
            // Main Container
            Container {
                layout: DockLayout {

                }

                // Application Background image is stretched to fill the display
                ImageView {
                    imageSource: "asset:///images/carbon_720.jpg"
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill    
                }
                
                //UI container
                Container {
                    leftPadding: 30.0
                    rightPadding: 30.0
                    topPadding: 30.0
                    bottomPadding: 30.0
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: DockLayout {}
                    
                    //Power Group
                    Container {
                        verticalAlignment: VerticalAlignment.Top
                        ToggleButton {
                            id: camPower
                            checked: _EasyGoApp.goProPowered
                            enabled: !_EasyGoApp.goProBusy
                            property bool userTouch: false
                            onTouch: {
                                userTouch = true;
                            }
                            onCheckedChanged: {
                            	if (userTouch) {
                            	    userTouch = false;
                                	_EasyGoApp.onCameraSetPower(camPower.checked);
                            	}
                            }
                        }
                    }
                    
                    Container{
                        verticalAlignment: VerticalAlignment.Center
                        //Mode Group
	                    Container {
	                        id: camModeContainer
	                        property bool userTouch: false
	                        topMargin: 50
	                        bottomMargin: 50
                            enabled: camPower.checked && !_EasyGoApp.goProBusy
	
	                        horizontalAlignment: HorizontalAlignment.Center
	                        SegmentedControl {
	                            id: camMode
	                            selectedIndex: _EasyGoApp.goProSelectedMode
	                            onTouch: {
	                                camModeContainer.userTouch = true;
	                            }
	                            Option {
	                                text: "Video"
	                            }
	                            Option {
	                                text: "Photo"
	                            
	                            }
	                            Option {
	                                text: "Burst"
	                            
	                            }
	                            Option {
	                                text: "T.Lapse"
	                            
	                            }
	                            onSelectedIndexChanged: {
	                                if (camModeContainer.userTouch) {
	                                    camModeContainer.userTouch = false;
	                                    _EasyGoApp.onCameraSetMode(camMode.selectedIndex);
	                                }
	                            }
	                            horizontalAlignment: HorizontalAlignment.Center
	                        }
	                    }
	                    
	                    //Video Status Group
	                    Container {
	                        layout: StackLayout {
	                            orientation: LayoutOrientation.TopToBottom
	                        }
                            visible: _EasyGoApp.goProPowered && (camMode.selectedIndex == 0) && !_EasyGoApp.goProBusy
	                        topMargin: 50
	                        bottomMargin: 50
	                        horizontalAlignment: HorizontalAlignment.Fill
	                        
	                        Container {
	                            layout: DockLayout {}
	                            horizontalAlignment: HorizontalAlignment.Fill
	                            Label {
	                                text: "Videos Count"
	                                textStyle.fontSize: FontSize.Large
	                                verticalAlignment: VerticalAlignment.Bottom
	                            }
	                            Label {
	                                text: _EasyGoApp.goProStatusVideoCount
	                                textStyle.fontSize: FontSize.XLarge
	                                horizontalAlignment: HorizontalAlignment.Right
	                            }   
	                        }
	                        Container {
	                            layout: DockLayout {}
	                            horizontalAlignment: HorizontalAlignment.Fill
	                            Label {
	                                text: "Recording Time"
	                                textStyle.fontSize: FontSize.Large
	                                verticalAlignment: VerticalAlignment.Bottom
	                            }
	                            Label {
	                                text: _EasyGoApp.goProStatusVideoRecTime
	                                textStyle.fontSize: FontSize.XLarge
	                                horizontalAlignment: HorizontalAlignment.Right
	                            }   
	                        }
	                        Container {
	                            layout: DockLayout {}
	                            horizontalAlignment: HorizontalAlignment.Fill
	                            Label {
	                                text: "Remaining Time"
	                                textStyle.fontSize: FontSize.Large
	                                verticalAlignment: VerticalAlignment.Bottom
	                            }
	                            Label {
	                                text: _EasyGoApp.goProStatusVideoRemainingRecTime
	                                textStyle.fontSize: FontSize.XLarge
	                                horizontalAlignment: HorizontalAlignment.Right
	                            }   
	                        } 
	                    }
	                    //Photo Status Group
	                    Container {
	                        layout: StackLayout {
	                            orientation: LayoutOrientation.TopToBottom
	                        }
                            visible: _EasyGoApp.goProPowered && !_EasyGoApp.goProBusy && (camMode.selectedIndex == 1 || camMode.selectedIndex == 2 || camMode.selectedIndex == 3)
	                        topMargin: 50
	                        bottomMargin: 50
	                        horizontalAlignment: HorizontalAlignment.Fill
	                        
	                        Container {
	                            layout: DockLayout {}
	                            horizontalAlignment: HorizontalAlignment.Fill
	                            Label {
	                                text: "Photos Count"
	                                textStyle.fontSize: FontSize.Large
	                                verticalAlignment: VerticalAlignment.Bottom
	                            }
	                            Label {
	                                text: _EasyGoApp.goProStatusPhotoCount
	                                textStyle.fontSize: FontSize.XLarge
	                                horizontalAlignment: HorizontalAlignment.Right
	                            }   
	                        }
	                        Container {
	                            layout: DockLayout {}
	                            horizontalAlignment: HorizontalAlignment.Fill
	                            Label {
	                                text: "Available Shots"
	                                textStyle.fontSize: FontSize.Large
	                                verticalAlignment: VerticalAlignment.Bottom
	                            }
	                            Label {
	                                text: _EasyGoApp.goProStatusPhotoAvailableShots
	                                textStyle.fontSize: FontSize.XLarge
	                                horizontalAlignment: HorizontalAlignment.Right
	                            }   
	                        } 
	                    }
	                    // Create ActivityIndicator and center it
	                    // in the parent container
	                    Container {
		                    horizontalAlignment: HorizontalAlignment.Center
		                    verticalAlignment: VerticalAlignment.Center
		                    Label {
		                        id:loadingText
		                        text: "Waiting Camera"
	                            horizontalAlignment: HorizontalAlignment.Center
	                        }
		                    ActivityIndicator {
		                        id: myIndicator
		                        preferredWidth: 130
		                        preferredHeight: 130
		                        horizontalAlignment: HorizontalAlignment.Center
		                        visible: _EasyGoApp.goProBusy
		                        onVisibleChanged: {
		                            if (_EasyGoApp.goProBusy){
		                                myIndicator.start();
		                            } else {
		                                myIndicator.stop();
		                                loadingText.visible = false;
		                            }
		                        }
		                        onCreationCompleted: {
		                            myIndicator.start();
		                        }
	                        }
		                }
                 	}
                }//UI container
            }//Main Container
            
            //Core Actions
            actions: [
                ActionItem {
                    id: capturePhoto
                    imageSource: "asset:///images/ic_camera_mode.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    title: "Capture"
                    enabled: _EasyGoApp.goProPowered && !_EasyGoApp.goProBusy && (camMode.selectedOption.text == "Photo" || camMode.selectedOption.text == "Burst")
                    onTriggered: {
                        _EasyGoApp.onCameraSetCapture(true);
                    }
                },
                ActionItem {
                    id: startVideo
                    imageSource: "asset:///images/ic_rec_red.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    title: "Start Rec"
                    enabled: _EasyGoApp.goProPowered && !_EasyGoApp.goProBusy && !_EasyGoApp.goProRecording && (camMode.selectedOption.text == "Video" || camMode.selectedOption.text == "T.Lapse")
                    onTriggered: {
                        _EasyGoApp.onCameraSetCapture(true);
                    }
                },
                ActionItem {
                    id: stopVideo
                    imageSource: "asset:///images/ic_stop.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    title: "Stop Rec"
                    enabled: _EasyGoApp.goProPowered && !_EasyGoApp.goProBusy && _EasyGoApp.goProRecording && (camMode.selectedOption.text == "Video" || camMode.selectedOption.text == "T.Lapse")
                    onTriggered: {
                        _EasyGoApp.onCameraSetCapture(false);                        
                    }
                }
            ]
        }//End of Page	
	}//End of Core Controls Tab
	
	//Advanced Controls Tab
	/*Tab {
        id: advancedTab
        title: "Advanced"
        imageSource: "asset:///images/ic_camera_mode.png"
        content: Page {
            
	        // Main Container
	        Container {
	        layout: DockLayout {}
	        
	        	// Application Background image is stretched to fill the display
	        	ImageView {
	        		imageSource: "asset:///images/carbon_720.jpg"
	        		verticalAlignment: VerticalAlignment.Fill
	        		horizontalAlignment: HorizontalAlignment.Fill    
	        	}
	        
	        	//UI container
	        	Container {
	        		Label {
	        			// Localized text with the dynamic translation and locale updates support
	        			text: qsTr("Advanced Controls") + Retranslate.onLocaleOrLanguageChanged
	        			textStyle.base: SystemDefaults.TextStyles.BigText
	        		}
	        	}
	        }   
        }
    }//End of Advanced Controls Tab
	*/
    //Preview Tab
    Tab {
        id: previewTab
        title: "Preview"
        imageSource: "asset:///images/ic_camera_mode.png"
        content: Page {
            id:previewPage
            property bool previewOn: false
            
            // Main Container
            Container {
                layout: DockLayout {
                
                }  
                // Application Background image is stretched to fill the display
                ImageView {
                    imageSource: "asset:///images/carbon_720.jpg"
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill    
                }
                
                //UI container
                Container {
                    layout: AbsoluteLayout {

                    }
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill
                                        
                    ForeignWindowControl {
                        id: vfForeignWindow
                        windowId: "vfForeignWindow"
                        // give the foreign window an objectName so we can look it up from C++
                        objectName: "vfForeignWindow"
                        
                        // override the default properties to update window's size and position only
                        updatedProperties: WindowProperty.Size | WindowProperty.Position
                                                
                        visible: false
                        // center the window in the parent container.  we will be adjusting the size to maintain aspect
                        // ratio, so let's keep it symmetrical.
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: AbsoluteLayoutProperties {
                            positionX: 0
                            positionY: 427
                        }
                        preferredWidth: 768.0
                        preferredHeight: 426.0
                        focusPolicy: FocusPolicy.None
                    }   
                }
                Button {
                    text: "Start Preview"
                    verticalAlignment: VerticalAlignment.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    onClicked: {
                        startPreview.triggered();
                    }
                    visible: !previewPage.previewOn
                }
            }
            //Preview Actions
            actions: [
                ActionItem {
                    id: startPreview
                    imageSource: "asset:///images/ic_play.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    title: "Start Preview"
                    onTriggered: {
                        vfForeignWindow.visible=true;
                        _EasyGoApp.onCameraSetPreview(true, OrientationSupport.orientation == UIOrientation.Landscape);
                        previewPage.previewOn=true;                     
                    }
                    enabled: !previewPage.previewOn
                },
                ActionItem {
                    id: stopPreview
                    imageSource: "asset:///images/ic_stop.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    title: "Stop Preview"
                    onTriggered: {
                        vfForeignWindow.visible=false;
                        _EasyGoApp.onCameraSetPreview(false, OrientationSupport.orientation == UIOrientation.Landscape);     
                        previewPage.previewOn=false;                
                    }
                    enabled: previewPage.previewOn
                }
            ]
            //Orientation handling
            attachedObjects: [
                OrientationHandler {
                    onRotationCompleted: {
                        if(previewPage.previewOn){
                            _EasyGoApp.onCameraUpdatePreview(orientation == UIOrientation.Landscape);  
                        }                   
                    }
                }
            ]
        }
    }//End of Preview Tab
    /*
    //File Explorer Tab
    Tab {
        id: explorerTab
        title: "File Explorer"
        content: Page {
            
            // Main Container
            Container {
                layout: DockLayout {}
                
                // Application Background image is stretched to fill the display
                ImageView {
                    imageSource: "asset:///images/carbon_720.jpg"
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill    
                }
                
                //UI container
                Container {
                    Label {
                        // Localized text with the dynamic translation and locale updates support
                        text: qsTr("File Explorer") + Retranslate.onLocaleOrLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                    }
                }
            }   
        }
    }//End of File Explorer Tab
    */
    onCreationCompleted: {
        timer.start();
    }
    
}//End of Navigation Pane