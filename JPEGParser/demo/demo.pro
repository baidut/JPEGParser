QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QHexEditor
TEMPLATE = app

HEADERS += \
    mainwindow.h \
    optionsdialog.h \
    searchdialog.h \
    tjpgdec/integer.h \
    tjpgdec/tjpgd.h


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    optionsdialog.cpp \
    searchdialog.cpp \
    tjpgdec/tjpgd.c

RESOURCES += \
    qhexedit.qrc

FORMS += \
    optionsdialog.ui \
    searchdialog.ui \
    mainwindow.ui

TRANSLATIONS += \
    translations/qhexedit_cs.ts \
    translations/qhexedit_de.ts

include(../src/qhexedit_widget.pri)
