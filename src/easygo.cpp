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

#include "easygo.hpp"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/TabbedPane>
#include <bb/cascades/LocaleHandler>
#include <bb/cascades/Window>
#include <bb/cascades/ForeignWindowControl>
#include <bb/cascades/AbsoluteLayout>
#include <bb/cascades/AbsoluteLayoutProperties>
#include <bb/platform/bbm/MessageService>
#include <bb/cascades/OrientationSupport>
#include <bps/soundplayer.h>
#include <bb/system/SystemToast>



using namespace bb::cascades;
using namespace bb::system;


#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <bps/bps.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <mm/renderer.h>
#include <wifi/wifi_service.h>


EasyGo::EasyGo(bb::platform::bbm::Context &context, bb::cascades::Application *app) :
        QObject(app)
    	, m_messageService(0)
    	, m_context(&context)
{
    qmlRegisterType<QTimer>("my.timer", 1, 0, "QTimer");
	// Prepare the localization
    m_pTranslator = new QTranslator(this);
    m_pLocaleHandler = new LocaleHandler(this);
    if(!QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()), this, SLOT(onSystemLanguageChanged()))) {
        // This is an abnormal situation! Something went wrong!
        // Add own code to recover here
        qWarning() << "Recovering from a failed connect()";
    }
    // Initial load
    onSystemLanguageChanged();

	// Set the application organization and name, which is used by QSettings
	// when saving values to the persistent store.
	QCoreApplication::setOrganizationName("Gael Jaffrain");
	QCoreApplication::setApplicationName("EasyGo");

}

EasyGo::~EasyGo(){

	//Stutting down and cleaning mm-renderer
	mmrenderer_stop();
}

void EasyGo::show()
{
    // Initialize some GoPro variables
    mGoProIP = "10.5.5.9";
    updateGoProSettings(); 			//Read settings from QSettings
    initNetworkAccessManager(); 	//Initialize Network Access Manager

    // Initialize the Boot up timer
    bootTimer = new QTimer(this);
    connect(bootTimer, SIGNAL(timeout()), this, SLOT(bootTimerUpdate()));
    bootTimer->setSingleShot(true);
    bootTimer->setInterval(2500);

    // Initialize the Change timer
    changeTimer = new QTimer(this);
	connect(changeTimer, SIGNAL(timeout()), this, SLOT(changeTimerUpdate()));
	changeTimer->setSingleShot(true);
	changeTimer->setInterval(1500);

    // Initialize the Stop timer
    stopTimer = new QTimer(this);
	connect(stopTimer, SIGNAL(timeout()), this, SLOT(stopTimerUpdate()));
	stopTimer->setSingleShot(true);
	stopTimer->setInterval(1500);

    // Initialize State Machine
    mGoProState = StateDisconnected;
    mGoProBusy = false;
    mGoProConnected = false;
    mGoProPowered = false;
    mGoProStatus.recording = false;
    statusPending = 0;
    replyPending = false;

    runStateMachine(StatePoweringOn);

    // Now Let's Load and Show main QML

    // Create scene document from main.qml asset, the parent is set
    // to ensure the document gets destroyed properly at shut down.
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);

    // Set Context to access C++ methods from QML
    qml->setContextProperty("_EasyGoApp", this);

    if (!qml->hasErrors())
    {
    	// The application NavigationPane is created from QML.
		mRoot = qml->createRootObject<TabbedPane>();

		if (mRoot)
		{
			qml->setContextProperty("_tabPane", mRoot);

			// Set the main scene for the application to the NavigationPane.
			Application::instance()->setScene(mRoot);
		}
    }

    // Initialize mm-renderer variables
    mmrenderer_init();
}

void EasyGo::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(m_pTranslator);
    // Initiate, load and install the application translation files.
    QString locale_string = QLocale().name();
    QString file_name = QString("EasyGo_%1").arg(locale_string);
    if (m_pTranslator->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(m_pTranslator);
    }
}

void EasyGo::onCameraSetPower(bool power)
{
	qDebug() << "onCameraSetPower" << power;
	GoProState state;

	if (power) {
		state = StatePoweringOn;
	} else {
		state = StatePoweringOff;
	}
	int err = runStateMachine(state);
	if (err) {
			qDebug() << "Error Running State Machine to enter state" << stateName(state);
	}
}

void EasyGo::setPowerCommand(bool power)
{
	qDebug() << "setPowerCommand" << power;

	// Creates the network request and sets the destination URL.
	QNetworkRequest request = QNetworkRequest();
	QUrl requestUrl;
	if (power) {
		requestUrl = buildUrl("bacpac/PW", "01");
	} else {
		requestUrl = buildUrl("bacpac/PW", "00");
	}

	// Send the request
	replyPending = true;
	request.setUrl(requestUrl);
	mpNetworkAccessManager->get(request);
}

void EasyGo::onCameraSetCapture(bool start)
{
	qDebug() << "onCameraSetCapture" << start;

	// Creates the network request and sets the destination URL.
	QNetworkRequest request = QNetworkRequest();
	QUrl requestUrl;
	int err;

	if (start) {
		requestUrl = buildUrl("bacpac/SH", "01");
		if (mGoProStatus.mode == VIDEO || mGoProStatus.mode == TIMELAPSE) {
			emit goProRecordingChanged(mGoProStatus.recording = true);
		    err = soundplayer_play_sound("event_recording_start");
		    if (err) {qDebug() << "Error Playing Sound" << err;}
		} else {
		    int ii_max=1;
		    if (mGoProStatus.mode == BURST) {ii_max = 5;}
			for(int ii=0 ; ii < ii_max ; ii++){
				err = soundplayer_play_sound("event_camera_shutter");
			    if (err) {qDebug() << "Error Playing Sound" << err;}
			}
		}
	} else {
		requestUrl = buildUrl("bacpac/SH", "00");
		if (mGoProStatus.mode == VIDEO || mGoProStatus.mode == TIMELAPSE) {
			emit goProRecordingChanged(mGoProStatus.recording = false);
		    err = soundplayer_play_sound("event_recording_stop");
		    if (err) {qDebug() << "Error Playing Sound" << err;}
		}
	}

	// Send the request
	request.setUrl(requestUrl);
	mpNetworkAccessManager->get(request);
}

void EasyGo::onCameraUpdatePreview(bool isLandscape)
{
	qDebug() << "onCameraUpdatePreview" << isLandscape;

	/* Do some work to make the aspect ratio correct.
	 */
	qDebug() << "Screen_size[0]=" << screen_size[0] << " - Screen_size[1]=" << screen_size[1];
	dict = calculate_rect(isLandscape);
	if (NULL == dict) {
		qDebug() << "Error getting aspect ratio dictionary";
	}

	if (mmr_output_parameters(mmr_context, video_device_output_id, dict) != 0) {
		qDebug() << "Error setting mmr output parameters";
	}

	 /* Note that we allocated memory for the dictionary, but the call to
	  * mmr_output_parameters() deallocates that memory even on failure.
	  */
	dict = NULL;

}

void EasyGo::onCameraSetPreview(bool preview, bool isLandscape)
{
	qDebug() << "onCameraSetPreview" << preview;

	if (preview) {
		emit goProPreviewChanged(true);

		//===================
		//Start mm-renderer
		//===================
		int rc;
		// Build up the path where our bundled resource is.
		char cwd[PATH_MAX];
		char media_file[PATH_MAX];
		getcwd(cwd,PATH_MAX);

		rc = snprintf(media_file, PATH_MAX, "http://10.5.5.9:8080/live/amba.m3u8", cwd);
		if ((rc == -1) || (rc >= PATH_MAX)) {
			qDebug() << "Error reading stream name";
		}

		/*
		 * Start the playback.
		 */
		if (mmr_input_attach(mmr_context, media_file, "track") != 0) {
			qDebug() << "Error attaching input";
		}

		if (mmr_play(mmr_context) != 0) {
			qDebug() << "Error starting playback";
		}

		/* Do some work to make the aspect ratio correct.
		 */
		qDebug() << "Screen_size[0]=" << screen_size[0] << " - Screen_size[1]=" << screen_size[1];
		dict = calculate_rect(isLandscape);
		if (NULL == dict) {
			qDebug() << "Error getting aspect ratio dictionary";
		}

		if (mmr_output_parameters(mmr_context, video_device_output_id, dict) != 0) {
			qDebug() << "Error setting mmr output parameters";
		}

		 /* Note that we allocated memory for the dictionary, but the call to
		  * mmr_output_parameters() deallocates that memory even on failure.
		  */
		dict = NULL;

		//===== mm-renderer ======

	} else {
		emit goProPreviewChanged(false);
		int rc;

		//Stop mm-renderer
		if ((rc = mmr_stop(mmr_context))) {
			qDebug() << "mmr - Error stopping" << rc;
		}
	}
}

void EasyGo::onCameraSetMode(int mode)
{
	qDebug() << "onCameraSetMode" << mode;

	if(mGoProStatus.recording) {
		qDebug() << "Stop Recording";
		runStateMachine(StateStopRecording);

		mNextMode = GoProMode(mode);
		onCameraSetCapture(false);
		stopTimer->start();

	} else {
		runStateMachine(StateChangingMode);

		// Creates the network request and sets the destination URL.
		QNetworkRequest request = QNetworkRequest();
		QUrl requestUrl;

		switch (mode) {
			case VIDEO:
				requestUrl = buildUrl("camera/CM", "00");
				break;
			case STILLS:
				requestUrl = buildUrl("camera/CM", "01");
				break;
			case BURST:
				requestUrl = buildUrl("camera/CM", "02");
				break;
			case TIMELAPSE:
				requestUrl = buildUrl("camera/CM", "03");
				break;
		}

		// Send the request
		replyPending = true;
		request.setUrl(requestUrl);
		mpNetworkAccessManager->get(request);
		changeTimer->start();
	}
}

void EasyGo::onCameraGetStatus()
{
	qDebug() << "onCameraGetStatus";

	if (statusPending < 3) {
		// Creates the network request and sets the destination URL.
		QNetworkRequest request = QNetworkRequest();
		QUrl requestUrl = buildUrl("camera/se");

		// Send the request
		statusPending++;
		request.setUrl(requestUrl);
		mpNetworkAccessManager->get(request);
	} else {
		int err = runStateMachine(StateDisconnected);
		if (err) {
				qDebug() << "Error Running State Machine to enter state" << stateName(StateDisconnected);
		}
	}
}

void EasyGo::onCheckConnection()
{
	qDebug() << "onCheckConnection";

	if (!mGoProBusy) {
		int err = runStateMachine(StatePoweringOn);
		if (err) {
			qDebug() << "Error Running State Machine to enter state" << stateName(StatePoweringOn);
		}
	}
}

bool EasyGo::isGoProBusy()
{
	return mGoProBusy;
}

bool EasyGo::isGoProConnected()
{
	return mGoProConnected;
}

bool EasyGo::isGoProPowered()
{
	return mGoProPowered;
}

bool EasyGo::isWifiConnected()
{

	SystemToast *toast = new SystemToast(this);
	toast->setPosition(SystemUiPosition::MiddleCenter);

	//NEW WIFI API
	wifi_result_t wifiErr;
	char* mySSID = new char[WIFI_MAX_SSID_BUFFER_LEN-1];
	if ((wifiErr = wifi_station_connection_get_ssid(mySSID))){
		qDebug() << "Error getting current connected SSID - code:" << wifiErr;
		toast->setBody("Cannot read SSID ! Wifi disabled ?");
		toast->show();
	} else {
		qDebug() << "Connected SSID:" << mySSID;
	}

	if (mySSID == mGoProWifiName){
		emit wifiConnectedChanged(true);
		return 1;
	} else {
		emit wifiConnectedChanged(false);
		toast->setBody("Not Connected ! Please check GoPro name and connection !");
		toast->show();
		return 0;
	}
}

int EasyGo::goProSelectedMode()
{
	int returnMode;

	switch(mGoProStatus.mode){
	case VIDEO:
		returnMode = 0;
		break;
	case STILLS:
		returnMode = 1;
		break;
	case BURST:
		returnMode = 2;
		break;
	case TIMELAPSE:
		returnMode = 3;
		break;
	}

	return returnMode;
}

bool EasyGo::isGoProRecording(){
	return mGoProStatus.recording;
}

void EasyGo::initNetworkAccessManager()
{

	// Creates the network access manager and connects a custom slot to its
	// finished signal. Checks the return value for errors.
	mpNetworkAccessManager = new QNetworkAccessManager(this);

	// If any Q_ASSERT statement(s) indicate that the slot failed to connect to
	// the signal, make sure you know exactly why this has happened. This is not
	// normal, and will cause your app to stop working!!
	bool result;

	// Since the variable is not used in the app, this is added to avoid a
	// compiler warning.
	Q_UNUSED(result);

	result = connect(mpNetworkAccessManager, SIGNAL(finished(QNetworkReply*)),
	        this, SLOT(onRequestFinished(QNetworkReply*)));

	// This is only available in Debug builds.
	Q_ASSERT(result);

	// Check if Wifi is connected
	if (!isWifiConnected())
	{
		qDebug() << "Wifi is NOT connected, we need to handle this ...";
	}



}

void EasyGo::onRequestFinished ( QNetworkReply* reply )
{
    QNetworkReply::NetworkError netError = reply->error();

    if(netError == QNetworkReply::NoError)
    {
    	qDebug() << "Network message sent without error !";

    	unsigned char buf[128];
		int reply_size = reply->read((char*)buf, sizeof(buf));
		qDebug() << "GoPro Reply length:" << reply_size << "bytes";
		qDebug() << "Mode:" << (int)buf[1];

		// If reply is 31 bytes, it means the query was the status request
		if (reply_size == 31) {
			statusPending--;

			emit goProSelectedModeChanged(mGoProStatus.mode = (GoProMode)buf[1]);
			mGoProStatus.startupMode = (GoProMode)buf[3];
			mGoProStatus.spotMeter = (bool)buf[4];
			mGoProStatus.timeLapseInterval = buf[5];
			mGoProStatus.powerOffTimer = (GoProPowerOff)buf[6];
			mGoProStatus.viewAngle = buf[7];
			mGoProStatus.photoMode = buf[8];
			mGoProStatus.videoMode = buf[9];
			emit goProStatusVideoModeChanged(goProStatusVideoMode());

			emit goProRecordingChanged(mGoProStatus.recording = (bool)buf[29]);
			mGoProStatus.recordingMinutes = buf[13];
			mGoProStatus.recordingSeconds = buf[14];
			emit goProStatusVideoRecTimeChanged(goProStatusVideoRecTime());


			mGoProStatus.beepVolume = buf[16];
			mGoProStatus.leds = (GoProLeds)buf[17];
			mGoProStatus.previewOn = (bool)(buf[18] & 0x01);
			mGoProStatus.orientation = (GoProOrientation)((buf[18] & 0x04) >> 2);
			mGoProStatus.oneButtonMode = (bool)((buf[18] & 0x08) >> 3);
			mGoProStatus.OSD = (bool)((buf[18] & 0x10) >> 4);
			mGoProStatus.videoFormat = (GoProFormat)((buf[18] & 0x20) >> 5);
			mGoProStatus.locate = (bool)((buf[18] & 0x40) >> 6);

			mGoProStatus.batteryPercent = buf[19];

			emit goProStatusPhotoAvailableShotsChanged(mGoProStatus.photosAvailable = ((unsigned int)buf[21]<< 8) + buf[22]);
			emit goProStatusPhotoCountChanged(mGoProStatus.photosCount = ((unsigned int)buf[23]<<8) + buf[24]);
			mGoProStatus.videoTimeRemaining = ((unsigned int)buf[25]<<8) + buf[26];
			emit goProStatusVideoRemainingRecTimeChanged(goProStatusVideoRemainingRecTime());
			mGoProStatus.videosCount = ((unsigned int)buf[27]<< 8) + buf[28];
			emit goProStatusVideoCountChanged(mGoProStatus.videosCount);

			if ((mGoProStatus.photosAvailable == 65535)&&
				(mGoProStatus.photosCount == 65535)&&
				(mGoProStatus.videoTimeRemaining == 65535)&&
				(mGoProStatus.videosCount == 65535)) {

				// SD card is not detected
				mGoProStatus.SDCard = false;

			} else {

				// SD card is detected
				mGoProStatus.SDCard = true;

			}

			//qDebug() << "Mode:" << mGoProStatus.mode;
			/*
			qDebug() << "Startup Mode:" << mGoProStatus.startupMode;
			qDebug() << "Spot Meter:" << mGoProStatus.spotMeter;
			qDebug() << "Time Lapse Interval:" << mGoProStatus.timeLapseInterval;
			qDebug() << "Power Off Timer:" << mGoProStatus.powerOffTimer;
			qDebug() << "Video View Angle:" << mGoProStatus.viewAngle;
			qDebug() << "Photo Resolution & FOV:" << mGoProStatus.photoMode;
			qDebug() << "Video Mode:" << mGoProStatus.videoMode;
			qDebug() << "Recording Minutes:" << mGoProStatus.recordingMinutes;
			qDebug() << "Recording Seconds:" << mGoProStatus.recordingSeconds;
			qDebug() << "Beep Volume:" << mGoProStatus.beepVolume;
			qDebug() << "LEDs:" << mGoProStatus.leds;
			qDebug() << "Preview On:" << mGoProStatus.previewOn;
			qDebug() << "Orientation:" << mGoProStatus.orientation;
			qDebug() << "One Button Mode:" << mGoProStatus.oneButtonMode;
			qDebug() << "OSD:" << mGoProStatus.OSD;
			qDebug() << "Video Format:" << mGoProStatus.videoFormat;
			qDebug() << "Locate (beeping):" << mGoProStatus.locate;
			qDebug() << "Battery Percent:" << mGoProStatus.batteryPercent;
			qDebug() << "Photos Available:" << mGoProStatus.photosAvailable;
			qDebug() << "Photo Count:" << mGoProStatus.photosCount;
			qDebug() << "Video Time Remaining:" << mGoProStatus.videoTimeRemaining;
			qDebug() << "Video Count:" << mGoProStatus.videosCount;
			qDebug() << "Recording:" << mGoProStatus.recording;
			qDebug() << "SD Card Detected:" << mGoProStatus.SDCard;
			*/
		}
		else
		{
		    replyPending = false;
		}
    }
    else
    {
    	switch(netError)
        {
            case QNetworkReply::ContentNotFoundError:
                qDebug() << "The content was not found on the server";
                break;

            case QNetworkReply::HostNotFoundError:
                qDebug() << "The server was not found";
                break;

            default:
                qDebug() << "Error:" << reply->errorString();
                break;
        }
    	int err = runStateMachine(StateDisconnected);
    	if (err) {
			qDebug() << "Error Running State Machine to enter state" << stateName(StateDisconnected);
		}

    }
}

QUrl EasyGo::buildUrl(QString command, QString value)
{
	// Build the request string URL
	QString requestString = "http://";
	requestString.append(mGoProIP);
	requestString.append("/");
	requestString.append(command);
	requestString.append("?t=");
	requestString.append(mGoProPassword);
	requestString.append("&p=%");
	requestString.append(value);

	qDebug() << requestString;

	// the "fromEncoded() is used to avoid "%" to be re-encoded as "%25"
	QUrl url = QUrl::fromEncoded(requestString.toAscii());
	return url;
}

QUrl EasyGo::buildUrl(QString command)
{
	// Build the request string URL
	QString requestString = "http://";
	requestString.append(mGoProIP);
	requestString.append("/");
	requestString.append(command);
	requestString.append("?t=");
	requestString.append(mGoProPassword);

	qDebug() << requestString;

	// the "fromEncoded() is used to avoid "%" to be re-encoded as "%25"
	QUrl url = QUrl::fromEncoded(requestString.toAscii());
	return url;
}

void EasyGo::sendInvite()
{
    if (!m_messageService) {
        // Instantiate the MessageService.
        m_messageService = new bb::platform::bbm::MessageService(m_context, this);
    }

    // Trigger the invite to download process.
    m_messageService->sendDownloadInvitation();
}

QString EasyGo::getValueFor(const QString &objectName, const QString &defaultValue)
{
    QSettings settings;

    // If no value has been saved, return the default value.
    if (settings.value(objectName).isNull()) {
        return defaultValue;
    }

    // Otherwise, return the value stored in the settings object.
    return settings.value(objectName).toString();
}

void EasyGo::saveValueFor(const QString &objectName, const QString &inputValue)
{
    // A new value is saved to the application settings object.
    QSettings settings;
    settings.setValue(objectName, QVariant(inputValue));
}

void EasyGo::updateGoProSettings()
{
	mGoProPassword = getValueFor("password", "");
    mGoProWifiName = getValueFor("wifiName", "");
    qDebug() << "Password extracted from settings" << mGoProPassword;
    qDebug() << "Wifi Name extracted from settings" << mGoProWifiName;
}

int EasyGo::enterState(GoProState state, GoProState &nextState)
{
	 int err = EOK;

	//Normal case is that we enter the requested state and keep it
	mGoProState = state;
	nextState = state;
	qDebug() << "Entering State" << stateName(state);

	switch (state) {
	case StateDisconnected:
		emit goProConnectedChanged(mGoProConnected = false);
		emit goProPoweredChanged(mGoProPowered = false);
		emit goProBusyChanged(mGoProBusy = false);
		break;

	case StatePoweringOn:
		if (isWifiConnected()) {
		    emit goProPoweredChanged(mGoProPowered = false);
		    emit goProBusyChanged(mGoProBusy = true);	//Signal that camera will be busy powering ON
			setPowerCommand(true);						//Send Power ON command. Reply will be asynchronous
			bootTimer->start(); 						//Wait for Camera to boot up
		} else {
			qDebug() << "Wifi is not connected";
			nextState = StateDisconnected;
		}
		break;

	case StatePoweringOff:
		if (isWifiConnected()) {
			emit goProPoweredChanged(mGoProPowered = false);
			emit goProBusyChanged(mGoProBusy = true);	//Signal that camera will be busy powering ON
			setPowerCommand(false);					//Send Power OFF command. Reply will be asynchronous
			bootTimer->start(); 						//Wait for Camera to shutdown
		} else {
			qDebug() << "Wifi is not connected";
			nextState = StateDisconnected;
		}
		break;

	case StateIdle:
		break;

	case StatePowered:
		emit goProConnectedChanged(mGoProConnected = true);
	    emit goProPoweredChanged(mGoProPowered = true);
	    onCameraGetStatus();
		break;

	case StateStopRecording:
		emit goProRecordingChanged(mGoProStatus.recording = false);
		emit goProBusyChanged(mGoProBusy = true);
		break;
	case StateChangingMode:
		emit goProBusyChanged(mGoProBusy = true);
		break;

	default:
		break;
	}

	return err;
}

int EasyGo::exitState()
{
	int err = EOK;
	qDebug() << "Exiting State" << stateName(mGoProState);

	switch (mGoProState) {
	case StateDisconnected:
		break;

	case StateStopRecording:
	case StatePoweringOn:
	case StatePoweringOff:
	case StateChangingMode:
		emit goProBusyChanged(mGoProBusy = false);
		break;

	case StateIdle:
		break;
	case StatePowered:
		break;
	default:
		break;
	}

	return err;
}

int EasyGo::runStateMachine(GoProState state)
{
	int err = EOK;
	GoProState nextState;

	while(mGoProState != state) {

		//Perform actions needed to exit current state
		err = exitState();
		if (err) {
			return err;
		}

		//Enter new state requested by the user or the previous iterations of the state machine
		err = enterState(state, nextState);
		if (err) {
			return err;
		}

		//Check if a new transition was needed after the previous enterState()
		if (nextState != state) {
			state = nextState;
		}
	}

	return err;
}

const char* EasyGo::stateName(GoProState state)
{
    switch(state) {
    case StateDisconnected:
    	return "Disconnected";
    case StatePoweringOn:
    	return "Powering On";
    case StatePowered:
    	return "Powered";
    case StatePoweringOff:
    	return "Powering Off";
    case StateIdle:
        return "Idle";
    case StateChangingMode:
    	return "Changing Mode";
    case StateStopRecording:
    	return "Stop Recording";
    default:
        return "UNKNOWN";
    }
}

void EasyGo::bootTimerUpdate() {
	qDebug() << "Boot timeout. Reply still pending ?" << replyPending;
	GoProState nextState;

	if (replyPending) {
		nextState = StateDisconnected;
	} else {
		switch (mGoProState) {
		case StatePoweringOn:
			nextState = StatePowered;
			break;
		case StatePoweringOff:
			nextState = StateIdle;
			break;
		default:
			nextState = StateDisconnected;
			break;
		}
	}
	int err = runStateMachine(nextState);
	if (err) {
		qDebug() << "Error Running State Machine to enter state" << stateName(nextState);
	}
}

void EasyGo::changeTimerUpdate() {
	qDebug() << "Mode Change timeout. Reply still pending ?" << replyPending;
	GoProState nextState;

	if (replyPending) {
		nextState = StateDisconnected;
	} else {
		nextState = StatePowered;
	}
	int err = runStateMachine(nextState);
	if (err) {
		qDebug() << "Error Running State Machine to enter state" << stateName(nextState);
	}
}

void EasyGo::stopTimerUpdate() {
	qDebug() << "Stop timeout. Reply still pending ?" << replyPending;
	GoProState nextState;

	if (replyPending) {
		nextState = StateDisconnected;
	} else {
		if (mNextMode == mGoProStatus.mode) {
			nextState = StatePowered;
		} else {
			nextState = StateChangingMode;
			onCameraSetMode(mNextMode);
		}
	}
	int err = runStateMachine(nextState);
	if (err) {
		qDebug() << "Error Running State Machine to enter state" << stateName(nextState);
	}
}

QString EasyGo::goProStatusVideoMode() {
	QString mode;

	switch (mGoProStatus.videoMode)
	{
	case 1:
		mode = "WVGA";
		break;
	case 2:
		mode = "720p";
		break;
	case 3:
		mode = "720p";
		break;
	case 4:
		mode = "960p";
		break;
	case 5:
		mode = "960p";
		break;
	case 6:
		mode = "1080p";
		break;
	case 0:
	default:
		mode = "";
		break;
	}
	return mode;
}

int EasyGo::goProStatusVideoCount(){
	return mGoProStatus.videosCount;
}

int EasyGo::goProStatusPhotoCount(){
	return mGoProStatus.photosCount;
}

int EasyGo::goProStatusPhotoAvailableShots(){
	return mGoProStatus.photosAvailable;
}

QString EasyGo::goProStatusVideoRecTime(){

	return (QString::number(mGoProStatus.recordingMinutes) + QString("m:") + QString::number(mGoProStatus.recordingSeconds) + QString("s"));
}
QString EasyGo::goProStatusVideoRemainingRecTime(){
	int hh = 0;
	int mm = 0;
	int ss = 0;

	int tt=mGoProStatus.videoTimeRemaining;

	hh = tt/3600;
	mm = (tt%3600)/60;
	ss = tt-3600*hh-60*mm;

	return (QString::number(hh) + QString("h:") + QString::number(mm) + QString("m:") + QString::number(ss) + QString("s"));
}

int EasyGo::mmrenderer_init()
{
	int err = EOK;

	// find the ForeignWindowControl and cache it for later
	mFwc = mRoot->findChild<ForeignWindowControl*>("vfForeignWindow");
	if (!mFwc) {
		qDebug() << "Foreign Window not found !";
	}

	// find the window group & window id required by the ForeignWindowControl
	QByteArray groupBA = mFwc->windowGroup().toLocal8Bit();
	QByteArray winBA = mFwc->windowId().toLocal8Bit();

	qDebug() << "mFwc group:" << groupBA;
	qDebug() << "mFwc win:" << winBA;

	// Screen variables
	screen_context = 0;
	screen_window = 0;
	screen_size[0]=0;
	screen_size[1]=0;

	// Renderer variables
	mmr_connection = 0;
	mmr_context = 0;
	dict = NULL;

	// I/O variables
	video_device_output_id = -1;
	audio_device_output_id = -1;

	/*
	 * Create the window used for video output.
	 */
	if (screen_create_context(&screen_context, SCREEN_APPLICATION_CONTEXT) != 0) {
		return EXIT_FAILURE;
	}

	if (screen_create_window_type(&screen_window, screen_context, SCREEN_CHILD_WINDOW) != 0) {
		screen_destroy_context(screen_context);
		return EXIT_FAILURE;
	}

	screen_set_window_property_cv(screen_window,SCREEN_PROPERTY_ID_STRING,256,winBA);
	screen_join_window_group(screen_window, groupBA);

	int format = SCREEN_FORMAT_RGBA8888;
	if (screen_set_window_property_iv(screen_window, SCREEN_PROPERTY_FORMAT, &format) != 0) {
		return EXIT_FAILURE;
	}

	int usage = SCREEN_USAGE_NATIVE;
	if (screen_set_window_property_iv(screen_window, SCREEN_PROPERTY_USAGE, &usage) != 0) {
		return EXIT_FAILURE;
	}

	if (screen_create_window_buffers(screen_window, 1) != 0) {
		return EXIT_FAILURE;
	}

	/*
	 * Configure mm-renderer.
	 */
	mmr_connection = mmr_connect(NULL);
	if (mmr_connection == NULL) {
		return EXIT_FAILURE;
	}

	static const char *video_context_name = "videocontextname";
	mmr_context = mmr_context_create(mmr_connection, video_context_name, 0, S_IRWXU|S_IRWXG|S_IRWXO);
	if (mmr_context == NULL) {
		return EXIT_FAILURE;
	}

	/*
	 * Configure video and audio output.
	 */
	QString video_device_url ="screen:?winid=" + winBA + "&wingrp=" + groupBA;
	video_device_output_id = mmr_output_attach(mmr_context, video_device_url.toLocal8Bit(), "video");
	if (video_device_output_id == -1) {
		return EXIT_FAILURE;
	}

	const char *audio_device_url = "audio:default";
	audio_device_output_id = mmr_output_attach(mmr_context, audio_device_url, "audio");
	if (audio_device_output_id == -1) {
		return EXIT_FAILURE;
	}

	// Get the render buffer
	screen_buffer_t temp_buffer[1];
	if (screen_get_window_property_pv( screen_window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)temp_buffer) != 0) {
		return EXIT_FAILURE;
	}

	// Fill the buffer with a solid color (blue)
	int fill_attributes[3] = {SCREEN_BLIT_COLOR, 0xff, SCREEN_BLIT_END};
	if (screen_fill(screen_context, temp_buffer[0], fill_attributes) != 0) {
		return EXIT_FAILURE;
	}

	int z = -5;
	screen_set_window_property_iv(screen_window, SCREEN_PROPERTY_ZORDER, &z);

	// Get window size
	if (screen_get_window_property_iv(screen_window, SCREEN_PROPERTY_SIZE, screen_size) != 0) {
		return EXIT_FAILURE;
	}
	qDebug() << "Screen size:" << screen_size[0] << "x" << screen_size[1];

	return err;
}

/*
 * we know the GoPro video has a resolution of 432x240 so it has an aspect ratio of
 * of 1.8.  Based on these facts, we can show the video as large
 * as possible without changing the aspect ratio.
 */
strm_dict_t* EasyGo::calculate_rect(bool isLandscape) {
    const int image_width = 432;
    const int image_height = 240;
    const float image_aspect = (float)image_width / (float)image_height;
    const float aspect_tolerance = 0.1;

    int width;
    int height;

    if (isLandscape){
    	//Width has to be the bigger dimension
    	if (screen_size[0] > screen_size[1]){
    		width = screen_size[0];
    		height = screen_size[1];
    	} else {
    		width = screen_size[1];
    		height = screen_size[0];
    	}
    } else {
    	//Width has to be the smaller dimension
		if (screen_size[0] < screen_size[1]){
			width = screen_size[0];
			height = screen_size[1];
		} else {
			width = screen_size[1];
			height = screen_size[0];
		}
    }

    int new_width = width;
    int new_height = height;
    int x_offset = 0;
    int y_offset = 0;

    char buffer[16];
    strm_dict_t *dict = strm_dict_new();

    if (NULL == dict) {
        return NULL;
    }

    //fullscreen is the default.
    dict = strm_dict_set(dict, "video_dest_x", "0");
    if (NULL != dict) { dict = strm_dict_set(dict, "video_dest_y", "0"); }
    if (NULL != dict) { dict = strm_dict_set(dict, "video_dest_w", itoa(width, buffer, 10)); }
    if (NULL != dict) { dict = strm_dict_set(dict, "video_dest_h", itoa(height, buffer, 10)); }
    if (NULL != dict)
    {
		float screen_aspect;
		screen_aspect = (float)width/(float)height;
		if (fabs(screen_aspect - image_aspect) < aspect_tolerance) {
			//if the screen is at almost the same aspect as the video, just
			//do full screen.  Nothing to do here.  Fall through and return
			//full screen.
		} else if (screen_aspect < image_aspect) {
			/* The screen is too tall.  We need to set the
			 * width the same as the screen's while maintaining the same aspect
			 * ratio.
			 */
			new_height = width / image_aspect;
			y_offset = (height - new_height) / 2;
			qDebug() << "Yoffset=" << y_offset;

			dict = strm_dict_set(dict, "video_dest_y", itoa(y_offset, buffer, 10));
			if (NULL == dict)
			{
				strm_dict_destroy(dict);
				return NULL;
			}

			dict = strm_dict_set(dict, "video_dest_h", itoa(new_height, buffer, 10));
			if (NULL == dict)
			{
				strm_dict_destroy(dict);
				return NULL;
			}
		} else {
			/* The screen is too wide.  We need to set the
			 * height the same as the screen's while maintaining the same aspect
			 * ratio.
			 */
			new_width = height * image_aspect;
			x_offset = (width - new_width) / 2;
			qDebug() << "Xoffset=" << x_offset;

			dict = strm_dict_set(dict, "video_dest_x", itoa(x_offset, buffer, 10));
			if (NULL == dict)
			{
				strm_dict_destroy(dict);
				return NULL;
			}

			dict = strm_dict_set(dict, "video_dest_w", itoa(new_width, buffer, 10));
			if (NULL == dict)
			{
				strm_dict_destroy(dict);
				return NULL;
			}
		}

		//Resize Foreign Window
		mFwc->setPreferredSize(new_width, new_height);

		AbsoluteLayoutProperties* pProperties = AbsoluteLayoutProperties::create();
		pProperties->setPositionX(x_offset);
		pProperties->setPositionY(y_offset);
		mFwc->setLayoutProperties(pProperties);

		qDebug() << "resized viewfinder win to" << new_width << "x" << new_height;
		qDebug() << "moved viewfinder win to" << x_offset << "x" << y_offset;

		return dict;
    }
    strm_dict_destroy(dict);
    return NULL;
}

int EasyGo::mmrenderer_stop() {
	int rc = EOK;

	qDebug() << "Stopping mm-renderer properly";

	//Stop mm-renderer
	if ((rc = mmr_stop(mmr_context))) {
		qDebug() << "mmr - Error stopping" << rc;
		return rc;
	}

	if ((rc = mmr_output_detach(mmr_context, audio_device_output_id))) {
		qDebug() << "mmr - Error detaching audio" << rc;
		return rc;
	}

	if ((rc = mmr_output_detach(mmr_context, video_device_output_id))) {
		qDebug() << "mmr - Error detaching video" << rc;
		return rc;
	}

	if ((rc = mmr_context_destroy(mmr_context))) {
		qDebug() << "mmr - Error destroying context" << rc;
		return rc;
	}

	mmr_context = 0;
	video_device_output_id = -1;
	audio_device_output_id = -1;

	mmr_disconnect(mmr_connection);
	mmr_connection = 0;

	return rc;
}
