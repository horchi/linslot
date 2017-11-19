//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iointerface.hpp
// Date 16.11.07 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _IO_INTERFACE_H_
#define _IO_INTERFACE_H_

#include <common.hpp>
#include <ioservice.hpp>

//***************************************************************************
// IO Interface
//***************************************************************************

class IoInterface : public IoService
{
   public:

      // object

      IoInterface();
      virtual ~IoInterface();

      // interface

      virtual int open(const char* dev = 0) = 0;
      virtual int close() = 0;
      virtual int reopen(const char* dev = 0) = 0;
      virtual int isOpen() = 0;
      virtual int connected() = 0;
      virtual int flush() = 0;
      virtual int getGcScale()  { return gcScale100; }
      virtual byte* getMessage() = 0;
      virtual int look(byte& command) = 0;

      // special io functions

      virtual void recordGhostCar(char /*vBit*/, char /*iBit*/) {}
      virtual void startGhostCar(int /*pwmBit*/, int/* iBit*/) {}
      virtual void stopGhostCar() {}
      virtual void writeGhostCarValue(byte /*volt*/, byte /*ampere*/) {}
      virtual void flushGhostCar() {}
      virtual int initIoSetup(word /*bitsInput*/, word /*bitsOutput*/,
                              byte /*withSpi*/) { return done; }

      // read/write

      virtual int writeBit(int bit, int value) = 0;
      virtual int readOutBit(int bit) = 0;

      // settings

      virtual int setTimeout(unsigned int timeout) = 0;
      virtual int setWriteTimeout(unsigned int timeout) = 0;
      virtual timeval getBoardStartTime() { return tvBoardStartTime; }

   protected:

      char deviceName[100+TB];
      timeval tvBoardStartTime;
};

//***************************************************************************
#endif // _IO_INTERFACE_H_
