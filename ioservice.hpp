//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File ioservice.hpp
// Date 11.03.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _IO_SERVICE_H_
#define _IO_SERVICE_H_

//***************************************************************************
// Include
//***************************************************************************

class IoService
{
   public:

      enum Misc
      {
         protocol      = 0x50,
         protocolMask  = 0xE0,            // -> 11100000
         commandMask   = ~protocolMask
      };

      enum Command
      {
         cNone = 0,

         // from PC

         cGetTime            = 0x01,
         cDigitalOut         = 0x02,
         cDigitalOutBit      = 0x03,
         cAnalogOut          = 0x04,
         cGetInputs          = 0x05,
         cRecordGhostCar     = 0x06,
         cStartGhostCar      = 0x07,
         cStopGhostCar       = 0x08,
         cGhostCarValue      = 0x09,
         cSetupIo            = 0x0A,
         cGhostCarFlush      = 0x0B,

         // to PC

         cGhostCarBufferFull = 0x0E,
         cDigitalIn          = 0x0F,
         cAnalogIn           = 0x10,
         cBoardTime          = 0x11,
         cDebug              = 0x12
      };

      enum GhostCarScale
      {
         // interrupt called every 1ms, this is the scale factor for
         // the ghost car recording intervall

         gcScale100 = 100,           // intervall 100ms
         gcScale200 = 200,           // intervall 200ms
         gcScale300 = 300,           // intervall 300ms
         gcScale400 = 400,           // intervall 400ms
         gcScale500 = 500,           // intervall 500ms
      };

#ifndef MKSKETCH
#  pragma pack(push, 1)
#endif

      // from PC

      struct DigitalOutput      // 4 + 2 byte
      {
          dword value;
      };

      struct DigitalOutputBit   // 2 + 2 byte
      {
          byte bit;
          byte state;
      };

      struct GhostCarValue      // 2 + 2 byte
      {
          byte volt;
          byte ampere;
      };

      struct GhostCarStart      // 6 + 2 byte
      {
         char cycle;            // outout cycle
         byte bit;              // pwm output bit
         byte ampereBit;        // ampereIn
         char controlCycle;     // regel Zyklus (na -> off)
         byte decFactor;
         byte incFactor;
      };

      struct AnalogOutput       // 2 + 2 byte
      {
         byte bit;
         byte value;
      };

      struct RecordGhostCar     // 2 + 2 byte
      {
         byte cycle;
         char voltBit;
         char ampereBit;
      };

      struct SetupIo            // 5 + 2 byte
      {
         word bitsInput;
         word bitsOutput;
         byte withSpiExtension;
      };

      // to PC

      struct DigitalInput       // 8 + 2 byte
      {
         dword time;
         dword value;
      };

      struct AnalogInput        // 2 + 2 byte
      {
         byte volt;
         byte ampere;
      };

      struct DebugValue         // 58 + 2 byte
      {
         char string[49+TB];
         dword value;
      };

#ifndef MKSKETCH
#  pragma pack(pop)
#endif

};

typedef IoService Ios;

//***************************************************************************
#endif //  _IO_SERVICE_H_
