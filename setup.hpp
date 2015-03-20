//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File setup.hpp
// Date 05.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _SETUP_H_
#define _SETUP_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <QDialog>
#include <QSettings>

#include <ui_setup.h>
#include <common.hpp>

//***************************************************************************
// Class LinslotWindow
//***************************************************************************

class SetupDialog : public QDialog, public Ui::SetupDialog, public SlotService
{
   Q_OBJECT
      
   public:

      enum DriverImageMode
      {
         mdDriver,
         mdCar,
         mdAnimated
      };
      
      // object
      
      SetupDialog(QString configPath);
      ~SetupDialog();

      // gettings

      void reset();

      QSettings* getSettings()      { return settings; }

      int getLapCountRace()         { return lapCountRace; }       // race
      int getLapCountTraining()     { return lapCountTraining; }   // training
      double getSlotLength()        { return slotLength; }  // Meter
      const char* getUsbDevice()    { return usbDevice; }
      const char* getAlsaDevice()   { return alsaDevice; }
      int getMaxTimeLapRace()       { return maxTimeLapRace; }
      int getMaxTimeTraining()      { return maxTimeTraining; }
      int getSpeedFactor()          { return speedFactor; }
      const char* getCourseName()   { return courseName; }
      const char* getDatabaseName() { return databaseName; }
      const char* getResourcePath() { return resourcePath; }
      double getAverageLap()        { return averageLap; }
      int getAbortAtJumpTheGun()    { return abortAtJumpTheGun; }
      int getPenaltyAtJumpTheGun()  { return penaltyAtJumpTheGun; }

      double getFuelMax()           { return fuelMax; }
      double getFuelPerLap()        { return fuelPerLap; }
      int getFuelingActive()        { return fuelingActive; }
      int getFuelingDelay()         { return fuelingDelay; }
      int getFuelingPerSecond()     { return fuelingPerSecond; }
      int getFuelFactorFastLap()    { return fuelFactorFastLap; }
      int getFuelFactorSlowLap()    { return fuelFactorSlowLap; }
      int getFuelPenaltyTime()      { return fuelPenaltyTime; }
      word getOutputMask();
      word getInputMask();
      byte getWithSpiExtension()    { return withSpiExtension ? yes : no; }
      int getGhostcarInvert()       { return yes; }  // todo: einstellbar
      QString getDriverImage(QString name) { return driverImages[name]; }
      QString getCarImage(QString car) { return carImages[car]; }
      int getAnimationInterval()    { return animationInterval; }
      int getDriverImageMode()      { return driverImageMode; }
      int hasDriverChanged()        { return driverChanged; }
      int hasCarChanged()           { return carChanged; }

   protected:
      
      QSettings* settings;
      int lapCountRace;              // lap limit for races
      int lapCountTraining;          // lap limit for training
      double slotLength;
      int maxTimeLapRace;
      int maxTimeTraining;
      int speedFactor;
      char courseName[100+TB];
      char usbDevice[100+TB];
      char alsaDevice[255+TB];
      char databaseName[100+TB];
      char resourcePath[1000+TB];
      int abortAtJumpTheGun;
      int penaltyAtJumpTheGun;
      double fuelPerLap;
      double fuelMax;
      int fuelingDelay;
      int fuelingPerSecond;
      int fuelFactorFastLap;
      int fuelFactorSlowLap; 
      double averageLap;
      int fuelingActive;
      int fuelPenaltyTime;
      byte withSpiExtension;
      int animationInterval;
      int driverImageMode;
      QHash<QString, QString> driverImages;
      QHash<QString, QString> carImages;

      int driverChanged;
      int carChanged;

   private slots:

      void on_tabWidget_currentChanged(int tab);
      void on_pushButtonAddDriver_clicked();
      void on_pushButtonDelDriver_clicked();
      void on_listWidgetDriver_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
      void on_pushButtonAddImage_clicked();

      void on_pushButtonAddCar_clicked();
      void on_pushButtonDelCar_clicked();
      void on_listWidgetCar_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
      void on_pushButtonAddCarImage_clicked();

      void on_spinBoxLapCount_valueChanged(int value);
      void on_spinBoxTrainingLapLimit_valueChanged(int value);
      void on_spinBoxJumpTheGunPenaltyTime_valueChanged(int value);
      void on_doubleSpinBoxSlotLength_valueChanged(double value);
      void on_comboBoxDevice_currentIndexChanged(const QString text);
      void on_comboBoxDevice_editTextChanged(const QString text);
      void on_lineEditCourse_editingFinished();
      void on_lineEditDbName_editingFinished();
      void on_lineEditResourcePath_editingFinished();
      void on_timeEditLapRace_editingFinished();
      void on_timeEditTraining_editingFinished();
      void on_radioButtonFaktor1_toggled(bool checked);
      void on_radioButtonFaktor24_toggled(bool checked);
      void on_radioButtonFaktor32_toggled(bool checked);
      void on_radioButtonFaktor43_toggled(bool checked);
      void on_radioButtonJumpTheGunAbort_toggled(bool checked);
      void on_radioButtonJumpTheGunPenalty_toggled(bool checked);
      void on_doubleSpinBoxFuelMax_valueChanged(double value);
      void on_doubleSpinBoxFuelPerLap_valueChanged(double value);
      void on_spinBoxFuelingDelay_valueChanged(int value);
      void on_spinBoxFuelingPerSecond_valueChanged(int value);
      void on_spinBoxFuelFactorFastLap_valueChanged(int value);
      void on_spinBoxFuelFactorSlowLap_valueChanged(int value);
      void on_doubleSpinBoxAverageLap_valueChanged(double value);
      void on_checkBoxFueling_stateChanged(int state);
      void on_spinBoxFuelPenaltyTime_valueChanged(int value);
      void outputSignalChanged(int row, int col);
      void inputSignalChanged(int row, int col);
      void analogInputChanged(int row, int col);
      void on_checkBoxWithSpiExtension_stateChanged(int state);
      void on_pushButtonRemoveDriverImage_clicked();
      void on_pushButtonRemoveCarImage_clicked();
      void on_radioButtonDriverImage_toggled(bool checked);
      void on_radioButtonCarImage_toggled(bool checked);
      void on_radioButtonAnimateImage_toggled(bool checked);
      void on_spinBoxAnimationInterval_valueChanged(int value);
      void on_comboBoxAlsaDevice_currentIndexChanged(const QString text);
      void on_tableWidgetSound_cellDoubleClicked(int row, int column);
      void soundSignalChanged(int row, int col);
};

//***************************************************************************
#endif // _SETUP_H_
