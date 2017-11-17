//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iothread.hpp
// Date 17.11.17 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _IO_THREAD_H_
#define _IO_THREAD_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <QThread>
#include <QTimer>

#include <common.hpp>
#include <iointerface.hpp>

class LinslotWindow;

//***************************************************************************
// I/O Thread
//***************************************************************************

class IoThread : public QThread, public SlotService, public IoService
{
      Q_OBJECT

   public:

      // object

      IoThread(LinslotWindow* aLinslot);
      virtual ~IoThread();

      // interface

      int init();
      int exit();
      int open();
      int close();
      void stop()                       { running = no; tell(eloDebug, "IO/Thread got stop signal"); }
      void setTestMode(int aFlag)       { testMode = aFlag; }
      int writeBit(int bit, int value)  { return ioDevice->writeBit(bit, value); }
      int readOutBit(int bit)           { return ioDevice->readOutBit(bit); }
      int isOpen()                      { return ioDevice->isOpen(); }
      void changeDevice(const char* dev);
      int waitIo();
      int lookIo();
      void control();
      void activate() { active = yes; }
      byte* getMessage();

      int getGcScale()                  { return ioDevice->getGcScale(); }
      void recordGhostCar(char vBit, char iBit)  { return ioDevice->recordGhostCar(vBit, iBit); }
      void startGhostCar(char pwmBit, char iBit, int profileId);
      void stopGhostCar();
      void ghostCarSync();
      void initIoSetup(word bitsInput, word bitsOutput, byte withSpi)
      { ioDevice->initIoSetup(bitsInput, bitsOutput, withSpi); }

      // get / set

      const char* getDevice()           { return device; }
      void setDevice(const char* dev);

   signals:

      void onDigitalInput(const DigitalEvent &ioEvent);
      void onAnalogInput(const AnalogEvent &ioEvent);

   private slots:

      void onReplayTimer();

   protected:

      struct GcValue
      {
         byte volt;
         byte ampere;
      };

      void run();
      int checkAndOpenConnetion();

      // data

      LinslotWindow* linslot;
      int gcPause;
      QTimer* gcReplayTimer;
      int gcValueIndex;
      QList<GcValue> gcValues;
      char device[100+TB];
      int testMode;
      IoInterface* ioDevice;
      int running;
      byte command;
      int active;
};

//***************************************************************************
#endif // _IO_THREAD_H_
