/*
    Copyright 2013 Gael Jaffrain

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EasyGo_HPP_
#define EasyGo_HPP_

#include <bb/platform/bbm/Context>
#include <bb/cascades/TabbedPane>
#include <bb/cascades/UIOrientation>
#include <QObject>
#include <QTimer>
#include <QtNetwork>
#include <errno.h>	//Error codes

#include <bps/screen.h>
#include <mm/renderer.h>


namespace bb
{
    namespace cascades
    {
        class Application;
        class LocaleHandler;
        class ForeignWindowControl;
       	class LayoutProperties;
    }
    namespace platform
    {
    	namespace bbm
    	{
    		class Context;
    		class MessageService;
    	}
    }
}

class QTranslator;


/*!
 * @brief Application object
 *
 *
 */

class EasyGo : public QObject
{
Q_OBJECT

// Flag indicating whether the application is successfully registered
// with BBM.
Q_PROPERTY(bool allowed READ isAllowed);

// Other Flags for wifi and gopro connection
Q_PROPERTY(bool wifiConnected READ isWifiConnected NOTIFY wifiConnectedChanged);
Q_PROPERTY(bool goProBusy READ isGoProBusy NOTIFY goProBusyChanged);
Q_PROPERTY(bool goProConnected READ isGoProConnected NOTIFY goProConnectedChanged);
Q_PROPERTY(bool goProPowered READ isGoProPowered NOTIFY goProPoweredChanged);
Q_PROPERTY(int goProSelectedMode READ goProSelectedMode NOTIFY goProSelectedModeChanged);
Q_PROPERTY(bool goProRecording READ isGoProRecording NOTIFY goProRecordingChanged);

Q_PROPERTY(QString goProStatusVideoMode READ goProStatusVideoMode NOTIFY goProStatusVideoModeChanged);
Q_PROPERTY(int goProStatusVideoCount READ goProStatusVideoCount NOTIFY goProStatusVideoCountChanged);
Q_PROPERTY(QString goProStatusVideoRecTime READ goProStatusVideoRecTime NOTIFY goProStatusVideoRecTimeChanged);
Q_PROPERTY(QString goProStatusVideoRemainingRecTime READ goProStatusVideoRemainingRecTime NOTIFY goProStatusVideoRemainingRecTimeChanged);

Q_PROPERTY(int goProStatusPhotoCount READ goProStatusPhotoCount NOTIFY goProStatusPhotoCountChanged);
Q_PROPERTY(int goProStatusPhotoAvailableShots READ goProStatusPhotoAvailableShots NOTIFY goProStatusPhotoAvailableShotsChanged);


public:
    EasyGo(bb::platform::bbm::Context &context, bb::cascades::Application *app);
    virtual ~EasyGo();

    /* Public enums */

    enum GoProMode {
    	VIDEO,
    	STILLS,
    	BURST,
    	TIMELAPSE
    };

    enum GoProPowerOff {
    	GP_NEVER,
    	GP_60SEC,
    	GP_120SEC,
    	GP_T300SEC
    };

    enum GoProLeds {
    	GP_LEDSOFF,
    	GP_2LEDS,
    	GP_4LEDS
    };

    enum GoProOrientation {
    	GP_UP,
    	GP_DOWN
    };

    enum GoProFormat {
    	NTSC,
    	PAL
    };

    enum GoProState {
    	StateDisconnected,
    	StatePoweringOn,
    	StatePowered,
    	StatePoweringOff,
    	StateIdle,
    	StateChangingMode,
    	StateStopRecording
    };

    struct GoProStatus {

    	//Modes
    	GoProMode 			mode;
    	GoProMode			startupMode;

    	//Video
    	bool				recording;
    	unsigned char		videoMode;
    	unsigned char		viewAngle;
    	GoProFormat			videoFormat;
    	unsigned char		recordingMinutes;
    	unsigned char		recordingSeconds;

    	//Photo
    	unsigned char		photoMode;

    	//Storage
    	bool				SDCard;
    	unsigned int		videosCount;
    	unsigned int		videoTimeRemaining;
    	unsigned int		photosCount;
    	unsigned int		photosAvailable;

    	//Various Configuration and Status
    	bool				spotMeter;
    	unsigned char		timeLapseInterval;
    	GoProPowerOff		powerOffTimer;
    	unsigned char		beepVolume;
    	GoProLeds			leds;
    	bool				previewOn;
    	GoProOrientation	orientation;
    	bool				oneButtonMode;
    	bool				OSD;
    	bool				locate;			//Beeping to locate camera
    	unsigned char		batteryPercent;
    };


    /* Invokable functions that we can call from QML*/

    // EasyGo commands
    Q_INVOKABLE void onCameraSetPower(bool power);
    Q_INVOKABLE void onCameraSetMode(int mode);
    Q_INVOKABLE void onCameraSetCapture(bool power);
    Q_INVOKABLE void onCameraGetStatus();
    Q_INVOKABLE void onCheckConnection();
    Q_INVOKABLE void onCameraSetPreview(bool preview, bool isLandscape);
    Q_INVOKABLE void onCameraUpdatePreview(bool isLandscape);

    // This method is invoked to open the invitation dialog
    Q_INVOKABLE void sendInvite();

    /**
     * This Invokable function gets a value from the QSettings,
     * if that value does not exist in the QSettings database, the default value is returned.
     *
     * @param objectName Index path to the item
     * @param defaultValue Used to create the data in the database when adding
     * @return If the objectName exists, the value of the QSettings object is returned.
     *         If the objectName doesn't exist, the default value is returned.
     */
    Q_INVOKABLE
    QString getValueFor(const QString &objectName, const QString &defaultValue);

    /**
     * This function sets a value in the QSettings database. This function should to be called
     * when a data value has been updated from QML
     *
     * @param objectName Index path to the item
     * @param inputValue new value to the QSettings database
     */
    Q_INVOKABLE
    void saveValueFor(const QString &objectName, const QString &inputValue);

    Q_INVOKABLE
    void updateGoProSettings();

public slots:
	// This methods creates the main UI and initializes the message service
	void show();

signals:
	void goProBusyChanged(bool);
	void goProConnectedChanged(bool);
	void goProPoweredChanged(bool);
	void goProSelectedModeChanged(int);
	void goProRecordingChanged(bool);
	void goProPreviewChanged(bool);
	void wifiConnectedChanged(bool);

	void goProStatusVideoModeChanged(QString);
	void goProStatusVideoCountChanged(int);
	void goProStatusVideoRecTimeChanged(QString);
	void goProStatusVideoRemainingRecTimeChanged(QString);
	void goProStatusPhotoCountChanged(int);
	void goProStatusPhotoAvailableShotsChanged(int);

private slots:
    void onSystemLanguageChanged();
    void onRequestFinished ( QNetworkReply* reply );
    void bootTimerUpdate();
    void changeTimerUpdate();
    void stopTimerUpdate();

private:
    bb::cascades::TabbedPane *mRoot;

    //Localization
    QTranslator* m_pTranslator;
    bb::cascades::LocaleHandler* m_pLocaleHandler;

    // BBM  - Return true if registration has completed successfully.
    bool isAllowed() const
    { return m_context->isAccessAllowed(); }
    // BBM - The service object to send BBM messages
    bb::platform::bbm::MessageService* m_messageService;
    bb::platform::bbm::Context* m_context;

    // Network Access
    QString mGoProIP;
	QString mGoProPassword;
	QString mGoProWifiName;
	QNetworkAccessManager *mpNetworkAccessManager;
	bool replyPending;
	int statusPending;
    void initNetworkAccessManager();
    QUrl buildUrl(QString command, QString value);
    QUrl buildUrl(QString command);
    bool isWifiConnected();

    //State machine management
    int exitState();
    int enterState(GoProState newState, GoProState &nextState);
    int runStateMachine(GoProState newState);
    const char* stateName(GoProState state);
    void setPowerCommand(bool power);

    bool isGoProBusy();
    bool isGoProConnected();
    bool isGoProPowered();
    int goProSelectedMode();
    bool isGoProRecording();
    QString goProStatusVideoMode();
    int goProStatusVideoCount();
    QString goProStatusVideoRecTime();
    QString goProStatusVideoRemainingRecTime();

    int goProStatusPhotoCount();
    int goProStatusPhotoAvailableShots();


    bool mGoProBusy;			//When camera is not ready to get a new command
    bool mGoProConnected;		//Camera in Idle or Powered On.
    bool mGoProPowered;
    GoProMode mNextMode;

    GoProStatus mGoProStatus;	//Latest Status report from GoPro
    GoProState mGoProState;		//State managed by a state machine
    QTimer *bootTimer;
    QTimer *changeTimer;
    QTimer *stopTimer;

    // Declare mm-renderer variables
    int mmrenderer_init();
    int mmrenderer_stop();
	// Screen variables
	screen_context_t screen_context;
	screen_window_t screen_window;
	int screen_size[2];
	// Renderer variables
	mmr_connection_t* mmr_connection;
	mmr_context_t* mmr_context;
	strm_dict_t* dict;
	// I/O variables
	int video_device_output_id;
	int audio_device_output_id;

    bb::cascades::ForeignWindowControl* mFwc;
    strm_dict_t* calculate_rect(bool isLandscape);

};

#endif /* EasyGo_HPP_ */
