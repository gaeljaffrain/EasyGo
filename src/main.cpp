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

#include <bb/cascades/Application>

#include <QLocale>
#include <QTranslator>
#include <Qt/qdeclarativedebug.h>

#include "easygo.hpp"
#include "RegistrationHandler.hpp"

using namespace bb::cascades;

Q_DECL_EXPORT int main(int argc, char **argv)
{
    Application app(argc, argv);

    // Define EasyGo UUID
	const QUuid uuid(QLatin1String("c070d720-431d-11e3-aa6e-0800200c9a66"));

	// Register with BBM.
	RegistrationHandler* registrationHandler = new RegistrationHandler(uuid, &app);

    // Create the EasyGo object, this is where the main.qml file
    // is loaded and the application scene is set.
    EasyGo* mainApp = new EasyGo(registrationHandler->context(), &app);

    QObject::connect(registrationHandler, SIGNAL(stateChanged()), mainApp, SLOT(show()));

	// Now that the signal is connected to the slot we can fire the registration process
	// When BBM registration process will be finished, then the main UI will be shown
	registrationHandler->registerApplication ();

	// We complete the transaction started in the main application constructor and start the
	// client event loop here. When loop is exited the Application deletes the scene which
	// deletes all its children.
    return Application::exec();
}
