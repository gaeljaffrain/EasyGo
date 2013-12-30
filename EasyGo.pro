APP_NAME = EasyGo

CONFIG += qt warn_on cascades10

QT += network
LIBS += -lbbplatformbbm -lbbsystem
LIBS += -lstrm -lmmrndclient -lscreen

include(config.pri)
