//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File linslot.cc
// Date 17.1.17 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <sys/time.h>
#  include <alsa.hpp>
#endif

#include <QMessageBox>
#include <QMetaType>
//#include <QSound>
#include <QGraphicsView>
#include <QDialog>
#include <QInputDialog>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSplitter>
#include <QHeaderView>
#include <QToolButton>

#include <linslot.hpp>
#include <lapprofile.hpp>

#include <version.hpp>

//***************************************************************************
// Object
//***************************************************************************

LinslotWindow::LinslotWindow()
   : QMainWindow()
{
   // first init class privates

   testMode = no;
   theEloquence = eloAlways;
   logFile = "";
   withSound = no;
   raceRunning = no;
   countdownStarted = no;
   countdown = 0;
   fastLapTime = 0;
   resourcePath = 0;
   configPath = QString(QDir::homePath() + "/.linslot");
   gcState = gcsOff;
   gcSlot = na;
   visibleImage = imgDriver;
   supressComboBoxUpdate = no;

   QDir d(configPath);

   if (!d.exists())
      d.mkdir(configPath);

   // now setup the user interface

   setupUi(this);
   setWindowTitle("linslot - the LINux SLOTrace manager (" + QString(BUILD) + ")");

   // dialogs

   setupDialog = new SetupDialog(configPath);
   highscoreDialog = new HighscoreDialog();

   connect(setupDialog, SIGNAL(accepted()),
           this, SLOT(onOptionsAccepted()));

   // init

   init();

   // init timer

   timer = new QTimer(this);
   connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

   timerFlash = new QTimer(this);
   connect(timerFlash, SIGNAL(timeout()), this, SLOT(onTimerFlash()));
   timerFlash->start(250);

   timerElapsed = new QTimer(this);
   connect(timerElapsed, SIGNAL(timeout()), this, SLOT(onElapsedTimer()));

   timerPenalty = new QTimer(this);
   connect(timerPenalty, SIGNAL(timeout()), this, SLOT(onPenaltyTimer()));

   timerAnimateImage = new QTimer(this);
   connect(timerAnimateImage, SIGNAL(timeout()), this, SLOT(onAnimateTimer()));

   // thread stuff

   thread = new IoThread();

   // connect thread

   qRegisterMetaType<DigitalEvent>("DigitalEvent");
   qRegisterMetaType<AnalogEvent>("AnalogEvent");

   connect(thread, SIGNAL(onDigitalInput(const DigitalEvent)),
           this, SLOT(onDigitalInput(const DigitalEvent)));

   connect(thread, SIGNAL(onAnalogInput(const AnalogEvent)),
           this, SLOT(onAnalogInput(const AnalogEvent)));

   connect(thread, SIGNAL(onDeviceConnected(const int)),
           this, SLOT(onDeviceConnected(const int)));

   // start io thread

   thread->init();
   thread->setDevice(setupDialog->getUsbDevice());
   thread->start();
   thread->setTestMode(testMode);

   setOutputs(isAllOff);

   // apply setup options

   applyOptions();
}

LinslotWindow::~LinslotWindow()
{
   exit();

   delete highscoreDialog;
   delete setupDialog;
   delete db;
   delete thread;

   delete timer;
   delete timerElapsed;
   delete timerPenalty;
   delete timerAnimateImage;

   if (resourcePath)
      free(resourcePath);
}

//***************************************************************************
// Init
//***************************************************************************

void LinslotWindow::init()
{
   int trySound = yes;

   resourcePath = strdup(setupDialog->getResourcePath());

   // widged stuff

   ledsParent = labelLed00->parent();           // !!

   // output states

   for (int i = 0; i < fctOutputCount; i++)
      outputFunctionState[i] = 0;

   // parse command line arguments

   for (int i = 0; i < QCoreApplication::arguments().size(); i++)
   {
      char arg[1000];

      strcpy(arg, QCoreApplication::arguments().at(i).toLatin1().constData());

      if (arg[0] != '-' || qstrlen(arg) != 2)
         continue;

      switch (arg[1])
      {
         case 'h':
         {
            printf("Usage: linslit [options]\n");
            printf("       -h         this help\n");
            printf("       -s         no sound\n");
            printf("       -f <file>  log to file\n");
            printf("       -e <n>     eloquence (log level)\n");
            printf("       -t         test mode\n");

            ::exit(0);
         }
         case 't':
         {
            testMode = yes;
            break;
         }
         case 'f':
         {
            i++;
            logFile = QCoreApplication::arguments().at(i);
            break;
         }
         case 'e':
         {
            i++;
            theEloquence = QCoreApplication::arguments().at(i).toInt();
            tell(eloAlways, "Set eloquence to (%d)", theEloquence);
            break;
         }
         case 's':
         {
            trySound = no;
            break;
         }
      }
   }

   // init slot data

   for (int i = 0; i < slotCount; i++)
   {
      theSlots[i].lap = -1;
      theSlots[i].penalty = 0;
      *theSlots[i].driver = 0;
      *theSlots[i].car = 0;
      theSlots[i].fueling = no;
      theSlots[i].gcProfile = na;

      theSlots[i].progressBarFuel = i == 0 ?  progressBarFuelSlot1 : progressBarFuelSlot2;
      theSlots[i].labelLapCount = i == 0 ?  labelLapCounter1 : labelLapCounter2;

      theSlots[i].labelImage = i == 0 ?  labelImageSlot1 : labelImageSlot2;
      theSlots[i].labelElapsedLap = i == 0 ? labelLapTimeSlot1 : labelLapTimeSlot2;
      theSlots[i].labelFastLap = i == 0 ? labelFastLapSlot1 : labelFastLapSlot2;
      theSlots[i].labelLastLap = i == 0 ? labelLastLapSlot1 : labelLastLapSlot2;
      theSlots[i].labelInfo = i == 0 ? labelInfoSlot1 : labelInfoSlot2;

      theSlots[i].comboDriver = i == 0 ? comboBoxDriver1 : comboBoxDriver2;
      theSlots[i].comboCar = i == 0 ? comboBoxCar1 : comboBoxCar2;

      theSlots[i].tableWidget = i == 0 ? tableWidgetSlot1 : tableWidgetSlot2;
      theSlots[i].tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // QT5
      // theSlots[i].tableWidget->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch); // QT4
      theSlots[i].tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
      theSlots[i].tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

      theSlots[i].fuelTimer = new QTimer(this);
      connect(theSlots[i].fuelTimer, SIGNAL(timeout()), this,
              i == 0 ? SLOT(onFuelTimerSlot1()) : SLOT(onFuelTimerSlot2()));
   }

   // db

   tell(eloDebug, "open db");

   if (openDb() != success)
      pushButtonHallOfFame->setEnabled(no);

   tell(eloDebug, "open db done");

   // check sound

   if (trySound)
      checkSound();
}

//***************************************************************************
// Exit
//***************************************************************************

void LinslotWindow::exit()
{
   tell(eloDebug, "Exit!");

   if (db) db->close();

   storeConfig();

   // stop thread

   tell(eloAlways, "Try to stop thread");
   thread->stop();

   if (!thread->wait(2000))
      tell(eloAlways, "Thread would not end!");
   else
      tell(eloAlways, "Thread ended regularly");
}

//***************************************************************************
// IO Opened
//***************************************************************************

void LinslotWindow::onDeviceConnected(int state)
{
   tell(eloAlways, "Got connection state (%d)", state);

   if (state)
   {
      thread->deactivate();
      labelLedConnected->setPixmap(QPixmap(QString(resourcePath) + "/pixmap/led-green-on.gif"));

      thread->initIoSetup(setupDialog->getInputMask(),
                          setupDialog->getOutputMask(),
                          setupDialog->getWithSpiExtension());

      // switch slot power off

      setOutputs(isAllOff);
      clearSlotPower();
      writeBit(bitPenaltyIndSlot1, off);
      writeBit(bitPenaltyIndSlot2, off);
      writeBit(bitPenaltyIndSlot3, off);
      writeBit(bitPenaltyIndSlot4, off);
      switchOutputs(isNoPowerInd, on);

      sleep(1);
      thread->flush();
      thread->activate();
   }
   else
   {
      thread->deactivate();
      labelLedConnected->setPixmap(QPixmap(QString(resourcePath) + "/pixmap/led-green-off.gif"));
   }

   pushButtonStartRace->setEnabled(state || testMode);
   pushButtonPower->setEnabled(state || testMode);
   toolButtonRecordGhostCar->setEnabled(state || testMode);
}

//***************************************************************************
// Store Config
//***************************************************************************

void LinslotWindow::storeConfig()
{
   // save config

   setupDialog->getSettings()->beginGroup("common");

   setupDialog->getSettings()->setValue("driver0", theSlots[0].driver);
   setupDialog->getSettings()->setValue("driver1", theSlots[1].driver);
   setupDialog->getSettings()->setValue("car0", theSlots[0].car);
   setupDialog->getSettings()->setValue("car1", theSlots[1].car);

   setupDialog->getSettings()->endGroup();
}

//***************************************************************************
// Reset Widgets
//***************************************************************************

void LinslotWindow::resetWidgets()
{
   QStringList header;
   header << "Zeit" << "km/h";

   frameTestMode->setVisible(testMode);  // #TODO, speciam test mode witch connection and buttons
   labelInfo->setText("Standby");
   labelFastLap->setText("");

   for (int i = 0; i < slotCount; i++)
   {
      theSlots[i].labelInfo->setText("");
      theSlots[i].labelFastLap->setText("");
      theSlots[i].labelLastLap->setText("");
      theSlots[i].labelElapsedLap->setText("0.0");

      theSlots[i].tableWidget->setEnabled(no);
      theSlots[i].tableWidget->clear();
      theSlots[i].tableWidget->setEnabled(yes);

      theSlots[i].tableWidget->setHorizontalHeaderLabels(header);
      theSlots[i].tableWidget->setRowCount(0);

      if (radioButtonLapRace->isChecked())
         theSlots[i].labelLapCount->setText(toStr(setupDialog->getLapCountRace()));
      else
         theSlots[i].labelLapCount->setText("0");

      if (setupDialog->getFuelingActive())
      {
         theSlots[i].progressBarFuel->setMaximum(QVariant(setupDialog->getFuelMax()).toInt());
         theSlots[i].progressBarFuel->setValue(QVariant(setupDialog->getFuelMax()).toInt());
      }

      theSlots[i].progressBarFuel->setVisible(setupDialog->getFuelingActive());
   }

   if (radioButtonLapRace->isChecked())
      labelLaps->setText(QString::number(setupDialog->getLapCountRace()) + "/"
                         + QString::number(setupDialog->getLapCountRace()));
   else
      labelLaps->setText("0/" + QString::number(setupDialog->getLapCountTraining()));

   labelFirst->setText("1");
   labelFirstTime->setText("");
   labelSecond->setText("2");
   labelSecondTime->setText("");

   if (thread->isOpen())
      labelLedConnected->setPixmap(QPixmap(QString(resourcePath) + "/pixmap/led-green-on.gif"));
   else if (testMode)
      labelLedConnected->setPixmap(QPixmap(QString(resourcePath) + "/pixmap/led-yellow-on.gif"));
   else
      labelLedConnected->setPixmap(QPixmap(QString(resourcePath) + "/pixmap/led-green-off.gif"));

   int powerOn = thread->isOpen() || testMode;
   setSlotPower(psAll, powerOn);
   switchOutputs(isNoPowerInd, !powerOn);

   updateDriverImage(labelImageSlot1->width(), labelImageSlot1->height());
   pushButtonStartRace->setEnabled(thread->isOpen() || testMode);
   pushButtonPower->setEnabled(thread->isOpen() || testMode);
   pushButtonSaveToDb->setEnabled(no);
   toolButtonRecordGhostCar->setEnabled(no);
}

//***************************************************************************
// Get Images For
//***************************************************************************

int LinslotWindow::getImagesFor(int led, const char* &ledOn, const char* &ledOff)
{
   switch (led)
   {
      case ledRed1:
      case ledRed2:
      case ledRed3:
      case ledRed4:
      case ledRed5:
      {
         ledOn  = "/pixmap/led-red-on.gif";
         ledOff = "/pixmap/led-red-off.gif";
         break;
      }
      case ledGreen:
      {
         ledOn  = "/pixmap/led-green-on.gif";
         ledOff = "/pixmap/led-green-off.gif";
         break;
      }
      case ledYellow1:
      case ledYellow2:
      case ledYellow3:
      case ledYellow4:
      {
         ledOn  = "/pixmap/led-yellow-on.gif";
         ledOff = "/pixmap/led-yellow-off.gif";
         break;
      }
      default: return fail;
   }

   return success;
}

//***************************************************************************
// Set Led
//***************************************************************************

void LinslotWindow::setLed(int function, int state)
{
   const char* ledOn  = "";
   const char* ledOff = "";

   if (outputBits[function].ledid <= 0)
      return ;

   if (getImagesFor(outputBits[function].ledid, ledOn, ledOff) != success)
      return ;

   for (int i = 0; i < ledsParent->children().size(); i++)
   {
      QLabel* label = qobject_cast<QLabel*>(ledsParent->children().at(i));

      if (label && label->indent() == outputBits[function].ledid)
      {
         if (state)
            label->setPixmap(QPixmap(QString(resourcePath) + ledOn));
         else
            label->setPixmap(QPixmap(QString(resourcePath) + ledOff));
      }
   }
}

//***************************************************************************
// Set Leds
//***************************************************************************

void LinslotWindow::setOutputs(int mask)
{
   writeBit(bitLightGreen, mask & isGreen);
   writeBit(bitLightRed1, mask & isRed1);
   writeBit(bitLightRed2, mask & isRed2);
   writeBit(bitLightRed3, mask & isRed3);
   writeBit(bitLightRed4, mask & isRed4);
   writeBit(bitLightRed5, mask & isRed5);

   writeBit(bitPowerIndSlot1, mask & isNoPowerIndSlot1);
   writeBit(bitPowerIndSlot2, mask & isNoPowerIndSlot2);
   writeBit(bitPowerIndSlot3, mask & isNoPowerIndSlot3);
   writeBit(bitPowerIndSlot4, mask & isNoPowerIndSlot4);

   writeBit(bitFuelingIndSlot1, mask & isFuelingIndSlot1);
   writeBit(bitFuelingIndSlot2, mask & isFuelingIndSlot2);
   writeBit(bitFuelingIndSlot3, mask & isFuelingIndSlot3);
   writeBit(bitFuelingIndSlot4, mask & isFuelingIndSlot4);

   writeBit(bitPenaltyIndSlot1, mask & isPenaltyIndSlot1);
   writeBit(bitPenaltyIndSlot2, mask & isPenaltyIndSlot2);
   writeBit(bitPenaltyIndSlot3, mask & isPenaltyIndSlot3);
   writeBit(bitPenaltyIndSlot4, mask & isPenaltyIndSlot4);
}

//***************************************************************************
// Switch Leds
//***************************************************************************

void LinslotWindow::switchOutputs(int mask, int state)
{
   if (mask & isGreen)
      writeBit(bitLightGreen, state);
   if (mask & isRed1)
      writeBit(bitLightRed1, state);
   if (mask & isRed2)
      writeBit(bitLightRed2, state);
   if (mask & isRed3)
      writeBit(bitLightRed3, state);
   if (mask & isRed4)
      writeBit(bitLightRed4, state);
   if (mask & isRed5)
      writeBit(bitLightRed5, state);

   if (mask & isNoPowerIndSlot1)
      writeBit(bitPowerIndSlot1, state);
   if (mask & isNoPowerIndSlot2)
      writeBit(bitPowerIndSlot2, state);
   if (mask & isNoPowerIndSlot3)
      writeBit(bitPowerIndSlot3, state);
   if (mask & isNoPowerIndSlot4)
      writeBit(bitPowerIndSlot4, state);

   if (mask & isFuelingIndSlot1)
      writeBit(bitFuelingIndSlot1, state);
   if (mask & isFuelingIndSlot2)
      writeBit(bitFuelingIndSlot2, state);
   if (mask & isFuelingIndSlot3)
      writeBit(bitFuelingIndSlot3, state);
   if (mask & isFuelingIndSlot4)
      writeBit(bitFuelingIndSlot4, state);

   if (mask & isPenaltyIndSlot1)
      writeBit(bitPenaltyIndSlot1, state);
   if (mask & isPenaltyIndSlot2)
      writeBit(bitPenaltyIndSlot2, state);
   if (mask & isPenaltyIndSlot3)
      writeBit(bitPenaltyIndSlot3, state);
   if (mask & isPenaltyIndSlot4)
      writeBit(bitPenaltyIndSlot4, state);
}

//***************************************************************************
// Write Bit
//***************************************************************************

void LinslotWindow::writeBit(int function, int state)
{
   tell(eloDebug, "Debug: Write bit %d of function %d to %d", outputBitOf(function), function, state);

   outputFunctionState[function] = state;
   thread->writeBit(outputBitOf(function), state);
   setLed(function, state);
}

//***************************************************************************
//
//***************************************************************************

void LinslotWindow::setPenalty(int slot)
{
   setSlotPower(slot+1, off);

   if (slot == 0)
      switchOutputs(isPenaltyIndSlot12, on);
   else
      switchOutputs(isPenaltyIndSlot22, on);
}

void LinslotWindow::clearPenalty(int slot)
{
   setSlotPower(slot+1, on);

   if (slot == 0)
      switchOutputs(isPenaltyIndSlot12, off);
   else
      switchOutputs(isPenaltyIndSlot22, off);
}

void LinslotWindow::setSlotPower()
{
   setSlotPower(psAll, on);
   switchOutputs(isNoPowerInd, off);
}

void LinslotWindow::clearSlotPower()
{
   setSlotPower(psAll, off);
   switchOutputs(isNoPowerInd, on);
}

//***************************************************************************
// Set Slot Power
//***************************************************************************

void LinslotWindow::setSlotPower(int slot, int flag)
{
   char buf[4+TB];
   tell(eloDetail, "Set power of slots (%s) to %d", toBinStr(slot, buf, 4), flag);

   if (slot & psSlot1)
      writeBit(bitPowerSlot1, flag);

   if (slot & psSlot2)
      writeBit(bitPowerSlot2, flag);

   if (slot & psSlot1 && slot & psSlot2)
      pushButtonPower->setIcon(QIcon(QString(resourcePath)
                                     + (flag ? "pixmap/led-blue-on.gif" : "pixmap/led-blue-off.gif")));
}

//***************************************************************************
// Play Sound
//***************************************************************************

void LinslotWindow::playSound(int fct)
{
   if (sounds[fct].sound != "na")
      ::playSound(QString(resourcePath) + "/sound/" + sounds[fct].sound);
}

//***************************************************************************
// On Options Accepted
//***************************************************************************

void LinslotWindow::onOptionsAccepted()
{
   if (oldSpiSetting != setupDialog->getWithSpiExtension())
      QMessageBox::information(0, "Adruino", "Nach ändern der SPI Einstellung muss der "
                               "Arduino neu gestartet werden!");

   tell(eloAlways, "Reconnect to apply option to arduino");

   thread->close();
   applyOptions();
   thread->setDevice(setupDialog->getUsbDevice());
   thread->open();
}

void LinslotWindow::applyOptions()
{
   QListWidgetItem* listItem;

   free(resourcePath);
   resourcePath = strdup(setupDialog->getResourcePath());

#ifndef Q_OS_WIN32

   if (withSound && setupDialog->getAlsaDevice() != QAlsaSound::getDeviceName())
   {
      QString oldDevice = QAlsaSound::getDeviceName();

      QAlsaSound::setDeviceName(setupDialog->getAlsaDevice());

      if (!QAlsaSound::isAvailable())
         QAlsaSound::setDeviceName(oldDevice);

   }

#endif

   // for speedup, don't load image during fill

   supressComboBoxUpdate = yes;

   if (setupDialog->hasDriverChanged() || setupDialog->hasCarChanged())
   {
      for (int i = 0; i < slotCount; i++)
      {
         theSlots[i].fuelLevel = setupDialog->getFuelMax();
         theSlots[i].comboDriver->clear();
         theSlots[i].comboCar->clear();

         for (int n = 0; n < setupDialog->listWidgetDriver->count(); n++)
         {
            if ((listItem = setupDialog->listWidgetDriver->item(n)))
               theSlots[i].comboDriver->addItem(listItem->text());
         }

         for (int n = 0; n < setupDialog->listWidgetCar->count(); n++)
         {
            if ((listItem = setupDialog->listWidgetCar->item(n)))
               theSlots[i].comboCar->addItem(listItem->text());
         }
      }
   }

   /*
   QSqlQuery query("select NAME from profiles where ACTIVE = 'true';");

   while (query.next())
      comboBoxDriver1->addItem("GC: " + query.value(0).toString());
   */

   setupDialog->getSettings()->beginGroup("common");

   int i;

   if ((i = comboBoxDriver1->findText(setupDialog->getSettings()->value("driver0", "").toString())) >= 0)
      comboBoxDriver1->setCurrentIndex(i);

   if ((i = comboBoxDriver2->findText(setupDialog->getSettings()->value("driver1", "").toString())) >= 0)
      comboBoxDriver2->setCurrentIndex(i);

   if ((i = comboBoxCar1->findText(setupDialog->getSettings()->value("car0", "").toString())) >= 0)
      comboBoxCar1->setCurrentIndex(i);

   if ((i = comboBoxCar2->findText(setupDialog->getSettings()->value("car1", "").toString())) >= 0)
      comboBoxCar2->setCurrentIndex(i);

   setupDialog->getSettings()->endGroup();

   supressComboBoxUpdate = no;

   // TODO to be removed (driver and car should replaced with getDriver(), getCar() calles)

   strncpy(theSlots[0].driver, theSlots[0].getDriver().toLatin1(), sizeName);
   strncpy(theSlots[1].driver, theSlots[1].getDriver().toLatin1(), sizeName);
   strncpy(theSlots[0].car, theSlots[0].getCar().toLatin1(), sizeName);
   strncpy(theSlots[1].car, theSlots[1].getCar().toLatin1(), sizeName);

   // Image animation mode

   timerAnimateImage->stop();

   if (setupDialog->getDriverImageMode() == SetupDialog::mdAnimated)
      timerAnimateImage->start(setupDialog->getAnimationInterval() * 1000);
   else if (setupDialog->getDriverImageMode() == SetupDialog::mdDriver)
      visibleImage = imgDriver;
   else
      visibleImage = imgCar;

   updateDriverImage(labelImageSlot1->width(),
                     labelImageSlot1->height());

   resetWidgets();
}

//***************************************************************************
// On Driver1 Index Changed
//***************************************************************************

void LinslotWindow::on_comboBoxDriver1_currentIndexChanged(QString value)
{
   if (!supressComboBoxUpdate)
   {
      strncpy(theSlots[0].driver, value.toLatin1(), sizeName);

      updateDriverImage(labelImageSlot1->width(),
                        labelImageSlot1->height());
   }
}

//***************************************************************************
// On Driver2 Index Changed
//***************************************************************************

void LinslotWindow::on_comboBoxDriver2_currentIndexChanged(QString value)
{
   if (!supressComboBoxUpdate)
   {
      strncpy(theSlots[1].driver, value.toLatin1(), sizeName);

      updateDriverImage(labelImageSlot1->width(),
                        labelImageSlot1->height());
   }
}

//***************************************************************************
// On Car1 Index Changed
//***************************************************************************

void LinslotWindow::on_comboBoxCar1_currentIndexChanged(QString value)
{
   if (!supressComboBoxUpdate)
   {
      strncpy(theSlots[0].car, value.toLatin1(), sizeName);

      updateDriverImage(labelImageSlot1->width(),
                        labelImageSlot1->height());
   }
}

//***************************************************************************
// On Car2 Index Changed
//***************************************************************************

void LinslotWindow::on_comboBoxCar2_currentIndexChanged(QString value)
{
   if (!supressComboBoxUpdate)
   {
      strncpy(theSlots[1].car, value.toLatin1(), sizeName);

      updateDriverImage(labelImageSlot1->width(),
                        labelImageSlot1->height());
   }
}

//***************************************************************************
// On Penalty Timer
//***************************************************************************

void LinslotWindow::onPenaltyTimer()
{
   int n = 0;

   tell(eloDebug2, "onPenaltyTimer");

   for (int i = 0; i < slotCount; i++)
   {
      if (theSlots[i].penalty > 0)
      {
         theSlots[i].penalty--;

         if (theSlots[i].penalty <= 0)
         {
            clearPenalty(i);
            theSlots[i].labelInfo->setText("");
         }
         else
            n += theSlots[i].penalty;
      }
   }

   if (n <= 0)
      timerPenalty->stop();
}

//***************************************************************************
// On Elapsed Timer
//  - race/trainging 'time limit' reached
//***************************************************************************

void LinslotWindow::onElapsedTimer()
{
   timeval tp;
   int finished = no;

   gettimeofday(&tp, 0);

   int usec = elapsed(&raceStart, &tp);

   labelElapsed->setText(QString::number(usec/1000000.0, 'f', 1));

   for (int i = 0; i < slotCount; i++)
   {
      int usecLap = elapsed(&theSlots[i].lastSignal, &tp);
      theSlots[i].labelElapsedLap->setText(QString::number(usecLap/1000000.0, 'f', 1));
   }

   if (radioButtonLapRace->isChecked() && setupDialog->getMaxTimeLapRace())
      finished = usec/1000000 >= setupDialog->getMaxTimeLapRace();
   else if (radioButtonTraining->isChecked() && setupDialog->getMaxTimeTraining())
      finished = usec/1000000 >= setupDialog->getMaxTimeTraining();

   if (finished)
   {
      int s = theSlots[0].lap == theSlots[1].lap ? -1 :
         theSlots[0].lap > theSlots[1].lap ? 0 : 1;

      tell(eloDetail, "Finished after %d seconds of race/training", usec/1000000);
      atFinish(!radioButtonTraining->isChecked() ? s : -1);
   }
}

//***************************************************************************
// On Timer
//***************************************************************************

void LinslotWindow::onTimer()
{
   if (countdown == 5)
   {
      switchOutputs(isPhase5, off);
      switchOutputs(isGreen, on);
      atStart();
   }
   else
   {
      playSound(sfCountdownPhase);

      switch (countdown)
      {
         case 0: switchOutputs(isPhase1, on); break;
         case 1: switchOutputs(isPhase2, on); break;
         case 2: switchOutputs(isPhase3, on); break;
         case 3: switchOutputs(isPhase4, on); break;
         case 4: switchOutputs(isPhase5, on); break;
      }

      countdown++;
   }
}

//***************************************************************************
// On Flash Timer
//***************************************************************************

void LinslotWindow::onTimerFlash()
{
   static int upFast = yes;
   static int upSlow = yes;
   static int count = 0;
   int value;

   if (count%2) upSlow = !upSlow;
   upFast = !upFast;

   count++;

   for (int i = 0; i < fctOutputCount; i++)
   {
      if (outputFunctionState[i])
      {
         value = na;

         switch (outputBits[i].mode)
         {
            case omFlashUpFast:   value = !upFast;              break;
            case omFlashDownFast: value = upFast;               break;
            case omFlashUpSlow:   if (count%2) value = !upSlow; break;
            case omFlashDownSlow: if (count%2) value = upSlow;  break;
            default: value = na;
         }

         if (value != na)
         {
            thread->writeBit(outputBitOf(i), value);
            setLed(i, value);
         }
      }
   }
}

//***************************************************************************
// On Analog Input
//***************************************************************************

void LinslotWindow::onAnalogInput(const AnalogEvent ioEvent)
{
   byte volt = ioEvent.volt;

   if (setupDialog->getGhostcarInvert())
      volt = 255 - volt;

   int u = (int)((double)volt / 255.0 * 100.0);
   int i = (int)((double)ioEvent.ampere / 255.0 * 100.0);

   if (gcState == gcsRecording)
   {
      labelFastLap->setText(QString::number(u) + "%");

      tell(eloAlways, "-> U = %d%%; I = %d%% [%d/%d]", u, i, volt, ioEvent.ampere);
      gcValues.append(volt | (ioEvent.ampere << 8));
   }
}

//***************************************************************************
// On I/O Signal
//***************************************************************************

void LinslotWindow::onDigitalInput(const DigitalEvent ioEvent)
{
   DigitalEvent theEvent = ioEvent;
   unsigned int value;

   if (!(value = getChanges(theEvent)))
      return ;

   char buf[100+TB];
   tell(eloDebug, "Debug: Changes detected (%s)", toBinStr(value, buf));

   // externer Start/Stop Taster (nicht im test mode) ?

   if (isBit(value, bitStartKey))
   {
      if (!countdownStarted)
      {
         if (radioButtonLapRace->isChecked())
            atStartCountdown();
         else
            atStartTraining();
      }
      else
         atAbort("Abbruch");
   }

   // process lap signals

   if (isBit(value, bitIrSlot1))
      atSlotSignal(0, &theEvent.tp);

   if (isBit(value, bitIrSlot2))
      atSlotSignal(1, &theEvent.tp);

   // process fueling signals

   if (isBit(value, bitFuelStartSlot1))
      atFuelSignal(0, &theEvent.tp, yes);

   if (isBit(value, bitFuelStartSlot2))
      atFuelSignal(1, &theEvent.tp, yes);

   if (isBit(value, bitFuelEndSlot1))
      atFuelSignal(0, &theEvent.tp, no);

   if (isBit(value, bitFuelEndSlot2))
      atFuelSignal(1, &theEvent.tp, no);
}

//***************************************************************************
// On Button Power
//***************************************************************************

void LinslotWindow::on_pushButtonPower_clicked()
{
   if (!outputFunctionState[bitPowerSlot1])
      setSlotPower();
   else
      clearSlotPower();
}

//***************************************************************************
// On Button Start
//***************************************************************************

void LinslotWindow::on_pushButtonStartRace_clicked()
{
   if (!countdownStarted)
   {
      if (radioButtonLapRace->isChecked())
         atStartCountdown();
      else
         atStartTraining();
   }
   else
      atAbort("Abbruch");
}

//***************************************************************************
// On Button Options
//***************************************************************************

void LinslotWindow::on_pushButtonOptions_clicked()
{
   storeConfig();
   oldSpiSetting = setupDialog->getWithSpiExtension();
   setupDialog->reset();
   setupDialog->show();
}

//***************************************************************************
// On Button Hall Of Fame
//***************************************************************************

void LinslotWindow::on_pushButtonHallOfFame_clicked()
{
   if (!db)
      return ;

   highscoreDialog->setDb(db);
   highscoreDialog->fill();
   highscoreDialog->show();
}

//***************************************************************************
// On Button Save To Db
//***************************************************************************

void LinslotWindow::on_pushButtonSaveToDb_clicked()
{
   saveRace();
   pushButtonSaveToDb->setEnabled(no);
}

//***************************************************************************
// At Start
//***************************************************************************

void LinslotWindow::atStartCountdown()
{
   initRace();

   labelInfo->setText("Countdown");
   timer->start(1000);
}

//***************************************************************************
// Start Training
//***************************************************************************

void LinslotWindow::atStartTraining()
{
   initRace();

   switchOutputs(isPhase5, off);
   switchOutputs(isGreen, on);

   atStart();
}

//***************************************************************************
//
//***************************************************************************

void LinslotWindow::initRace()
{
   countdownStarted = yes;
   countdown = 0;

   // init slot data

   for (int i = 0; i < slotCount; i++)
   {
      theSlots[i].penalty = 0;
      theSlots[i].lap = -1;
      theSlots[i].fastLapTime = 0;
      theSlots[i].fueling = no;
      theSlots[i].fuelLevel = setupDialog->getFuelMax();
   }

   // power on

   setSlotPower();

   // set/reset widget stuff

   radioButtonTraining->setEnabled(radioButtonTraining->isChecked());
   radioButtonLapRace->setEnabled(radioButtonLapRace->isChecked());
   pushButtonOptions->setEnabled(no);

   resetWidgets();
   pushButtonStartRace->setText("&Abbrechen");
   labelElapsed->setText("0.0");
}

//***************************************************************************
// At Start
//***************************************************************************

void LinslotWindow::atStart()
{
   // init

   countdown = 0;
   fastLapTime = 0;
   raceRunning = yes;
   gettimeofday(&raceStart, 0);
   timer->stop();

   if (QString(theSlots[0].driver).indexOf("GC: ") == 0)
   {
      QSqlQuery query("select PROFILE_ID from profiles where NAME = '"
                      + QString(theSlots[0].driver[4]) + "';");

      if (query.next())
         theSlots[0].gcProfile = query.value(0).toInt();
      else
      {
         tell(eloAlways, "Fatal: Profile for '%s' not found", theSlots[0].driver);
         return ;
      }

      tell(eloDebug, "Starting ghost car '%s' profile (%d)",
           theSlots[0].driver, theSlots[0].gcProfile);

      thread->startGhostCar(outputBits[bitPwmOutSlot1].bit,
                            analogBits[fctGhostISlot1].bit,
                            theSlots[0].gcProfile);

      gcState = gcsRunning;
      gcSlot = 0;
   }

   timerElapsed->start(100);          // 0.1 sec
   timerPenalty->start(1000);         // 1.0 sec

   for (int i = 0; i < slotCount; i++)
      theSlots[i].lastSignal = raceStart;

   if (radioButtonLapRace->isChecked())
      labelInfo->setText("Rennen läuft");
   else
      labelInfo->setText("Freies Training");

   playSound(sfRaceStart);
}

//***************************************************************************
// At Finish
//***************************************************************************

void LinslotWindow::atFinish(int slot)
{
   timeval tp;
   int usec;
   char buf[100];

   gettimeofday(&tp, 0);

   if (slot >= 0)
      sprintf(buf, "Finish\n Sieger %s", theSlots[slot].driver);
   else
      sprintf(buf, "Finish");

   atStop(buf);

   playSound(sfRaceFinished);

   usec = elapsed(&raceStart, &tp);

   tell(eloDebug, "Total: %2.2d:%2.2d.%06d",
        usec/1000000/60, usec/1000000%60, usec%1000000);

   for (int i = 0; i < slotCount; i++)
      tell(eloDebug, "Bahn %d - %d Runden gefahren", i+1, theSlots[i].lap);

   pushButtonSaveToDb->setEnabled(yes);
}

//***************************************************************************
// At Abort
//***************************************************************************

void LinslotWindow::atAbort(const char* info)
{
   atStop(info);
   labelElapsed->setText("0.0");

   // playSound(sfRaceAborted);
}

//***************************************************************************
// At Stop
//***************************************************************************

void LinslotWindow::atStop(const char* info)
{
   timeval tp;
   int usec;

   raceRunning = no;
   countdownStarted = no;
   timer->stop();
   timerElapsed->stop();

   // slot power off

   setOutputs(isAllOff | isNoPowerInd);
   clearSlotPower();

   if (gcState == gcsRunning)
   {
      thread->stopGhostCar();
      tell(eloDebug, "Stopped chost car");
      gcState = gcsOff;
      gcSlot = na;
   }

   // update widgets

   gettimeofday(&tp, 0);
   usec = elapsed(&raceStart, &tp);
   labelElapsed->setText(QString::number(usec/1000000.0, 'f', 1));

   pushButtonStartRace->setText("St&arten");

   labelInfo->setText(info);

   radioButtonTraining->setEnabled(yes);
   radioButtonLapRace->setEnabled(yes);
   pushButtonOptions->setEnabled(yes);
}

//***************************************************************************
// At Jump The Gun
//***************************************************************************

void LinslotWindow::atJumpStart(int slot)
{
   char info[100];

   if (setupDialog->getAbortAtJumpTheGun())
   {
      sprintf(info, "Abbruch Frühstart '%s'", theSlots[slot].driver);
      atAbort(info);
   }
   else
   {
      playSound(sfJumpTheGun);
      theSlots[slot].labelInfo->setText("Frühstart");

      tell(eloAlways, "Jump start on slot %d, time penalty of (%d) seconds",
           slot+1, setupDialog->getPenaltyAtJumpTheGun());

      setPenalty(slot);
      theSlots[slot].penalty += setupDialog->getPenaltyAtJumpTheGun();
   }
}

//***************************************************************************
// At No Fuel
//***************************************************************************

void LinslotWindow::atNoFuel(int slot)
{
   tell(eloAlways, "No fuel on slot %d, time penalty of (%d) seconds",
        slot+1, setupDialog->getFuelPenaltyTime());

   playSound(sfFuelEmpty);
   setPenalty(slot);
   theSlots[slot].penalty += setupDialog->getFuelPenaltyTime();
   timerPenalty->start(1000);         // 1.0 sec
}

//***************************************************************************
//
//***************************************************************************

int LinslotWindow::isActiveEdge(int function, int state)
{
   if (state && inputBits[function].mode == teRising)
      return yes;
   else if (!state && inputBits[function].mode == teFalling)
      return yes;

   return no;
}

int LinslotWindow::inputBitOf(int function)
{
   return inputBits[function].bit;
}

int LinslotWindow::functionOfInput(int bit)
{
   for (int i = 0; i < bitInputCount; i++)
      if (inputBits[i].bit == bit)
         return i;

   return na;
}

int LinslotWindow::outputBitOf(int function)
{
   return outputBits[function].bit;
}

//***************************************************************************
// Bounce Check (entprellen)
//***************************************************************************

unsigned int LinslotWindow::getChanges(DigitalEvent& ioEvent)
{
   static int initialized = no;
   static timeval lastInputTimes[4*bitsPerByte] = { { 0, 0 } };
   static unsigned int lastValue = 0xFFFFFFFF;

   // init

   if (!initialized)
   {
      initialized = yes;
      lastValue = 0xFFFFFFFF;

      for (int i = 0; i < 4*bitsPerByte; i++)
      {
         gettimeofday(&lastInputTimes[i], 0);
         lastInputTimes[i].tv_sec -= 10;
      }
   }

   // detect changes

   unsigned int mask, b, state, fctMask;
   unsigned int changedFunctions = 0;

   for (int bit = 0; bit < 4*bitsPerByte; bit++)
   {
      mask = 1 << bit;
      state = (ioEvent.value & mask) ? 1 : 0;
      b =  state << bit;

      if (((lastValue & mask) ^ b)
          && (elapsed(&lastInputTimes[bit], &ioEvent.tp) > bounceTime))
      {
         // bit changed -and- last change older than 'bounceTime'

         tell(eloDebug2, "Bit %d toggled to %d", bit, state);

         for (int fct = 0; fct < bitInputCount; fct++)
         {
            fctMask = 1 << fct;

            if (inputBits[fct].bit == bit)
            {
               if (isActiveEdge(fct, state))
               {
                  tell(eloDebug, "Debug: Function '%s' (%d), bit (%d) detected", inputFunctionName(fct), fct, bit);
                  changedFunctions |= fctMask;
                  lastInputTimes[bit] = ioEvent.tp;
               }
            }
         }
      }
   }

   lastValue = ioEvent.value;

   return changedFunctions;
}

//***************************************************************************
// Fueling Timer
//***************************************************************************

void LinslotWindow::onFuelTimerSlot1()
{
   atFuelTimer(0);
}

void LinslotWindow::onFuelTimerSlot2()
{
   atFuelTimer(1);
}

void LinslotWindow::atFuelTimer(int slot)
{
   theSlots[slot].fuelTimer->start(100);  // fuel trigger each 0.1 second

   if (!theSlots[slot].fueling)
   {
      theSlots[slot].fueling = yes;
      switchOutputs(!slot ? isFuelingIndSlot12 : isFuelingIndSlot22, on);

      tell(eloAlways, "Start fueling slot (%d)", slot);
      playSound(sfFuelingStart);
   }
   else
   {
      theSlots[slot].fuelLevel += setupDialog->getFuelingPerSecond() / 10.0;
      tell(eloAlways, "Added %.2f liter fuel for slot (%d)",
           setupDialog->getFuelingPerSecond() / 10.0, slot);

      if (theSlots[slot].fuelLevel > setupDialog->getFuelMax())
      {
         theSlots[slot].fuelLevel = setupDialog->getFuelMax();
         theSlots[slot].fueling = no;
         theSlots[slot].fuelTimer->stop();
         switchOutputs(!slot ? isFuelingIndSlot12 : isFuelingIndSlot22, off);

         playSound(sfFuelingFinished);
         tell(eloAlways, "Finished fueling slot (%d)", slot);
      }

      theSlots[slot].progressBarFuel->setValue(QVariant(theSlots[slot].fuelLevel).toInt());
   }
}

//***************************************************************************
// At Fuel Signal
//***************************************************************************

void LinslotWindow::atFuelSignal(int slot, const timeval*, int fuelStartSignal)
{
   if (!raceRunning)
      return ;

   tell(eloDebug, "Got '%s' signal for slot %d",
        fuelStartSignal ? "start fueling" : "stop fueling", slot);

   if (!setupDialog->getFuelingActive())
      return ;

   if (fuelStartSignal)
   {
      theSlots[slot].fuelTimer->start(setupDialog->getFuelingDelay()*1000);
   }
   else
   {
      theSlots[slot].fuelTimer->stop();
      switchOutputs(!slot ? isFuelingIndSlot12 : isFuelingIndSlot22, off);

      if (theSlots[slot].fueling)
      {
         theSlots[slot].fueling = no;
         tell(eloAlways, "Stopped fueling, slot (%d)", slot);
         playSound(sfFuelingInterrupted);
      }
   }
}

//***************************************************************************
// Decrement Fuel
//***************************************************************************

void LinslotWindow::decrementFuel(int slot, unsigned int usec)
{
   if (theSlots[slot].fuelLevel > 0)
   {
      double f = setupDialog->getFuelPerLap();
      int uAverageLap = (int)(setupDialog->getAverageLap() * 1000000.0);
      double diffPercent = labs(uAverageLap - usec) / (uAverageLap / 100.0);

      diffPercent = qMin(diffPercent, 30.0);

      tell(eloDebug3, "Debug: Diff to average lap %2d,%03d seconds => (%.2f%%), uAverageLap (%u), usec (%u)",
           (uAverageLap-usec)/1000000L, labs((uAverageLap-usec))%1000000L,
           diffPercent, uAverageLap, usec);

      double korr = 0;

      if (uAverageLap - usec > 0)
         korr = 100 + diffPercent/10.0 * setupDialog->getFuelFactorFastLap();  // faster
      else
         korr = 100 - diffPercent/10.0 * setupDialog->getFuelFactorSlowLap();  // slower

      theSlots[slot].fuelLevel -= f/100 * korr;

      tell(eloAlways, "Calc fuel, decrement with (%.2f liter) => (%.2f); korr was (%.2f)",
           f/100 * korr, theSlots[slot].fuelLevel, korr);

      if (theSlots[slot].fuelLevel <= 0)
      {
         theSlots[slot].fuelLevel = 0;
         atNoFuel(slot);
      }
   }
   else
   {
      atNoFuel(slot);
   }
}

//***************************************************************************
// At Slot Signal
//***************************************************************************

void LinslotWindow::atSlotSignal(int slot, const timeval* tp)
{
   int rnd = 0;
   int other = slot == 0 ? 1 : 0;
   unsigned int usec = 0;

   tell(eloDebug, "Debug: Signal slot %d", slot);

   if (slot == 0 && !thread->readOutBit(outputBitOf(bitPowerSlot1)) && !testMode)
      return;

   if (slot == 1 && !thread->readOutBit(outputBitOf(bitPowerSlot2)) && !testMode)
      return;

   // recording ghostcar

   if (gcState == gcsRecording && slot == gcSlot)
   {
      tell(eloAlways, "GC recording finished, got (%d) values", gcValues.size());
      gcState = gcsOff;
      gcSlot = na;
      thread->recordGhostCar(na, na);
      clearSlotPower();
      storeGcRecording(&gcValues);
      labelInfo->setText("Standby");
      labelFastLap->setText("");

      return ;
   }
   else if (gcState == gcsWaitingStart && slot == gcSlot)
   {
      tell(eloAlways, "GC recording started");
      gcState = gcsRecording;
      labelInfo->setText("Ghostcar");
      thread->recordGhostCar(analogBits[fctGhostUSlot1].bit,
                             analogBits[fctGhostISlot1].bit);

      return ;
   }
   else if (gcState == gcsRunning && slot == gcSlot)
   {
      thread->ghostCarSync();
   }

   if (!countdownStarted && !raceRunning)
      return ;

   // increment lap

   theSlots[slot].lap++;

   // check 'jump start'

   if (!raceRunning)
   {
      atJumpStart(slot);
      return ;
   }

   playSound(sfLapSignal);

   // show overview

   unsigned int diff;

   labelFirstTime->setText("");

   if (theSlots[slot].lap == theSlots[other].lap)
   {
      // same lap -> less than 1 lap behind other car ...

      diff = elapsed(&theSlots[other].lastSignal, tp);

      labelFirst->setText("1 " + QString(theSlots[other].driver));
      labelSecond->setText("2 " + QString(theSlots[slot].driver));
      labelSecondTime->setText("+" + QString::number(diff/1000000.0, 'f', 3) + "s");
   }
   else if (theSlots[slot].lap < theSlots[other].lap)
   {
      // over one lap behind other car ...

      diff = theSlots[other].lap - theSlots[slot].lap;

      labelFirst->setText("1 " + QString(theSlots[other].driver));
      labelSecond->setText("2 " + QString(theSlots[slot].driver));
      labelSecondTime->setText("+" + QString::number(diff) + " lap"
                               + QString(diff > 1 ? "s": ""));
   }
   else
   {
      QString oldSecond = labelSecond->text();

      // before other car

      diff = (theSlots[slot].lap - theSlots[other].lap) -1;


      labelFirst->setText("1 " + QString(theSlots[slot].driver));
      labelSecond->setText("2 " + QString(theSlots[other].driver));

      if (diff)
         labelSecondTime->setText("+" + QString::number(diff) + " lap"
                                  + QString(diff > 1 ? "s": ""));

      else if (oldSecond != labelSecond->text())
         labelSecondTime->setText("");
   }

   // erstes ueberfahren der Startlinie ?

   if (!theSlots[slot].lap)
      return ;

   // calc

   usec = elapsed(&theSlots[slot].lastSignal, tp);

   tell(eloDetail, "%d:%d)  %2d.%03d Sekunden (%02.2f km/h)",
        slot, theSlots[slot].lap,
        usec/1000000L, usec%1000000L,
        kmh(setupDialog->getSlotLength(), usec));


   theSlots[slot].tableWidget->setRowCount(theSlots[slot].lap);

   QTableWidgetItem* itemTime = new QTableWidgetItem(QString::number(usec/1000000.0, 'f', 3));
   QTableWidgetItem* itemKmh = new QTableWidgetItem(QString::number(kmh(setupDialog->getSlotLength(), usec)                                                                    * setupDialog->getSpeedFactor(), 'f', 2));

   // show lap overview

   if (!fastLapTime || fastLapTime > usec)
   {
      fastLapTime = usec;

      labelFastLap->setText("Schnellste Runde\n"
                            + QString(theSlots[slot].driver)
                            + QString("  -  ")
                            + QString::number(usec/1000000.0, 'f', 3)
                            + QString("  (")
                            + QString::number(kmh(setupDialog->getSlotLength(), usec)*setupDialog->getSpeedFactor(), 'f', 2)
                            + QString("km/h)"));
   }

   theSlots[slot].labelLastLap->setText(QString::number(usec/1000000.0, 'f', 3));

   if (!theSlots[slot].fastLapTime || theSlots[slot].fastLapTime > usec)
   {
      theSlots[slot].labelFastLap->setText(QString::number(usec/1000000.0, 'f', 3));
      theSlots[slot].fastLapTime = usec;
   }

   // fueling

   if (setupDialog->getFuelingActive())
      decrementFuel(slot, usec);

   //

   if (radioButtonLapRace->isChecked())
      rnd = setupDialog->getLapCountRace() - theSlots[slot].lap;
   else
      rnd = theSlots[slot].lap;

   theSlots[slot].tableWidget->setItem(theSlots[slot].lap-1, 0, itemTime);
   theSlots[slot].tableWidget->setItem(theSlots[slot].lap-1, 1, itemKmh);
   theSlots[slot].tableWidget->setCurrentCell(theSlots[slot].lap-1, 0);
   theSlots[slot].labelLapCount->setText(toStr(rnd));
   theSlots[slot].progressBarFuel->setValue(QVariant(theSlots[slot].fuelLevel).toInt());

   if (theSlots[slot].lap > theSlots[other].lap)
   {
      if (radioButtonLapRace->isChecked())
         labelLaps->setText(QString::number(rnd) + "/"
                            + QString::number(setupDialog->getLapCountRace()));
      else
         labelLaps->setText(QString::number(rnd) + "/"
                            + QString::number(setupDialog->getLapCountTraining()));
   }

   theSlots[slot].lastSignal = *tp;

   if (radioButtonLapRace->isChecked() &&
       theSlots[slot].lap >= setupDialog->getLapCountRace())
   {
      tell(eloDetail, "Race finished after %d laps of slot %d",
           theSlots[slot].lap, slot);
      atFinish(slot);
   }
   else if (radioButtonTraining->isChecked() &&
            theSlots[slot].lap >= setupDialog->getLapCountTraining())
   {
      tell(eloDetail, "Training finished after %d laps of slot %d",
           theSlots[slot].lap, slot);
      atFinish(na);
   }
}

//***************************************************************************
// Update Driver Images
//***************************************************************************

void LinslotWindow::updateDriverImage(int width, int height)
{
   QString path;

   for (int i = 0; i < slotCount; i++)
   {
      if (visibleImage == imgDriver)
         path = setupDialog->getDriverImage(theSlots[i].driver);
      else
         path = setupDialog->getCarImage(theSlots[i].car);

      if (path.size() && QFile::exists(path))
      {
         QPixmap pixmap(path);

         theSlots[i].labelImage->setPixmap(
            pixmap.scaled(width, height, Qt::KeepAspectRatio));
      }
      else
      {
         theSlots[i].labelImage->clear();
         theSlots[i].labelImage->setText(i == 1 ? ";)" : ":)");
      }
   }
}

//***************************************************************************
// On Animation Timer
//***************************************************************************

void LinslotWindow::onAnimateTimer()
{
   if (visibleImage == imgDriver)
      visibleImage = imgCar;
   else
      visibleImage = imgDriver;

   updateDriverImage(labelImageSlot1->width(),
                     labelImageSlot1->height());
}

//***************************************************************************
// Paint Event
//***************************************************************************

void LinslotWindow::paintEvent(QPaintEvent* event)
{
   int static lastHeight = na;
   int static lastWidth = na;

   // call inherited

   QWidget::paintEvent(event);

   // rescale image if nessecarry

   if (lastHeight != labelImageSlot1->height()
      || lastWidth != labelImageSlot1->width())
   {
      lastHeight = labelImageSlot1->height();
      lastWidth = labelImageSlot1->width();

      updateDriverImage(lastWidth, lastHeight);
   }
}

//***************************************************************************
// Open Db
//***************************************************************************

int LinslotWindow::openDb()
{
   char dbPath[255];
   int status;

   // db path

   sprintf(dbPath, "%s/%s", configPath.toLatin1().constData(),
           setupDialog->getDatabaseName());

   // Open database via QT

   QSqlDatabase _db;

   _db = QSqlDatabase::addDatabase("QSQLITE");
   _db.setDatabaseName(dbPath);

   if (!_db.open())
   {
      tell(eloAlways, "opening database failed :(");

      if (!_db.isDriverAvailable("QSQLITE"))
         QMessageBox::information(0, "Database", "QSQLITE is not available");
   }

   // the old way, used for highscores

   db = new SqliteDb(dbPath);

   if ((status = db->open()) != success)
   {
      tell(eloAlways, "Error: Open database '%s' failed. "  \
           "Result was (%d)", dbPath, status);
   }

   else
   {
      if ((status = db->execute("select * from races;")) != success)
      {
         if (QMessageBox::question(this, "Warnung", "Tabellen nicht gefunden, anlegen?",
                                   QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
         {
            createDb();

            if ((status = db->execute("select * from races;")) != success)
               QMessageBox::warning(this, "Fehler", "Tabellen konnten nicht angelegt werden!");
         }
      }
   }

   if (status != success)
   {
      db->close();
      delete db;
      db = 0;
   }

   return status;
}

//***************************************************************************
// Create Db
//***************************************************************************

int LinslotWindow::createDb()
{
   QSqlQuery* query;

   tell(eloDebug, "Creating tables");

   query = new QSqlQuery("CREATE TABLE races ("                          \
                         "RACE_ID INTEGER PRIMARY KEY AUTOINCREMENT, "    \
                         "DRIVER1 INTEGER, "                             \
                         "DRIVER2 INTEGER, "                             \
                         "DATE DATETIME, "                               \
                         "LAPS INTEGER, "                                \
                         "LAP_LENGTH REAL, "                             \
                         "COURSE TEXT"                                   \
                         ");");

   delete query;

   query = new QSqlQuery("CREATE TABLE laps ("                         \
                         "LAP_ID INTEGER PRIMARY KEY AUTOINCREMENT, "  \
                         "RACE_ID INTEGER, "                           \
                         "DRIVER_NR INTEGER, "                         \
                         "LAP_NR INTEGER, "                            \
                         "PROFILE_ID INTEGER, "                        \
                         "LAP_TIME REAL"                               \
                         ");");

   delete query;

   query = new QSqlQuery("CREATE TABLE drivers ("                       \
                         "DRIVER_ID INTEGER PRIMARY KEY AUTOINCREMENT, " \
                         "NAME TEXT"                                    \
                         ");");

   delete query;

   query = new QSqlQuery("CREATE TABLE courses ("                       \
                         "COURSE_ID INTEGER PRIMARY KEY AUTOINCREMENT, " \
                         "LENGTH REAL, "                                \
                         "NAME TEXT"                                    \
                         ");");


   delete query;

   query = new QSqlQuery("CREATE TABLE profiles ("                      \
                         "PROFILE_ID INTEGER PRIMARY KEY AUTOINCREMENT, " \
                         "NAME TEXT, "                                  \
                         "ACTIVE BOOLEAN, "                             \
                         "COURSE TEXT, "                                \
                         "COLOR TEXT, "                                 \
                         "LAP_LENGTH REAL"                              \
                         ");");

   delete query;

   query = new QSqlQuery("CREATE TABLE lap_profiles ("                  \
                         "LAP_PROFILE_ID INTEGER PRIMARY KEY AUTOINCREMENT, " \
                         "PROFILE_ID INTEGER, "                         \
                         "SEQUENCE INTEGER, "                           \
                         "VOLT INTEGER, "                               \
                         "AMPERE INTEGER"                               \
                         ");");

   delete query;

   return 0;
}


//***************************************************************************
// Show Database
//***************************************************************************

int LinslotWindow::showDb()
{
   db->execute("SELECT r.DRIVER1, r.DRIVER2, r.DATE, l.LAP_TIME from races as r join laps as l where r.RACE_ID = 1 and l.RACE_ID = r.RACE_ID;");
   db->show();

   return 0;
}

//***************************************************************************
// Store Race in Database
//***************************************************************************

int LinslotWindow::saveRace()
{
   sqlite3_stmt* sqlInsertRace;
   sqlite3_stmt* sqlInsertLap;
   int status;
   const char* duration;
   int driver1, driver2;
   // int courseId;

   if (!*theSlots[0].driver || !*theSlots[1].driver)
   {
      QMessageBox::critical(this, "Fehler", "Kein Fahrer gewählt!");
      return 0;
   }

   driver1 = getDriverId(theSlots[0].driver);
   driver2 = getDriverId(theSlots[1].driver);
   // courseId = getCourseId();

   db->prepare("INSERT INTO races(DRIVER1,DRIVER2,DATE,LAPS,LAP_LENGTH,COURSE) "\
               "VALUES(?,?,datetime(?, 'unixepoch', 'utc'),?,?,?);",
               sqlInsertRace);

   db->prepare("INSERT INTO laps(RACE_ID,DRIVER_NR,LAP_NR,LAP_TIME) "\
               "VALUES(?,?,?,?);",
               sqlInsertLap);

   db->execute("BEGIN;");

   db->reset(sqlInsertRace);

   db->bindInt(sqlInsertRace,    1, driver1);
   db->bindInt(sqlInsertRace,    2, driver2);
   db->bindInt(sqlInsertRace,    3, time(0));
   db->bindInt(sqlInsertRace,    4, setupDialog->getLapCountRace());
   db->bindDouble(sqlInsertRace, 5, setupDialog->getSlotLength());
   db->bindText(sqlInsertRace,   6, setupDialog->getCourseName());

   status = db->step(sqlInsertRace);

   if (status != 0)
      tell(eloAlways, "sqlite3_step(INSERT INTO races): '%s' (%d)",
           db->lastError(), status);

   int raceId = db->getInsertRowId();  // buggy ... ??

   for (int l = 0; l < tableWidgetSlot1->rowCount(); l++)
   {
      db->reset(sqlInsertLap);

      duration = 0;

      if (tableWidgetSlot1->item(l, 1))
         duration = tableWidgetSlot1->item(l, 0)->text().toLatin1();

      db->bindInt(sqlInsertLap, 1, raceId);       // RACE_ID
      db->bindInt(sqlInsertLap, 2, driver1);      // DRIVER_NR
      db->bindInt(sqlInsertLap, 3, l+1);          // LAP_NR

      if (duration)
         db->bindText(sqlInsertLap, 4, duration); // LAP_TIME
      else
         db->bindNull(sqlInsertLap, 4);           // LAP_TIME

      status = db->step(sqlInsertLap);

      if (status != 0)
         tell(eloAlways, "sqlite3_step(INSERT INTO laps): %s (%d)",
              db->lastError(), status);
   }

   for (int l = 0; l < tableWidgetSlot2->rowCount(); l++)
   {
      db->reset(sqlInsertLap);

      duration = 0;

      if (tableWidgetSlot2->item(l, 1))
         duration = tableWidgetSlot2->item(l, 0)->text().toLatin1();

      db->bindInt(sqlInsertLap, 1, raceId);       // RACE_ID
      db->bindInt(sqlInsertLap, 2, driver2);      // DRIVER_NR
      db->bindInt(sqlInsertLap, 3, l+1);          // LAP_NR

      if (duration)
         db->bindText(sqlInsertLap, 4, duration); // LAP_TIME
      else
         db->bindNull(sqlInsertLap, 4);           // LAP_TIME

      status = db->step(sqlInsertLap);

      if (status != 0)
         tell(eloAlways, "sqlite3_step(INSERT INTO laps): %s (%d)",
              db->lastError(), status);
   }

   db->execute("COMMIT;");

   db->finalize(sqlInsertRace);
   db->finalize(sqlInsertLap);

   return 0;
}

//***************************************************************************
// Get Driver Id
//***************************************************************************

int LinslotWindow::getDriverId(const char* driver)
{
   sqlite3_stmt* sqlSelectDriver;
   sqlite3_stmt* sqlInsertDriver;
   char sql[1000+TB];
   int status;
   int id;

   // get driver ID's

   sprintf(sql, "SELECT * from drivers where NAME=?;");
   db->prepare(sql, sqlSelectDriver);
   db->bindText(sqlSelectDriver, 1, driver);

   status = db->steps(sqlSelectDriver);

   db->reset(sqlSelectDriver);
   db->finalize(sqlSelectDriver);

   if (status == 0)
      id = db->getIntValueOf("DRIVER_ID");
   else
   {
      sprintf(sql, "INSERT INTO drivers(NAME) values(?);");
      db->prepare(sql, sqlInsertDriver);
      db->execute("BEGIN;");
      db->bindText(sqlInsertDriver, 1, driver);
      db->step(sqlInsertDriver);
      db->execute("COMMIT;");
      id = db->getInsertRowId();
      db->finalize(sqlInsertDriver);
   }

   return id;
}

//***************************************************************************
// Get Driver Id
//***************************************************************************

int LinslotWindow::getCourseId()
{
   sqlite3_stmt* sqlSelect;
   sqlite3_stmt* sqlInsert;
   char sql[1000];
   int status;
   int id;

   // get driver ID's

   sprintf(sql, "SELECT * from cources where NAME=?;");
   db->prepare(sql, sqlSelect);
   db->bindText(sqlSelect, 1, setupDialog->getCourseName());

   status = db->steps(sqlSelect);

   db->reset(sqlSelect);
   db->finalize(sqlSelect);

   if (status == 0)
      id = db->getIntValueOf("COURSE_ID");
   else
   {
      sprintf(sql, "INSERT INTO courses(NAME,LENGTH) values(?,?);");
      db->prepare(sql, sqlInsert);
      db->execute("BEGIN;");
      db->bindText(sqlInsert, 1, setupDialog->getCourseName());
      db->bindDouble(sqlInsert, 2, setupDialog->getSlotLength());
      db->step(sqlInsert);
      db->execute("COMMIT;");
      id = db->getInsertRowId();
      db->finalize(sqlInsert);
   }

   return id;
}

//***************************************************************************
//
//***************************************************************************

void LinslotWindow::on_toolButtonRecordGhostCar_clicked()
{
   if (gcState == gcsOff)
   {
      tell(eloAlways, "GC waiting for start signal");
      labelInfo->setText("Ghostcar");
      labelFastLap->setText("bereit");
      gcState = gcsWaitingStart;
      gcSlot = 0;
      gcValues.clear();
      setSlotPower();
   }
   else
   {
      thread->recordGhostCar(na, na);
      labelFastLap->setText("abbruch");
      labelInfo->setText("Standby");
      tell(eloAlways, "GC aborted");
      gcState = gcsOff;
      gcSlot = na;
   }
}

//***************************************************************************
// Store Ghostcar Data
//***************************************************************************

void LinslotWindow::storeGcRecording(QList<unsigned short>* values)
{
   QSqlQuery* insert = new QSqlQuery;
   bool ok = true;
   QString name = "";
   QString hint = "";
   int profileId;

   while (ok)
   {
      name = QInputDialog::getText(this, "Speichern ?",
                                   hint + "Name: ", QLineEdit::Normal,
                                   "", &ok);

      if (ok && !name.isEmpty())
      {
         // name schon vergeben ... ?

         QSqlQuery query("select NAME from profiles "
                         "where NAME = '" + name + "';");

         if (!query.next())
            break ;

         hint = "Name bereits vergeben. ";
      }
   }

   if (ok && !name.isEmpty())
   {
      // create profile record

      insert->prepare("INSERT INTO profiles("
                      "NAME, ACTIVE, COURSE, LAP_LENGTH) "
                      "VALUES(:name, :active, :course, :length);");

      insert->bindValue(":name", name);
      insert->bindValue(":active", true);
      insert->bindValue(":course", setupDialog->getCourseName());
      insert->bindValue(":length", setupDialog->getSlotLength());
      insert->exec();

      // get id of new profile

      QSqlQuery query("select PROFILE_ID from profiles "
                      "where NAME = '" + name + "';");

      if (!query.next())
      {
         tell(eloAlways, "Fatal: Can't find new created profile");
         return ;
      }

      profileId = query.value(0).toInt();

      // create lap profile records

      insert->prepare("INSERT INTO lap_profiles("
                      "PROFILE_ID, SEQUENCE, VOLT, AMPERE) "
                      "VALUES(:prid, :seq, :volt, :ampere);");

      for (int i = 0; i < values->size(); i++)
      {
         insert->bindValue(":prid", profileId);
         insert->bindValue(":seq", i);
         insert->bindValue(":volt", values->at(i) & 0x00FF);
         insert->bindValue(":ampere", values->at(i) >> 8);

         if (!insert->exec())
         {
            tell(eloAlways,  "Daten konnten nicht gespeichert werden (%d)!", i);
            break ;
         }
      }
   }

   // applyOptions();

   delete insert;
}

//***************************************************************************
// Test Mode
//***************************************************************************

void LinslotWindow::simulateEvent(int fct, int state)
{
   static unsigned int lastValue = 0x0FFFFFFFF;
   DigitalEvent event;

   gettimeofday(&event.tp, 0);

   if (inputBits[fct].mode != teRising)
      state = !state;

   setBitTo(lastValue, inputBitOf(fct), state);

   event.value = lastValue;
   emit onDigitalInput(event);
}

void LinslotWindow::on_toolButtonSignalSlot1_pressed()
{
   simulateEvent(bitIrSlot1, yes);
}

void LinslotWindow::on_toolButtonSignalSlot1_released()
{
   simulateEvent(bitIrSlot1, no);
}

void LinslotWindow::on_toolButtonSignalSlot2_pressed()
{
   simulateEvent(bitIrSlot2, yes);
}

void LinslotWindow::on_toolButtonSignalSlot2_released()
{
   simulateEvent(bitIrSlot2, no);
}

//***************************************************************************
// Show Profiles
//***************************************************************************

void LinslotWindow::on_toolButtonTest_clicked()
{
   QDialog* profileDialog = new QDialog(this);

   RenderArea* renderArea = new RenderArea(this);
   QGridLayout* mainLayout = new QGridLayout;
   QToolButton* scaleUp = new QToolButton(this);
   QToolButton* scaleDown = new QToolButton(this);
   QToolButton* scrollLeft = new QToolButton(this);
   QToolButton* scrollRight = new QToolButton(this);
   QToolButton* edit = new QToolButton(this);
   QToolButton* remove = new QToolButton(this);
   QCheckBox* checkAmpere = new QCheckBox(this);
   QCheckBox* checkVolt = new QCheckBox(this);
   QCheckBox* checkPower = new QCheckBox(this);

   QTableView* tableView = new QTableView(this);
   QSqlTableModel* model = new QSqlTableModel(this);
   QSplitter* splitter = new QSplitter(this);

   model->setTable("PROFILES");
   model->removeColumn(model->fieldIndex("COLOR"));
   model->removeColumn(model->fieldIndex("ACTIVE"));
   model->setHeaderData(model->fieldIndex("PROFILE_ID"), Qt::Horizontal, "Id");
   model->setHeaderData(model->fieldIndex("NAME"), Qt::Horizontal, "Name");
   model->setHeaderData(model->fieldIndex("COURSE"), Qt::Horizontal, "Strecke");
   model->setHeaderData(model->fieldIndex("LAP_LENGTH"), Qt::Horizontal, "Länge");

   model->select();
   tableView->setModel(model);
   tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
   tableView->setMaximumSize(400, 9999999);

   tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
   tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
   tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

   /* QT4
   tableView->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
   tableView->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
   tableView->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
   tableView->horizontalHeader()->setResizeMode(3, QHeaderView::Fixed); */

   profileDialog->setWindowTitle("Rundenprofile");

   scaleUp->setAutoRepeat(true);
   scaleUp->setText("+");
   scaleDown->setAutoRepeat(true);
   scaleDown->setText("-");
   scrollLeft->setAutoRepeat(true);
   scrollLeft->setText("<");
   scrollRight->setAutoRepeat(true);
   scrollRight->setText(">");
   edit->setText("bearbeiten");
   remove->setText("löschen");
   checkVolt->setText("Spannung");
   checkAmpere->setText("Strom");
   checkPower->setText("Leistung");
   checkVolt->setChecked(yes);
   checkAmpere->setChecked(yes);
   checkPower->setChecked(yes);

   splitter->addWidget(renderArea);
   splitter->addWidget(tableView);

   QList<int> sizes;
   sizes << 750 << 250;
   splitter->setSizes(sizes);

   mainLayout->addWidget(splitter, 0, 0, 1, 9);

   mainLayout->addWidget(scaleUp, 1, 0);
   mainLayout->addWidget(scaleDown, 1, 1);
   mainLayout->addWidget(scrollLeft, 1, 2);
   mainLayout->addWidget(scrollRight, 1, 3);
   mainLayout->addWidget(checkVolt, 1, 4);
   mainLayout->addWidget(checkAmpere, 1, 5);
   mainLayout->addWidget(checkPower, 1, 6);
   mainLayout->addWidget(remove, 1, 7, Qt::AlignRight);
   mainLayout->addWidget(edit, 1, 8, Qt::AlignRight);

   connect(scaleUp, SIGNAL(clicked(bool)), renderArea, SLOT(scaleUpClicked(bool)));
   connect(scaleDown, SIGNAL(clicked(bool)), renderArea, SLOT(scaleDownClicked(bool)));
   connect(scrollLeft, SIGNAL(clicked(bool)), renderArea, SLOT(scrollLeftClicked(bool)));
   connect(scrollRight, SIGNAL(clicked(bool)), renderArea, SLOT(scrollRightClicked(bool)));
   connect(tableView, SIGNAL(doubleClicked(const QModelIndex&)), renderArea, SLOT(doubleClicked(const QModelIndex&)));
   connect(edit, SIGNAL(clicked(bool)), renderArea, SLOT(editClicked(bool)));
   connect(remove, SIGNAL(clicked(bool)), renderArea, SLOT(removeClicked(bool)));
   connect(checkVolt, SIGNAL(stateChanged(int)), renderArea, SLOT(checkVolt(int)));
   connect(checkAmpere, SIGNAL(stateChanged(int)), renderArea, SLOT(checkAmpere(int)));
   connect(checkPower, SIGNAL(stateChanged(int)), renderArea, SLOT(checkPower(int)));

   profileDialog->setLayout(mainLayout);
   profileDialog->resize(1000, 600);

   renderArea->setTableView(tableView);
   renderArea->setGcScale(thread->getGcScale());
   tableView->resizeColumnsToContents();
   tableView->resizeRowsToContents();


   // profileDialog->setAnimated(false);
   profileDialog->exec();
   // applyOptions();

   delete profileDialog;
}
