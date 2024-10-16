QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Utility/DumpUtil.cpp \
    Utility/IcrCriticalSection.cpp \
    Utility/ImCharset.cpp \
    Utility/ImPath.cpp \
    Utility/LogBuffer.cpp \
    Utility/LogUtil.cpp \
    chargedialog.cpp \
    chargephonedialog.cpp \
    couponbuyer.cpp \
    couponquerier.cpp \
    datamodel.cpp \
    httpclientbase.cpp \
    logincontroller.cpp \
    main.cpp \
    mainwindow.cpp \
    mfhttpclient.cpp \
    multichargecontroller.cpp \
    settingdialog.cpp \
    settingmanager.cpp \
    singlechargecontroller.cpp \
    uiutil.cpp \
    yqbhttpclient.cpp

HEADERS += \
    Utility/DumpUtil.h \
    Utility/IcrCriticalSection.h \
    Utility/ImCharset.h \
    Utility/ImPath.h \
    Utility/LogBuffer.h \
    Utility/LogMacro.h \
    Utility/LogUtil.h \
    chargedialog.h \
    chargephonedialog.h \
    couponbuyer.h \
    couponquerier.h \
    datamodel.h \
    httpclientbase.h \
    logincontroller.h \
    mainwindow.h \
    mfhttpclient.h \
    multichargecontroller.h \
    settingdialog.h \
    settingmanager.h \
    singlechargecontroller.h \
    uiutil.h \
    yqbhttpclient.h

FORMS += \
    chargedialog.ui \
    chargephonedialog.ui \
    mainwindow.ui \
    settingdialog.ui

# Enable PDB generation
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

# Enable log context
DEFINES += QT_MESSAGELOGCONTEXT

# QXlsx code for Application Qt project
include(../QXlsx/QXlsx.pri)
INCLUDEPATH += ../QXlsx/header/
