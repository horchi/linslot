//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File arduino.hpp
// Date 16.11.07 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _IO_ARDUINO_H_
#define _IO_ARDUINO_H_

//***************************************************************************
// Include
//***************************************************************************

#ifndef Q_OS_WIN32
#  include <termios.h>
#else
#  include <windows.h>
#endif

#include <QMutex>

#include <common.hpp>
#include <iointerface.hpp>

//***************************************************************************
// IO Interface
//***************************************************************************

class Arduino : public IoInterface, public SlotService
{
   public:

      enum Misc
      {
         sizeCmdMax = 100
      };

      // object

      Arduino();
      virtual ~Arduino();

      // interface

      virtual int open(const char* dev = 0);
      virtual int close();
      virtual int reopen(const char* dev = 0);
      virtual int isOpen()              { return fdDevice != 0 && opened; }
      virtual int connected();
      virtual int flush();
      virtual int look(byte& command);
      virtual byte* getMessage()        { if (*message) return message; return 0; };
      virtual byte getMessageSize()     { return messageSize; };

      // special io functions

      virtual void recordGhostCar(char vBit, char iBit);
      virtual void startGhostCar(int pwmBit, int iBit);
      virtual void stopGhostCar();
      virtual void writeGhostCarValue(byte volt, byte ampere);
      virtual void flushGhostCar();
      virtual int initIoSetup(word bitsInput, word bitsOutput, byte withSpi);

      // read / write

      virtual int writeBit(int bit, int state);
      virtual int sendCommand(byte command, void* line = 0, byte size = 0);
      virtual int readOutBit(int bit);
      virtual void analogWrite(int bit, byte value);

      // settings

      virtual int setTimeout(unsigned int timeout);
      virtual int setWriteTimeout(unsigned int timeout);

      timeval getBoardStartTime() { return tvBoardStartTime; }

   protected:

      virtual byte receiveCommand();
      virtual int _look();
      int initBoardTime();
      int read(void* buf, unsigned int count);

      // data

      byte message[sizeCmdMax+TB];
      byte messageSize;
      byte command;

      int opened;
      int readTimeout;
      int writeTimeout;
      unsigned int outValue;
      QMutex deviceMutex;

#ifndef Q_OS_WIN32
      int fdDevice;
      struct termios oldtio;
#else
      HANDLE fdDevice;
#endif
};

//***************************************************************************
#endif // _IO_INTERFACE_H_
