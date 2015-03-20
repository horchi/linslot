//***************************************************************************
// Group Common Stuff
// File common.hpp
// Date 04.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

//***************************************************************************
// Includes
//***************************************************************************

#include <qglobal.h>
#include <QString>

#ifndef Q_OS_WIN32
#  include <sys/time.h>
#endif

#include <time.h>

#ifdef Q_OS_WIN32

#  include <winsock.h>

   int gettimeofday(struct timeval *tv, struct timezone *tz);
 
   unsigned int usleep(unsigned int usec);
   unsigned int sleep(unsigned int sec);

#  define strcasecmp _stricmp
#  define strdup _strdup

#endif

//***************************************************************************
// Misc
//***************************************************************************

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

enum Result
{
   success = 0,
   fail = -1,
   na = -1,
   done = 0,
   ignore = -1,

   yes = 1,
   no = 0,

   on = 1,
   off = 0,

   TB = 1
};

enum Eloquence
{
   eloOff,                   // 0
   eloAlways,                // 1
   eloDetail,                // 2
   eloDebug,                 // 3
   eloDebug1 = eloDebug,     // 3
   eloDebug2,                // 4
   eloDebug3                 // 5
};

extern int theEloquence;
extern QString logFile;
extern int withSound;

int tell(int eloquence, const char* format, ...);
int tell(const char* format, ...);
void tellFile(const char* msg);
void playSound(QString file);
void checkSound();

const char* notNull(const char* s);

//***************************************************************************
// Slot Service
//***************************************************************************

class SlotService
{
   public:

      // declarations

      // Signals

      enum InputSignal
      {
         bitStartKey,         // external start/stop key

         bitPanicKey1,        // panic keys
         bitPanicKey2,
         bitPanicKey3,
         bitPanicKey4,

         bitIrSlot1,          // slot signal
         bitIrSlot2,
         bitIrSlot3,
         bitIrSlot4,

         bitFuelStartSlot1,   // fueling start signal
         bitFuelStartSlot2,
         bitFuelStartSlot3,
         bitFuelStartSlot4,

         bitFuelEndSlot1,     // fueling end signal
         bitFuelEndSlot2,
         bitFuelEndSlot3,
         bitFuelEndSlot4,

         bitInputCount
      };

      enum OutputSignal
      {
         bitLightRed1,
         bitLightRed2,
         bitLightRed3,
         bitLightRed4,
         bitLightRed5,
         bitLightGreen,

         bitPowerIndSlot1,
         bitPowerIndSlot2,
         bitPowerIndSlot3,
         bitPowerIndSlot4,

         bitPowerSlot1,
         bitPowerSlot2,
         bitPowerSlot3,
         bitPowerSlot4,

         bitPwmOutSlot1,
         bitPwmOutSlot3,

         bitFuelingIndSlot1,
         bitFuelingIndSlot2,
         bitFuelingIndSlot3,
         bitFuelingIndSlot4,

         bitPenaltyIndSlot1,
         bitPenaltyIndSlot2,
         bitPenaltyIndSlot3,
         bitPenaltyIndSlot4,

         fctOutputCount
      };

      enum AnalogInputs
      {
         fctGhostUSlot1,
         fctGhostUSlot3,
         fctGhostISlot1,
         fctGhostISlot3,
         fctAnalogInFree0,
         fctAnalogInFree1,

         fctAnalogCount
      };

      enum LedId
      {
         ledNone    = 0,

         ledGreen   = 1,
         ledRed1    = 2,
         ledRed2    = 4,
         ledRed3    = 8,
         ledRed4    = 16,
         ledRed5    = 32,
         ledYellow1 = 64,
         ledYellow2 = 128,
         ledYellow3 = 256,
         ledYellow4 = 512,

         ledCount = 11 
      };

      // HW parameters

      enum TriggerEdge
      {
         teRising,
         teFalling
      };

      enum OutputMode
      {
         omUnknown = na,

         omNormal,
         omPwm,
         omFlashUpFast,
         omFlashDownFast,
         omFlashUpSlow,
         omFlashDownSlow,

         omCount
      };

      enum SoundFunction
      {
         sfRaceStart,
         sfRaceFinished,
         sfRaceAborted,
         sfCountdownPhase,
         sfLapSignal,
         sfFuelingStart,
         sfFuelingInterrupted,
         sfFuelingFinished,
         sfJumpTheGun,
         sfFuelEmpty,

         sfCount
      };

      struct InputDefinition
      {
         int bit;
         TriggerEdge mode;
      };

      struct OutputDefinition
      {
         int bit;
         OutputMode mode;
         LedId ledid;
      };

      struct AnalogInDefinition
      {
         int bit;
      };

      // IO Events

      struct DigitalEvent
      {
         unsigned int value;
         timeval tp;
      };

      struct AnalogEvent
      {
         byte volt;
         byte ampere;
      };

      struct Led
      {
         int id;
         const char* name;
      };

      struct SoundSignal
      {
         const char* name;
         QString sound;
      };

      // bit manipulation

      static int isBit(unsigned int value, int bit);
      static void toggleBit(unsigned int& value, int bit);
      static void setBitTo(unsigned int& value, int bit, int state);

      static const char* toBinStr(unsigned int value, char* mask, int bits = 32);

      // misc 

      static double kmh(double meter, double usec);
      static const char* toStr(int value);
      
      // time stuff
      
      static void tvNull(timeval* tv);
      static timeval* tvNow(timeval* tv);
      static timeval addMs2Tv(timeval tv, unsigned long milli);
      static timeval subMs2Tv(timeval tv, unsigned long milli);

      static long long elapsed(const timeval* tpFrom, const timeval* tpNow);
      static const char* secStr(long usec, char* buf);

      //

      static const char* inputFunctionName(int fct);
      static const char* outputFunctionName(int fct);
      static const char* analogInFunctionName(int fct);

      static const char* toName(OutputMode mode);
      static OutputMode toOutputMode(const char* name);

      const char* toName(LedId id);
      static LedId toLedId(const char* name);

      // data

      static InputDefinition inputBits[bitInputCount];
      static OutputDefinition outputBits[fctOutputCount];
      static AnalogInDefinition analogBits[fctAnalogCount];
      static Led leds[ledCount];
      static SoundSignal sounds[sfCount];

      static const char* inputFunctions[];
      static const char* outputFunctions[];
      static const char* analogInFunctions[];
      static const char* outputModes[];
};

//***************************************************************************
#endif // __COMMON_HPP__
