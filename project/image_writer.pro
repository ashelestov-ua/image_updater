TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        bootupdater.cpp \
        imageupdater.cpp \
        main.cpp

LIBS += -lssl -lcrypto -ljsoncpp -lblkid

HEADERS += \
    bootupdater.h \
    imageupdater.h

