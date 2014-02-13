# The name of your app.
# NOTICE: name defined in TARGET has a corresponding QML filename.
#         If name defined in TARGET is changed, following needs to be
#         done to match new name:
#         - corresponding QML filename must be changed
#         - desktop icon filename must be changed
#         - desktop filename must be changed
#         - icon definition filename in desktop file must be changed
TARGET = gcide

QT += xml

CONFIG += sailfishapp

SOURCES += src/gcide.cpp \
    src/dict.cpp

OTHER_FILES += qml/gcide.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    rpm/gcide.spec \
    rpm/gcide.yaml \
    gcide.desktop

dictionary.path = /usr/share/gcide
dictionary.files = gcide-entries.xml

INSTALLS += dictionary

HEADERS += \
    src/dict.h
