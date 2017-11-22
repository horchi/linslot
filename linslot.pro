#**************************************************************************
# Group Linslot / Linux - Slotrace Manager
# File setup.pro
# Date 06.12.06 - Jörg Wendel
# This code is distributed under the terms and conditions of the
# GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
#***************************************************************************

#----------------------------------------------------------
# select your hardware
#----------------------------------------------------------

interface = arduino

#----------------------------------------------------------
# Definitions
#----------------------------------------------------------

TEMPLATE  = app
CONFIG    += qt debug

QT += sql
QT += widgets

FORMS       = linslot.ui setup.ui highscore.ui

DEPENDPATH  += .
INCLUDEPATH += .
LIBS        += -lsqlite3
HEADERS     += linslot.hpp iothread.hpp common.hpp iointerface.hpp \
               setup.hpp sqlite.hpp list.hpp highscore.hpp \
               iointerface.hpp lapprofile.hpp delegateitems.hpp arduino.hpp

SOURCES     += main.cc arduino.cc linslot.cc iothread.cc common.cc \
               setup.cc sqlite.cc list.cc highscore.cc \
               iointerface.cc lapprofile.cc delegateitems.cc

# Linux / Unix

unix:HEADERS      += alsa.hpp
unix:SOURCES      += alsa.cc
unix:LIBS         += -lalsatoss -lasound
# unix:LIBPATH    += /usr/local/lib
unix:QMAKE_LIBDIR += /usr/local/lib

# Win...

win32:DEFINES     += _TTY_WIN_
win32:INCLUDEPATH += ./win
win32:LIBPATH     += ./win
win32:LIBS        += -lsetupapi
win32:DEFINES     += _CRT_SECURE_NO_DEPRECATE

message("Building  ...")
