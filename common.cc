//***************************************************************************
// Group Common Stuff
// File common.cc
// Date 04.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <alsa.hpp>
#endif

// #include <QSound>
// #include <QtTest/QTest>
#include <QFile>
#include <QMessageBox>

#include <common.hpp>

//***************************************************************************
// Globals
//***************************************************************************

int theEloquence;
QString logFile;
int withSound;

//***************************************************************************
// Tell
//***************************************************************************

int tell(int eloquence, const char* format, ...)
{
   const int sizeDate = 8;        // "12.04.67"
   const int sizeTime = 8;        // "12:12:34"
   const int sizeDateTime = sizeDate + 1 + sizeTime;
   const int sizeMSec = 4;        // ",142"
   const int sizeHeader = sizeDateTime + sizeMSec + 1;
   const int maxBuf = 1000;

   struct timeval tp;
   char buf[maxBuf+TB];
   va_list ap;
   time_t now;

   va_start(ap, format);

   if (theEloquence >= eloquence)
   {
      time(&now);
      gettimeofday(&tp, 0);

      vsnprintf(buf + sizeHeader, maxBuf - sizeHeader, format, ap);
      strftime(buf, sizeDateTime+1, "%y.%m.%d %H:%M:%S", localtime(&now));

      sprintf(buf+sizeDateTime, ",%3.3ld", tp.tv_usec / 1000);

      buf[sizeHeader-1] = ' ';

      if (logFile.size())
         tellFile(buf);
      else
         printf("%s\n", buf);
   }

   va_end(ap);

   return 0;
}

//***************************************************************************
// Tell To File
//***************************************************************************

void tellFile(const char* msg)
{
   QFile file(logFile);

   if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
      QMessageBox::information(0, "Info", "Opening " + logFile + " failed");

   file.write(msg, qstrlen(msg));
   file.write("\n", qstrlen("\n"));

   file.close();
}

//***************************************************************************
// Play Sound
//***************************************************************************

void playSound(QString file)
{
   if (withSound)
   {
#ifndef Q_OS_WIN32
      QAlsaSound::play(file);
#else
      QSound::play(file);
#endif
   }
}

//***************************************************************************
// Check Sound
//***************************************************************************

void checkSound()
{
#ifdef Q_OS_WIN32
   withSound = QSound::isAvailable();
#else
   withSound = QAlsaSound::isAvailable();
#endif

   if (!withSound)
      tell(eloAlways, "No sound available");
}

//***************************************************************************
// Not Null
//***************************************************************************

const char* notNull(const char* s)
{
   return s ? s : "";
}

//***************************************************************************
// Class SlotService
//***************************************************************************
//***************************************************************************
// Is Bit
//***************************************************************************

int SlotService::isBit(unsigned int value, int bit)
{
   return value & (1 << bit);
}

//***************************************************************************
// Toggle Bit
//***************************************************************************

void SlotService::toggleBit(unsigned int& value, int bit)
{
   setBitTo(value, bit, !isBit(value, bit));
}

//***************************************************************************
// Set Bit To
//***************************************************************************

void SlotService::setBitTo(unsigned int& value, int bit, int state)
{
   if (state)
      value |= 1 << bit;
   else
      value &= ~(1 << bit);
}

//***************************************************************************
// To Bin Str
//***************************************************************************

const char* SlotService::toBinStr(unsigned int value, char* mask, int bits)
{
   int bytes = bits / 8;
   int bit = 1;

   if (bytes * 8 < bits)
      bytes++;

   for (int i = 0, b = 0; b < bits; i++, b++)
   {
      if (!(b % 8) && b)
      {
         mask[bits+bytes-i-2] = ':';
         i++;
      }

      mask[bits+bytes-i-2] = (value & bit) ? '1' : '0';
      bit = bit << 1;
   }

   mask[bits+bytes-1] = 0;

   return mask;
}

//***************************************************************************
// kmh
//***************************************************************************

double SlotService::kmh(double meter, double usec)
{
   return ((double)meter/1000L)  /
      ((double)usec / 1000000L / 60L / 60L);
}

//***************************************************************************
// Init TV
//***************************************************************************

void SlotService::tvNull(timeval* tv)
{
   memset(tv, 0, sizeof(timeval));
}

timeval* SlotService::tvNow(timeval* tv)
{
   gettimeofday(tv, 0);
   return tv;
}

//***************************************************************************
// Add/Sub from timeval
//***************************************************************************

timeval SlotService::addMs2Tv(timeval tv, unsigned long milli)
{
   timeval tvSum;
   unsigned long long usec = tv.tv_usec + (milli % 1000) * 1000;

   tvSum.tv_usec = usec % 1000000;
   tvSum.tv_sec  = tv.tv_sec + (milli / 1000) + (usec / 1000000);

   return tvSum;
}

timeval SlotService::subMs2Tv(timeval tv, unsigned long milli)
{
   unsigned long long sec = (milli / 1000);
   long long usec = (milli % 1000) * 1000;

   tv.tv_sec -= sec;
   usec = tv.tv_usec - usec;

   if (usec > 0)
      tv.tv_usec = usec;
   else
   {
      tv.tv_sec--;
      tv.tv_usec = 1000000 + usec;
   }

   return tv;
}

//***************************************************************************
// Elapsed in µSeconds
//***************************************************************************

long long SlotService::elapsed(const timeval* tpFrom, const timeval* tpNow)
{
   long long sec = tpNow->tv_sec - tpFrom->tv_sec;
   long long usec = sec*1000000 + tpNow->tv_usec;

   usec -= tpFrom->tv_usec;

   return usec;
}

//***************************************************************************
// Seconds String
//***************************************************************************

const char* SlotService::secStr(long usec, char* buf)
{
   sprintf(buf, "%2ld.%06ld", usec/1000000, usec%1000000);

   return buf;
}

//***************************************************************************
// To Str
//***************************************************************************

const char* SlotService::toStr(int value)
{
   static char buf[10+TB];

   sprintf(buf, "%d", value);

   return buf;
}

//***************************************************************************
// Input Functions
//***************************************************************************

SlotService::InputDefinition SlotService::inputBits[] =
{
   // bit, mode,              function

   {   4, teFalling  },   //  StartKey

   {  16, teRising   },   //  PanicKey1
   {  17, teRising   },   //  PanicKey2
   {  18, teRising   },   //  PanicKey3
   {  19, teRising   },   //  PanicKey4

   {  20, teFalling  },   //  IrSlot1
   {  21, teFalling  },   //  IrSlot2
   {  22, teFalling  },   //  IrSlot3
   {  23, teFalling  },   //  IrSlot4

   {  24, teFalling  },   //  FuelStartSlot1
   {  25, teFalling  },   //  FuelStartSlot2
   {  26, teFalling  },   //  FuelStartSlot3
   {  27, teFalling  },   //  FuelStartSlot4

   {  28, teFalling  },   //  FuelEndSlot1
   {  29, teFalling  },   //  FuelEndSlot2
   {  30, teFalling  },   //  FuelEndSlot3
   {  31, teFalling  }    //  FuelEndSlot4
};

const char* SlotService::inputFunctions[] =
{
   "Start Key",

   "Panic Key 1",
   "Panic Key 2",
   "Panic Key 3",
   "Panic Key 4",

   "Signal Slot 1",
   "Signal Slot 2",
   "Signal Slot 3",
   "Signal Slot 4",

   "Fuel Start Slot 1",
   "Fuel Start Slot 2",
   "Fuel Start Slot 3",
   "Fuel Start Slot 4",

   "Fuel End Slot 1",
   "Fuel End Slot 2",
   "Fuel End Slot 3",
   "Fuel End Slot 4",
};

const char* SlotService::inputFunctionName(int fct)
{
   if (fct < 0 || fct > bitInputCount)
      return "<unknown>";

   return inputFunctions[fct];
}

//***************************************************************************
// Output Functions
//***************************************************************************

SlotService::OutputDefinition SlotService::outputBits[] =
{
   // bit, mode,           function

   {  16, omNormal,        ledRed1     },    //  Red Phase 1
   {  17, omNormal,        ledRed2     },    //  Red Phase 2
   {  18, omNormal,        ledRed3     },    //  Red Phase 3
   {  19, omNormal,        ledRed4     },    //  Red Phase 4
   {  20, omNormal,        ledRed5     },    //  Red Phase 5
   {  21, omNormal,        ledGreen    },    //  Green

   {  24, omNormal,        ledYellow1  },    //  Indicator Power Slot 1
   {  25, omNormal,        ledYellow2  },    //  Indicator Power Slot 2
   {  26, omNormal,        ledYellow3  },    //  Indicator Power Slot 3
   {  27, omNormal,        ledYellow4  },    //  Indicator Power Slot 4

   {  28, omNormal,        ledNone     },    //  Power Slot 1
   {  29, omNormal,        ledNone     },    //  Power Slot 2
   {  30, omNormal,        ledNone     },    //  Power Slot 3
   {  31, omNormal,        ledNone     },    //  Power Slot 4

   {   5, omPwm,           ledNone     },    //  Pwm Out Slot 1
   {   6, omPwm,           ledNone     },    //  Pwm Out Slot 3

   {  24, omFlashUpSlow,   ledYellow1  },    // Indicator Fueling Slot 1
   {  25, omFlashDownSlow, ledYellow2  },    // Indicator Fueling Slot 2
   {  26, omFlashUpSlow,   ledYellow3  },    // Indicator Slot 3
   {  27, omFlashDownSlow, ledYellow4  },    // Indicator Slot 4

   {  24, omFlashUpFast,   ledYellow1  },    // Indicator Penalty Slot 1
   {  25, omFlashUpFast,   ledYellow2  },    // Indicator Penalty Slot 2
   {  26, omFlashUpFast,   ledYellow3  },    // Indicator Penalty Slot 3
   {  27, omFlashUpFast,   ledYellow4  }     // Indicator Penalty Slot 4
};


const char* SlotService::outputFunctions[] =
{
   "Red Phase 1",
   "Red Phase 2",
   "Red Phase 3",
   "Red Phase 4",
   "Red Phase 5",
   "Green",

   "Indicator Power Slot 1   *(1)",
   "Indicator Power Slot 1   *(2)",
   "Indicator Power Slot 2   *(3)",
   "Indicator Power Slot 2   *(4)",

   "Power Slot 1",
   "Power Slot 2",
   "Power Slot 3",
   "Power Slot 4",

   "PWM Slot 1",
   "PWM Slot 3",

   "Indicator Fueling Slot 1   *(1)",
   "Indicator Fueling Slot 1   *(2)",
   "Indicator Fueling Slot 2   *(3)",
   "Indicator Fueling Slot 2   *(4)",

   "Indicator Penalty Slot 1   *(1)",
   "Indicator Penalty Slot 1   *(2)",
   "Indicator Penalty Slot 2   *(3)",
   "Indicator Penalty Slot 2   *(4)",

};

const char* SlotService::outputFunctionName(int fct)
{
   if (fct < 0 || fct > fctOutputCount)
      return "<unknown>";

   return outputFunctions[fct];
}

//***************************************************************************
// Analog Input Functions
//***************************************************************************

SlotService::AnalogInDefinition SlotService::analogBits[] =
{
   // bit,         function

   {  0   },   //  Spannung Slot 1
   {  1   },   //  Spannung Slot 3
   {  2   },   //  Strom Slot 1
   {  3   },   //  Strom Slot 3
   {  4   },   //  Frei 1
   {  5   }    //  Frei 2
};

const char* SlotService::analogInFunctions[] =
{
   "Spannung Slot 1",
   "Spannung Slot 3",
   "Strom Slot 1",
   "Strom Slot 3",
   "Frei 1",
   "Frei 2"
};

const char* SlotService::analogInFunctionName(int fct)
{
   if (fct < 0 || fct > fctAnalogCount)
      return "<unknown>";

   return analogInFunctions[fct];
}

//***************************************************************************
// Sound Definition
//***************************************************************************

SlotService::SoundSignal SlotService::sounds[] =
{
   { "Race start",           "explos.wav"   },
   { "Race finished",        "left.wav"     },
   { "Race aborted",         "applause.wav" },
   { "Countdown phase",      "pluck.wav"    },
   { "Lap signal",           "notify.wav"   },
   { "Lap signal (fastest)", "fall.wav"     },
   { "Fueling start",        "pass.wav"     },
   { "Fueling interrupted",  "coin.wav"     },
   { "Fueling finished",     "coin.wav"     },
   { "'Jump the gun'",       "send.wav"     },
   { "Fuel empty",           "skid.wav"     }
};

//***************************************************************************
// Leds
//***************************************************************************

SlotService::Led SlotService::leds[] =
{
   // id, led

   { ledNone,    "NA"    },
   { ledGreen,   "Green"    },
   { ledRed1,    "Red 1"    },
   { ledRed2,    "Red 2"    },
   { ledRed3,    "Red 3"    },
   { ledRed4,    "Red 4"    },
   { ledRed5,    "Red 5"    },
   { ledYellow1, "Yellow 1" },
   { ledYellow2, "Yellow 2" },
   { ledYellow3, "Yellow 3" },
   { ledYellow4, "Yellow 4" }
};

const char* SlotService::toName(LedId id)
{
   for (int i = 0; i < ledCount; i++)
      if (leds[i].id == id)
         return leds[i].name;

   return "<unknown>";
}

SlotService::LedId SlotService::toLedId(const char* name)
{
   for (int i = 0; i < ledCount; i++)
      if (strcmp(leds[i].name, name) == 0)
         return (LedId)leds[i].id;

   return ledNone;
}

//***************************************************************************
// Output Modes
//***************************************************************************

const char* SlotService::outputModes[] =
{
   "normal",
   "pwm",
   "flash-up fast",
   "flash-down fast",
   "flash-up slow",
   "flash-down slow"
};

const char* SlotService::toName(OutputMode mode)
{
   if (mode < 0 || mode > omCount)
      return "<unknown>";

   return outputModes[mode];
}

SlotService::OutputMode SlotService::toOutputMode(const char* name)
{
   for (int i = 0; i < omCount; i++)
      if (strcmp(outputModes[i], name) == 0)
         return (OutputMode)i;

   return omUnknown;
}

//***************************************************************************
// Windows
//***************************************************************************
//***************************************************************************
// usleep / sleep
//***************************************************************************

#ifdef Q_OS_WIN32

unsigned int usleep(unsigned int usec)
{
   Sleep(usec/1000);
   return 0;
}

unsigned int sleep(unsigned int sec)
{
   Sleep(sec*1000);
   return 0;
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
  int tz_minuteswest; /* minutes W of Greenwich */
  int tz_dsttime;     /* type of dst correction */
};

//***************************************************************************
// gettimeofday
//***************************************************************************

int gettimeofday(struct timeval* tv, struct timezone* tz)
{
   FILETIME ft;
   unsigned __int64 tmpres = 0;
   static int tzflag;

   if (tv)
   {
      GetSystemTimeAsFileTime(&ft);

      tmpres |= ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*converting file time to unix epoch*/
      tmpres /= 10;  /*convert into microseconds*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;
      tv->tv_sec = (long)(tmpres / 1000000UL);
      tv->tv_usec = (long)(tmpres % 1000000UL);
   }

   if (tz)
   {
      if (!tzflag)
      {
         _tzset();
         tzflag++;
      }
      tz->tz_minuteswest = _timezone / 60;
      tz->tz_dsttime = _daylight;
   }

   return 0;
}

#endif // Q_OS_WIN32
