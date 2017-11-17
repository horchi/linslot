//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iothread.cc
// Date 17.11.17 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <unistd.h>
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QSqlQuery>
#include <QVariant>

#include <iothread.hpp>
#include <common.hpp>
#include <linslot.hpp>
#include <arduino.hpp>

//***************************************************************************
// Class IoThread
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

IoThread::IoThread()
   : QThread()
{
   testMode = no;
   running = no;
   *device = 0;
   command = cNone;
   gcPause = 0;
   active = no;

   gcReplayTimer = new QTimer();
   ioDevice = new Arduino;

   connect(gcReplayTimer, SIGNAL(timeout()), this, SLOT(onReplayTimer()));
}

IoThread::~IoThread()
{
   exit();

   delete ioDevice;
}

//***************************************************************************
// Init
//***************************************************************************

int IoThread::init()
{
   return success;
}

//***************************************************************************
// Set Device
//***************************************************************************

void IoThread::setDevice(const char* dev)
{
   if (!dev)
      return ;

   if (strcmp(device, dev) != 0)
   {
      strncpy(device, dev, 100);
      device[100] = 0;
   }
}

//***************************************************************************
// Set Device
//***************************************************************************

void IoThread::changeDevice(const char* dev)
{
   if (!dev)
      return ;

   if (strcmp(device, dev) != 0)
   {
      setDevice(dev);

      close();
      open();
   }
}

//***************************************************************************
// Exit
//***************************************************************************

int IoThread::exit()
{
   // Close device

   close();

   return success;
}

//***************************************************************************
// Open
//***************************************************************************

int IoThread::open()
{
   close();

   if (ioDevice->open(device) != success)
      return fail;

   ioDevice->flush();
   ioDevice->setWriteTimeout(1000);
   ioDevice->setTimeout(1000);

   emit onDeviceConnected();

   return success;
}

//***************************************************************************
// Close
//***************************************************************************

int IoThread::close()
{
   ioDevice->close();

   return 0;
}

void IoThread::stopGhostCar()
{
   tell(eloAlways, "Stopping ghost car");

   gcReplayTimer->stop();
   usleep(200);
   gcValues.clear();
   gcValueIndex = 0;
   gcPause = 0;

   ioDevice->stopGhostCar();
}

void IoThread::startGhostCar(char pwmBit, char iBit, int profileId)
{
   GcValue value;
   int count = 0;

   gcValueIndex = 0;

   QSqlQuery q("select VOLT, AMPERE from lap_profiles where PROFILE_ID = '" +
               QString::number(profileId) + "' order by LAP_PROFILE_ID;");

   while (q.next())
   {
      gcValues.append(value);

      value.volt = (byte)q.value(0).toInt();
      value.ampere = (byte)q.value(1).toInt();
      count++;
   }

   if (count)
   {
      tell(eloAlways, "Starting ghost car with profile (%d)", profileId);

      ioDevice->startGhostCar(pwmBit, iBit);
      gcPause = 0;
      gcReplayTimer->start(20);
      tell(eloAlways, "added %d values", count);
   }
   else
   {
      tell(eloAlways, "No data for profile %d found", profileId);
   }
}

void IoThread::onReplayTimer()
{
   if (!gcValues.size())
      return ;

   if (gcValueIndex == na)
      return ;

   if (gcPause > 0)
   {
      gcPause--;
      return ;
   }

   byte volt = gcValues.at(gcValueIndex).volt;
   byte ampere = gcValues.at(gcValueIndex).ampere;

   // bei halten des Wetes am Ende der Liste
   // konservativ regeln

   if (gcValueIndex == gcValues.size()-1)
   {
      tell(eloAlways, "'Freeze' last value until sync signal");
      volt /= 2;
      ampere /= 2;

      if (volt < 100)
         volt = 100;
   }

   tell(eloAlways, "[%04d] %d/%d", gcValueIndex, volt, ampere);

   ioDevice->writeGhostCarValue(volt, ampere);

   gcValueIndex++;

   // am letzen Wert halten (bis zum nächsten sync signal)

   if (gcValueIndex >= gcValues.size())
      gcValueIndex = na;
}

//***************************************************************************
//
//***************************************************************************

void IoThread::ghostCarSync()
{
   ioDevice->flushGhostCar();
   gcValueIndex = 0;
   gcPause = 0;

   ioDevice->writeGhostCarValue(gcValues.at(gcValueIndex).volt,
                                gcValues.at(gcValueIndex).ampere);
   gcValueIndex++;

   tell(eloAlways, "Sync gc to lap signal");
}

//***************************************************************************
// At Trouble
//***************************************************************************

int IoThread::checkAndOpenConnetion()
{
   const int retryEvery = 10;
   static int lastTry = 0;

   if (testMode)
      return success;

   if (!ioDevice->isOpen() && time(0) >= lastTry + retryEvery)
   {
      int state = open();

      if (state != success)
         tell(eloAlways, "Retrying to open in %d seconds", retryEvery);

      lastTry = time(0);
   }

   return ioDevice->isOpen() ? success : fail;
}

//***************************************************************************
// Look IO
//***************************************************************************

int IoThread::lookIo()
{
   // needed ... ?

   return done;
}

//***************************************************************************
// Control
//***************************************************************************

byte* IoThread::getMessage()
{
   return ioDevice->getMessage();
}

//***************************************************************************
// Control
//***************************************************************************

void IoThread::control()
{
   char buf[100+TB];

   if (!active)
      return ;

   switch (command)
   {
      case cDigitalIn:
      {
         DigitalEvent event;
         DigitalInput* input;

         if (getMessage())
         {
            input = (DigitalInput*)getMessage();
            event.value = input->value;
            event.tp = addMs2Tv(ioDevice->getBoardStartTime(), input->time);

            tell(eloDebug, "Got digital input (%s)", toBinStr(input->value, buf));

            emit onDigitalInput(event);
         }

         break;
      }
      case cAnalogIn:
      {
         AnalogEvent event;
         AnalogInput* input;

         if (getMessage())
         {
            input = (AnalogInput*)getMessage();
            event.volt = input->volt;
            event.ampere = input->ampere;

            emit onAnalogInput(event);
         }

         break;
      }
      case cDebug:
      {
         DebugValue* debug;

         if (getMessage())
         {
            debug = (DebugValue*)getMessage();

            tell(eloAlways, "<- [%s] (0x%lx)", debug->string, debug->value);
         }

         break;
      }
      case cGhostCarBufferFull:
      {
         gcPause = 50;  // wait 0.5 seconds
         gcValueIndex--;
         tell(eloDetail, "<- buffer full, waiting");

         break;
      }

      default: break;
   }
}

//***************************************************************************
// Run
//***************************************************************************

void IoThread::run()
{
   int status;

   running = yes;

   // loop

   tell(eloDebug, "IO thread started");

   while (running)
   {
      if (checkAndOpenConnetion() != success)
      {
         sleep(1);
         continue ;
      }

      if ((status = ioDevice->look(command)) == success)
         control();
      else
         usleep(100);
   }

   tell(eloDebug, "IO thread ennded");
}
