QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Added to provide backwards compatibility (C++11 support)
greaterThan(QT_MAJOR_VERSION, 4) {
    CONFIG += c++11
} else {
    QMAKE_CXXFLAGS += -std=c++11
}

TARGET = mcp2221-conf
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    common.cpp \
    configuratorwindow.cpp \
    libusb-extra.c \
    main.cpp \
    mainwindow.cpp \
    mcp2221.cpp

HEADERS += \
    aboutdialog.h \
    common.h \
    configuratorwindow.h \
    libusb-extra.h \
    mainwindow.h \
    mcp2221.h

FORMS += \
    aboutdialog.ui \
    configuratorwindow.ui \
    mainwindow.ui

LIBS += -lusb-1.0

RESOURCES += \
    resources.qrc

!isEmpty(target.path): INSTALLS += target
