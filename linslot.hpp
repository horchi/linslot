//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File linslot.hpp
// Date 05.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _LINSLOT_H_
#define _LINSLOT_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <QMainWindow>
#include <QThread>
#include <QReadWriteLock>
#include <QTimer>
#include <QDir>

#include <ui_linslot.h>

#include <highscore.hpp>
#include <setup.hpp>
#include <iothread.hpp>

#define BUILD "0.0.17"

//***************************************************************************
// Class LinslotWindow
//***************************************************************************

class LinslotWindow : public QMainWindow, private Ui::LinslotWindow, public SlotService
{
   Q_OBJECT

   public:

      // declarations

      enum Misc
      {
         sizeName       = 50,
         bitsPerByte    = 8,

         bounceTime     = 30000,    // µSeconds (0.03 sec)
         slotCount      = 2
      };

      enum PowerState
      {
         psSlot1 = 1,
         psSlot2 = 2,
         psSlot3 = 4,
         psSlot4 = 8,

         psAll = psSlot1 | psSlot2 | psSlot3 | psSlot4
      };

      enum IndicatorState
      {
         isAllOff     = 0x0,

         isGreen      = 0x01,
         isRed1       = 0x02,
         isRed2       = 0x04,
         isRed3       = 0x08,
         isRed4       = 0x010,
         isRed5       = 0x020,

         isPhase1     = isRed1,
         isPhase2     = isPhase1 | isRed2,
         isPhase3     = isPhase2 | isRed3,
         isPhase4     = isPhase3 | isRed4,
         isPhase5     = isPhase4 | isRed5,
         isPhase6     = isGreen,

         isNoPowerIndSlot1  = 0x040,
         isNoPowerIndSlot2  = 0x080,
         isNoPowerIndSlot3  = 0x0100,
         isNoPowerIndSlot4  = 0x0200,
         isNoPowerIndSlot12 = isNoPowerIndSlot1 | isNoPowerIndSlot2,  // Spur 1 bei 2 Spuren
         isNoPowerIndSlot22 = isNoPowerIndSlot3 | isNoPowerIndSlot4,  // Spur 2 bei 2 Spuren
         isNoPowerInd       = isNoPowerIndSlot12 | isNoPowerIndSlot22,

         isFuelingIndSlot1  = 0x0400,
         isFuelingIndSlot2  = 0x0800,
         isFuelingIndSlot3  = 0x01000,
         isFuelingIndSlot4  = 0x02000,
         isFuelingIndSlot12 = isFuelingIndSlot1 | isFuelingIndSlot2,  // Spur 1 bei 2 Spuren
         isFuelingIndSlot22 = isFuelingIndSlot3 | isFuelingIndSlot4,  // Spur 2 bei 2 Spuren
         isFuelingInd       = isFuelingIndSlot12 | isFuelingIndSlot22,

         isPenaltyIndSlot1  = 0x04000,
         isPenaltyIndSlot2  = 0x08000,
         isPenaltyIndSlot3  = 0x010000,
         isPenaltyIndSlot4  = 0x020000,
         isPenaltyIndSlot12 = isPenaltyIndSlot1 | isPenaltyIndSlot2,   // Spur 1 bei 2 Spuren
         isPenaltyIndSlot22 = isPenaltyIndSlot3 | isPenaltyIndSlot4,   // Spur 2 bei 2 Spuren
         isPenaltyInd       = isPenaltyIndSlot12 | isPenaltyIndSlot22,

         isAllOn      = isPhase5 | isGreen | isNoPowerInd | isFuelingInd | isPenaltyInd
      };

      enum GhostCarState
      {
         gcsOff,
         gcsWaitingStart,
         gcsRecording,
         gcsRunning
      };

      enum VisibleImage
      {
         imgDriver,
         imgCar
      };

      struct Slot
      {
         timeval lastSignal;
         int lap;
         double fastLapTime;
         char driver[sizeName+TB];
         char car[sizeName+TB];
         int penalty;
         double fuelLevel;            // fuel level
         int fueling;
         int gcProfile;
         QString getDriver() { return comboDriver->currentText();}
         QString getCar()    { return comboCar->currentText();}

         QTimer* fuelTimer;
         QLabel* labelLapCount;
         QProgressBar* progressBarFuel;
         QLabel* labelFastLap;
         QLabel* labelLastLap;
         QLabel* labelInfo;
         QLabel* labelElapsedLap;
         QLabel* labelImage;
         QTableWidget* tableWidget;
         QComboBox* comboDriver;
         QComboBox* comboCar;
      };

      // object

      LinslotWindow();
      ~LinslotWindow();

      void init();
      void exit();
      // int initInterface();
      void initRace();
      void ioOpened();

   protected:

      // functions

      void atSlotSignal(int slot, const timeval* tp);
      void atFuelSignal(int slot, const timeval* tp, int fuelStartSignal);
      void atStartCountdown();
      void atStart();
      void atStartTraining();
      void atFinish(int slot);
      void atAbort(const char* info);
      void atJumpTheGun(int slot);
      void atNoFuel(int slot);
      void atStop(const char* info);

      int getImagesFor(int led, const char* &ledOn, const char* &ledOff);
      void setOutputs(int mask);
      void switchOutputs(int mask, int state);
      void setLed(int function, int state);
      void setSlotPower(int slot, int flag);
      void setPenalty(int slot);
      void clearPenalty(int slot);
      void setSlotPower();
      void clearSlotPower();

      unsigned int getChanges(DigitalEvent &ioEvent);
      int isActiveEdge(int bit, int value);
      int inputBitOf(int function);
      int functionOfInput(int bit);
      int outputBitOf(int function);
      void writeBit(int function, int state);
      void simulateEvent(int bit, int state);
      void playSound(int fct);

      void resetWidgets();
      void applyOptions();
      void storeConfig();
      void atFuelTimer(int slot);
      void decrementFuel(int slot, unsigned int usec);
      void storeGcRecording(QList<unsigned short>* values);
      void updateDriverImage(int width, int height);
      void paintEvent(QPaintEvent* event);

      // db stuff

      int openDb();
      int createDb();
      int saveRace();
      int showDb();
      int getDriverId(const char* driver);
      int getCourseId();

      // data

      int testMode;
      IoThread* thread;
      QTimer* timer;
      QTimer* timerFlash;
      QTimer* timerElapsed;
      QTimer* timerPenalty;
      QTimer* timerAnimateImage;
      Slot theSlots[slotCount];
      timeval raceStart;
      int raceRunning;
      int countdownStarted;
      int countdown;
      QObject* ledsParent;
      double fastLapTime;
      int withSound;
      byte oldSpiSetting;
      int visibleImage;
      int supressComboBoxUpdate;

      SqliteDb* db;

      char* resourcePath;
      QString configPath;

      QList<unsigned short> gcValues;
      int gcState;
      int gcSlot;

      // dialogs

      SetupDialog* setupDialog;
      HighscoreDialog* highscoreDialog;

      int outputFunctionState[fctOutputCount];

   private slots:

      void onDigitalInput(const DigitalEvent ioEvent);
      void onAnalogInput(const AnalogEvent ioEvent);

      void on_pushButtonPower_clicked();
      void on_pushButtonSaveToDb_clicked();
      void on_pushButtonHallOfFame_clicked();
      void on_pushButtonStartRace_clicked();
      void on_pushButtonOptions_clicked();
      void on_toolButtonTest_clicked();
      void on_toolButtonRecordGhostCar_clicked();
      void onTimer();
      void onTimerFlash();
      void onFuelTimerSlot1();
      void onFuelTimerSlot2();
      void onElapsedTimer();
      void onPenaltyTimer();
      void onAnimateTimer();
      void onOptionsAccepted();
      void on_comboBoxDriver1_currentIndexChanged(QString value);
      void on_comboBoxDriver2_currentIndexChanged(QString value);
      void on_comboBoxCar1_currentIndexChanged(QString value);
      void on_comboBoxCar2_currentIndexChanged(QString value);

      void on_toolButtonSignalSlot1_pressed();
      void on_toolButtonSignalSlot1_released();
      void on_toolButtonSignalSlot2_pressed();
      void on_toolButtonSignalSlot2_released();
};

//***************************************************************************
#endif // _LINSLOT_H_
