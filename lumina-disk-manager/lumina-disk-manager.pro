#
# Copyright (c) 2018, Ole-André Rodlie <ole.andre.rodlie@gmail.com> All rights reserved.
#
# Available under the 3-clause BSD license
# See the LICENSE file for full details
#

QT += core gui dbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = lumina-disk-manager
VERSION = 0.2
TEMPLATE = app

SOURCES += main.cpp systray.cpp manager.cpp
HEADERS += systray.h udisks2.h manager.h

include(../lumina-extra.pri)

target.path = $${PREFIX}/bin
target_desktop.path = $${XDGDIR}/autostart
target_desktop.files = $${TARGET}.desktop
target_docs.path = $${DOCDIR}/$${TARGET}-$${VERSION}
target_docs.files = ../LICENSE ../README.md
INSTALLS += target target_desktop target_docs
