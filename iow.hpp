//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iow.hpp
// Date 05.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _IOW_H_
#define _IOW_H_

//***************************************************************************
// Include
//***************************************************************************

#include "iowarrior.h"

#include <iointerface.hpp>

// default device: /dev/usb/iowarrior

//***************************************************************************
// IO Warrior
//***************************************************************************

class Iow : public IoInterface, public SlotService
{

   public:

      // declarations

#     pragma pack(push, 1)

      struct IoValue
      {
         union
         {
            unsigned int dword;
            unsigned char bytes[4];
         };
      };

#     pragma pack(pop)

      typedef void* IowHandle;

      enum Misc
      {
         maxDevices = 16
      };

      struct IowDevice
      {
         int dev_num;                      // numeric part of device name for pipe 0
         iowarrior_info info;              // all information available for this device
         int fdIO;                         // the filedescriptor for the io-pins
         timeval readTimeout;              // timeout for reading from the device
         timeval writeTimeout;             // timeout for writing to the device, not used on linux
         unsigned int readTimeoutVal;      // read timeout from SetTimeout
         unsigned int writeTimeoutVal;     // write timeout from SetWriteTimeout
         unsigned int lastValue;
      };

#     pragma pack(push, 1)

      struct Report
      {
         unsigned char ReportID;
         
         union
         {
            unsigned int Value;
            unsigned char Bytes[4];
         };
      };



      struct Report24
      {
         unsigned char ReportID;

         union
         {
            unsigned short Value;
            unsigned char Bytes[2];
         };
      };

#     pragma pack(pop)

      // object

      Iow();

      // interface

      int open(const char* dev = 0);
      int close();
      int reopen(const char* dev = 0);
      int isOpen()                        { return devHandle != 0; }
      int flush();

      int writeBit(int bit, int value);
      int readOutBit(int bit);
      int writeByte(unsigned char byte, int byteNum);
      int readValue(unsigned int* value);
      int lookByte(int byteNum, unsigned  char* byte);

      int getNumDevs()    { return numIoWarriors; }

      int setTimeout(unsigned int timeout);
      int setWriteTimeout(unsigned int timeout);

   protected:

      // functions

      void clear();
      IowHandle GetDeviceHandle(unsigned int numDevice);
      IowDevice* GetDeviceByHandle(IowHandle iowHandle);

      bool ReadImmediate(unsigned int* value);
      int Read(char* buffer, int length);
      int Write(char* buffer, int length);

      // data

      Iow::IowHandle devHandle;
      IowDevice _IowDevice;
      unsigned int numIoWarriors;
      IowDevice IoWarriors[maxDevices];
      unsigned char out[2];
};

//***************************************************************************
#endif // _IOW_H_
