//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File setup.cc
// Date 16.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>

#include <common.hpp>
#include <setup.hpp>
#include <delegateitems.hpp>

#ifndef Q_OS_WIN32
#  include <alsa.hpp>
#endif

// #TODO FIXME !!
// reset all settings on cancel !!

//***************************************************************************
// Object
//***************************************************************************

SetupDialog::SetupDialog(QString configPath)
{
   int count;
   char tmpDevice[100+TB];

   driverChanged = yes;
   carChanged = yes;

   setupUi(this);

   QCoreApplication::setOrganizationName("Jörg Wendel");
   QCoreApplication::setApplicationName("linslot") ;

   settings = new QSettings(configPath + "/linslotrc", QSettings::IniFormat);

   settings->beginGroup("common");

   lapCountRace = settings->value("lapCount", 10).toInt();
   lapCountTraining = settings->value("lapCountTraining", 50).toInt();
   slotLength = settings->value("slotLength", 14.0).toDouble();
   maxTimeLapRace = settings->value("maxTimeLapRace", 0).toInt();
   maxTimeTraining = settings->value("maxTimeTraining", 0).toInt();
   speedFactor = settings->value("speedFactor", 1).toInt();
   abortAtJumpTheGun = settings->value("abortAtJumpTheGun", 0).toInt();
   strncpy(tmpDevice, settings->value("usbDevice", "/dev/ttyUSB0").toString().toLatin1(), 100);
   tmpDevice[100] = 0;
   strcpy(alsaDevice, settings->value("alsaDevice", "default").toString().toLatin1());
   strcpy(courseName, settings->value("course", "Heimstrecke").toString().toLatin1());
   strcpy(databaseName, settings->value("databaseName", "linslot.db").toString().toLatin1());
   strcpy(resourcePath, settings->value("resourcePath", "/usr/local/share/linslot/").toString().toLatin1());
   penaltyAtJumpTheGun = settings->value("penaltyAtJumpTheGun", 5).toInt();
   fuelMax = settings->value("fuelMax", 75.0).toDouble();
   fuelPerLap = settings->value("fuelPerLap", 9.5).toDouble();
   fuelingDelay = settings->value("fuelingDelay", 2).toInt();
   fuelingPerSecond = settings->value("fuelingPerSecond", 15).toInt();
   fuelFactorFastLap = settings->value("fuelFactorFastLap", 8).toInt();
   fuelFactorSlowLap = settings->value("fuelFactorSlowLap", 8).toInt();
   averageLap = settings->value("averageLap", 12.5).toDouble();
   fuelingActive = settings->value("fuelingActive", yes).toInt();
   fuelPenaltyTime = settings->value("fuelPenaltyTime", 5).toInt();
   withSpiExtension = settings->value("withSpiExtension", true).toBool();
   driverImageMode = settings->value("driverImageMode", mdAnimated).toInt();
   animationInterval = settings->value("animationInterval", 5).toInt();
   settings->endGroup();

   // fahrer

   settings->beginGroup("driver");
   count = settings->beginReadArray("driver");

   for (int i = 0; i < count; ++i)
   {
      settings->setArrayIndex(i);
      listWidgetDriver->addItem(settings->value("name").toString());

      if (settings->value("image").toString().size())
         driverImages[settings->value("name").toString()] = settings->value("image").toString();
   }

   settings->endArray();
   settings->endGroup();

   // fahrzeuge

   settings->beginGroup("cars");
   count = settings->beginReadArray("car");

   for (int i = 0; i < count; ++i)
   {
      settings->setArrayIndex(i);
      listWidgetCar->addItem(settings->value("name").toString());

      if (settings->value("image").toString().size())
         carImages[settings->value("name").toString()] = settings->value("image").toString();
   }

   settings->endArray();
   settings->endGroup();

   tabWidget->setCurrentIndex(0);

   // ------------------------------
   // read input signal definitions

   settings->beginGroup("inputSignals");
   count = settings->beginReadArray("inputSignals");

   if (count > bitInputCount)
   {
      tell(eloAlways, "Warning: To many input signals configured, ignoring");
      count = bitInputCount;
   }

   for (int i = 0; i < count; ++i)
   {
      settings->setArrayIndex(i);

      inputBits[i].bit = settings->value("bit", inputBits[i].bit).toInt();
      inputBits[i].mode = (TriggerEdge)settings->value("mode", inputBits[i].mode).toInt();
   }

   settings->endArray();
   settings->endGroup();

   // --------------------------------
   // read output signals definitions

   settings->beginGroup("outputSignals");
   count = settings->beginReadArray("outputSignals");

   if (count > fctOutputCount)
   {
      tell(eloAlways, "Warning: To many output signals configured, ignoring");
      count = fctOutputCount;
   }

   for (int i = 0; i < count; ++i)
   {
      settings->setArrayIndex(i);

      outputBits[i].bit = settings->value("bit", outputBits[i].bit).toInt();
      outputBits[i].mode = (OutputMode)settings->value("mode", outputBits[i].mode).toInt();
      outputBits[i].ledid = (LedId)settings->value("led", outputBits[i].ledid).toInt();
   }

   settings->endArray();
   settings->endGroup();

   // -----------------------
   // read sound definitions

   settings->beginGroup("soundSignals");
   count = settings->beginReadArray("soundSignals");

   if (count > sfCount)
   {
      tell(eloAlways, "Warning: To many sound signals defined, ignoring");
      count = sfCount;
   }

   for (int i = 0; i < count; ++i)
   {
      settings->setArrayIndex(i);
      sounds[i].sound = settings->value("sound", sounds[i].sound).toString();
   }

   settings->endArray();
   settings->endGroup();


#ifndef Q_OS_WIN32

   // sound devices

   int i = 0;
   int alsaIndex = 0;

   while (const QAlsaSound::Device* d = QAlsaSound::getDevice(i++))
   {
      if (d->name.indexOf("default") == -1)
         continue;

      QStringList l = d->desc.split('\n', QString::SkipEmptyParts);

      if (l.size())
      {
         comboBoxAlsaDevice->addItem(l.at(0));

         if (d->name == alsaDevice)
            alsaIndex = comboBoxAlsaDevice->count()-1;
      }
   }

   comboBoxAlsaDevice->setCurrentIndex(alsaIndex);

#else
   comboBoxAlsaDevice->setVisible(false);
   labelAlsaDevice->setVisible(false);
#endif

   // serial devices
   // todo other serial devices ... ?

#ifndef Q_OS_WIN32
   comboBoxDevice->addItem("/dev/ttyUSB0");
   comboBoxDevice->addItem("/dev/ttyUSB1");
   comboBoxDevice->addItem("/dev/ttyUSB2");
   comboBoxDevice->addItem("/dev/ttyUSB3");
#else
   comboBoxDevice->addItem("COM1");
   comboBoxDevice->addItem("COM2");
   comboBoxDevice->addItem("COM3");
#endif

   comboBoxDevice->setCurrentIndex(comboBoxDevice->findText(tmpDevice));

   lineEditCourse->setText(courseName);
   lineEditDbName->setText(databaseName);
   lineEditResourcePath->setText(resourcePath);
   doubleSpinBoxSlotLength->setValue(slotLength);
   spinBoxLapCount->setValue(lapCountRace);
   spinBoxTrainingLapLimit->setValue(lapCountTraining);
   spinBoxJumpTheGunPenaltyTime->setValue(penaltyAtJumpTheGun);
   spinBoxAnimationInterval->setValue(animationInterval);

   doubleSpinBoxFuelPerLap->setValue(fuelPerLap);
   doubleSpinBoxFuelMax->setValue(fuelMax);
   spinBoxFuelingDelay->setValue(fuelingDelay);
   spinBoxFuelingPerSecond->setValue(fuelingPerSecond);
   spinBoxFuelFactorFastLap->setValue(fuelFactorFastLap);
   spinBoxFuelFactorSlowLap->setValue(fuelFactorSlowLap);
   doubleSpinBoxAverageLap->setValue(averageLap);
   checkBoxFueling->setChecked(fuelingActive);
   groupBoxFueling->setEnabled(fuelingActive);
   spinBoxFuelPenaltyTime->setValue(fuelPenaltyTime);
   checkBoxWithSpiExtension->setChecked(withSpiExtension);

   if (abortAtJumpTheGun)
      radioButtonJumpTheGunAbort->setChecked(true);
   else
      radioButtonJumpTheGunPenalty->setChecked(true);

   if (driverImageMode == mdDriver)
      radioButtonDriverImage->setChecked(true);
   else if (driverImageMode == mdCar)
      radioButtonCarImage->setChecked(true);
   else
      radioButtonAnimateImage->setChecked(true);

   switch (speedFactor)
   {
      case 1:  radioButtonFaktor1->setChecked(true);   break;
      case 24: radioButtonFaktor24->setChecked(true);  break;
      case 32: radioButtonFaktor32->setChecked(true);  break;
      case 43: radioButtonFaktor43->setChecked(true);  break;
      default: radioButtonFaktor1->setChecked(true);   break;
   }

   // ---------------------
   // input table view

   tableWidgetInputSignals->setRowCount(bitInputCount);
   tableWidgetInputSignals->setColumnCount(3);

   for (int i = 0; i < bitInputCount; ++i)
   {
      QTableWidgetItem* itemFunction = new QTableWidgetItem(inputFunctionName(i));
      itemFunction->setFlags(itemFunction->flags() & ~Qt::ItemIsEditable);
      tableWidgetInputSignals->setItem(i, 0, itemFunction);

      QTableWidgetItem* itemBit = new QTableWidgetItem(inputBits[i].bit == na ? "NA" : QString::number(inputBits[i].bit));
      tableWidgetInputSignals->setItem(i, 1, itemBit);

      QTableWidgetItem* itemMode = new QTableWidgetItem(inputBits[i].mode == 0 ? "rising" : "falling");
      tableWidgetInputSignals->setItem(i, 2, itemMode);
   }

   QStringList labels;
   labels << "Funktion" << "Bit" << "Trigger-Flanke";

   tableWidgetInputSignals->setHorizontalHeaderLabels(labels);
   tableWidgetInputSignals->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   tableWidgetInputSignals->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
   tableWidgetInputSignals->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

/* QT4
   tableWidgetInputSignals->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
   tableWidgetInputSignals->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
   tableWidgetInputSignals->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed); */
   tableWidgetInputSignals->verticalHeader()->hide();

   // bit delegate

   ComboBoxDelegate* bitDelegate = new ComboBoxDelegate(this);
   bitDelegate->addItem("NA");

   for (int i = 0; i < 32; i++)
      bitDelegate->addItem(QString::number(i));

   tableWidgetInputSignals->setItemDelegateForColumn(1, bitDelegate);

   // mode delegate

   ComboBoxDelegate* modeDelegate = new ComboBoxDelegate(this);
   modeDelegate->addItem("rising");
   modeDelegate->addItem("falling");
   tableWidgetInputSignals->setItemDelegateForColumn(2, modeDelegate);

   connect(tableWidgetInputSignals, SIGNAL(cellChanged(int, int)),
           this, SLOT(inputSignalChanged(int, int)));

   // -----------------------
   // fill output table view

   tableWidgetOutputSignals->setRowCount(fctOutputCount);
   tableWidgetOutputSignals->setColumnCount(4);

   for (int i = 0; i < fctOutputCount; ++i)
   {
      QTableWidgetItem* itemFunction = new QTableWidgetItem(outputFunctionName(i));
      itemFunction->setFlags(itemFunction->flags() & ~Qt::ItemIsEditable);
      tableWidgetOutputSignals->setItem(i, 0, itemFunction);

      QTableWidgetItem* itemLed = new QTableWidgetItem(toName(outputBits[i].ledid));
      tableWidgetOutputSignals->setItem(i, 1, itemLed);

      QTableWidgetItem* itemBit = new QTableWidgetItem(outputBits[i].bit == na ? "NA" : QString::number(outputBits[i].bit));
      tableWidgetOutputSignals->setItem(i, 2, itemBit);

      QTableWidgetItem* itemMode = new QTableWidgetItem(toName(outputBits[i].mode));
      tableWidgetOutputSignals->setItem(i, 3, itemMode);
   }

   QStringList olabels;
   olabels << "Funktion" << "LED" << "Bit" << "Mode";

   tableWidgetOutputSignals->setHorizontalHeaderLabels(olabels);

   tableWidgetOutputSignals->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   tableWidgetOutputSignals->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
   tableWidgetOutputSignals->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
   tableWidgetOutputSignals->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

   /* QT4
   tableWidgetOutputSignals->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
   tableWidgetOutputSignals->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
   tableWidgetOutputSignals->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
   tableWidgetOutputSignals->horizontalHeader()->setResizeMode(3, QHeaderView::Fixed); */
   tableWidgetOutputSignals->verticalHeader()->hide();

   // led delegate

   ComboBoxDelegate* ledDelegate = new ComboBoxDelegate(this);

   for (int i = 0; i < ledCount; i++)
      ledDelegate->addItem(toName((LedId)leds[i].id));

   tableWidgetOutputSignals->setItemDelegateForColumn(1, ledDelegate);

   // mode delegate

   ComboBoxDelegate* omodeDelegate = new ComboBoxDelegate(this);

   for (int mode = 0; mode < omCount; mode++)
      omodeDelegate->addItem(toName((OutputMode)mode));

   tableWidgetOutputSignals->setItemDelegateForColumn(3, omodeDelegate);

   // bit delegate

   ComboBoxDelegate* obitDelegate = new ComboBoxDelegate(this);
   obitDelegate->addItem("NA");

   for (int i = 0; i < 32; i++)
      obitDelegate->addItem(QString::number(i));

   tableWidgetOutputSignals->setItemDelegateForColumn(2, obitDelegate);

   // connect

   connect(tableWidgetOutputSignals, SIGNAL(cellChanged(int, int)),
           this, SLOT(outputSignalChanged(int, int)));

   // -----------------------------
   // fill sound signal table view

   tableWidgetSound->setRowCount(sfCount);
   tableWidgetSound->setColumnCount(2);

   for (int i = 0; i < sfCount; ++i)
   {
      QTableWidgetItem* itemFunction = new QTableWidgetItem(sounds[i].name);
      itemFunction->setFlags(itemFunction->flags() & ~Qt::ItemIsEditable);
      tableWidgetSound->setItem(i, 0, itemFunction);

      QTableWidgetItem* itemSound = new QTableWidgetItem(sounds[i].sound);
      tableWidgetSound->setItem(i, 1, itemSound);
   }

   QStringList slabels;
   slabels << "Funktion" << "Sound";

   tableWidgetSound->setHorizontalHeaderLabels(slabels);
   tableWidgetSound->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   tableWidgetSound->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
   /* QT4
   tableWidgetSound->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
   tableWidgetSound->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed); */
   tableWidgetSound->verticalHeader()->hide();

   // sound delegate

   ComboBoxDelegate* soundDelegate = new ComboBoxDelegate(this);
   soundDelegate->addItem("no");

   QDir dir(QString(resourcePath) + "/sound", "*.wav", QDir::Name, QDir::Files);
   QFileInfoList list = dir.entryInfoList();

   for (int i = 0; i < list.size(); ++i)
      soundDelegate->addItem(list.at(i).fileName());

   tableWidgetSound->setItemDelegateForColumn(1, soundDelegate);

   connect(tableWidgetSound, SIGNAL(cellChanged(int, int)),
           this, SLOT(soundSignalChanged(int, int)));

   // -----------------------------
   // fill analog input table view

   tableWidgetAnalogInputs->setRowCount(fctAnalogCount);
   tableWidgetAnalogInputs->setColumnCount(2);

   for (int i = 0; i < fctAnalogCount; ++i)
   {
      QTableWidgetItem* itemFunction = new QTableWidgetItem(analogInFunctionName(i));
      itemFunction->setFlags(itemFunction->flags() & ~Qt::ItemIsEditable);
      tableWidgetAnalogInputs->setItem(i, 0, itemFunction);

      QTableWidgetItem* itemBit = new QTableWidgetItem(analogBits[i].bit == na ? "NA" : QString::number(analogBits[i].bit));
      tableWidgetAnalogInputs->setItem(i, 1, itemBit);
   }

   QStringList alabels;
   alabels << "Funktion" << "Bit";

   tableWidgetAnalogInputs->setHorizontalHeaderLabels(olabels);
   tableWidgetAnalogInputs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   tableWidgetAnalogInputs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
   /* QT4
   tableWidgetAnalogInputs->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
   tableWidgetAnalogInputs->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed); */
   tableWidgetAnalogInputs->verticalHeader()->hide();

   ComboBoxDelegate* abitDelegate = new ComboBoxDelegate(this);
   abitDelegate->addItem("NA");
   for (int i = 0; i < 6; i++)
      abitDelegate->addItem(QString::number(i));
   tableWidgetAnalogInputs->setItemDelegateForColumn(1, abitDelegate);

   connect(tableWidgetAnalogInputs, SIGNAL(cellChanged(int, int)),
           this, SLOT(analogInputChanged(int, int)));
}

SetupDialog::~SetupDialog()
{
   tell(eloAlways, "save settings");

   settings->beginGroup("common");

   settings->setValue("maxTimeTraining", maxTimeTraining);
   settings->setValue("maxTimeLapRace", maxTimeLapRace);
   settings->setValue("speedFactor", speedFactor);
   settings->setValue("slotLength", slotLength);
   settings->setValue("lapCount", lapCountRace);
   settings->setValue("lapCountTraining", lapCountTraining);
   settings->setValue("usbDevice", usbDevice);
   settings->setValue("alsaDevice", alsaDevice);
   settings->setValue("course", courseName);
   settings->setValue("databaseName", databaseName);
   settings->setValue("resourcePath", resourcePath);
   settings->setValue("abortAtJumpTheGun", abortAtJumpTheGun);
   settings->setValue("penaltyAtJumpTheGun", penaltyAtJumpTheGun);
   settings->setValue("fuelMax", fuelMax);
   settings->setValue("fuelPerLap", fuelPerLap);
   settings->setValue("fuelingDelay", fuelingDelay);
   settings->setValue("fuelingPerSecond", fuelingPerSecond);
   settings->setValue("fuelFactorFastLap", fuelFactorFastLap);
   settings->setValue("fuelFactorSlowLap", fuelFactorSlowLap);
   settings->setValue("averageLap", averageLap);
   settings->setValue("fuelingActive", fuelingActive);
   settings->setValue("fuelPenaltyTime", fuelPenaltyTime);
   settings->setValue("withSpiExtension", withSpiExtension);
   settings->setValue("driverImageMode", driverImageMode);
   settings->setValue("animationInterval", animationInterval);

   settings->endGroup();

   // fahrer

   settings->beginGroup("driver");
   settings->beginWriteArray("driver");

   for (int i = 0; i < listWidgetDriver->count(); ++i)
   {
      QString name = listWidgetDriver->item(i)->text();
      settings->setArrayIndex(i);
      settings->setValue("name", name);

      if (driverImages.contains(name))
         settings->setValue("image", driverImages[name]);
   }

   settings->endArray();
   settings->endGroup();

   // fahrzeuge

   settings->beginGroup("cars");
   settings->beginWriteArray("car");

   for (int i = 0; i < listWidgetCar->count(); ++i)
   {
      QString name = listWidgetCar->item(i)->text();
      settings->setArrayIndex(i);
      settings->setValue("name", name);

      if (carImages.contains(name))
         settings->setValue("image", carImages[name]);
   }

   settings->endArray();
   settings->endGroup();

   // input signals

   settings->beginGroup("inputSignals");
   settings->beginWriteArray("inputSignals");

   for (int i = 0; i < bitInputCount; ++i)
   {
      settings->setArrayIndex(i);

      settings->setValue("bit", inputBits[i].bit);
      settings->setValue("mode", inputBits[i].mode);
   }

   settings->endArray();
   settings->endGroup();

   // output signals

   settings->beginGroup("outputSignals");
   settings->beginWriteArray("outputSignals");

   for (int i = 0; i < fctOutputCount; ++i)
   {
      settings->setArrayIndex(i);

      settings->setValue("bit", outputBits[i].bit);
      settings->setValue("mode", outputBits[i].mode);
      settings->setValue("led", outputBits[i].ledid);
   }

   settings->endArray();
   settings->endGroup();

   // sound signals

   settings->beginGroup("soundSignals");
   settings->beginWriteArray("soundSignals");

   for (int i = 0; i < sfCount; ++i)
   {
      settings->setArrayIndex(i);

      settings->setValue("sound", sounds[i].sound);
   }

   settings->endArray();
   settings->endGroup();

   // anlaog inputs

   settings->beginGroup("analogInputs");
   settings->beginWriteArray("analogInputs");

   for (int i = 0; i < fctAnalogCount; ++i)
   {
      settings->setArrayIndex(i);

      settings->setValue("bit", analogBits[i].bit);
      // settings->setValue("mode", analogBits[i].mode);
   }

   settings->endArray();
   settings->endGroup();

   delete settings;
}

//***************************************************************************
// Reset
//***************************************************************************

void SetupDialog::reset()
{
   driverChanged = no;
   carChanged = no;
}

//***************************************************************************
// On Button Add Driver
//***************************************************************************

void SetupDialog::on_pushButtonAddDriver_clicked()
{
   if (!lineEditDriver->text().isEmpty())
   {
      QListWidgetItem* listItem;

      for (int i = 0; i < listWidgetDriver->count(); i++)
      {
         if ((listItem = listWidgetDriver->item(i)))
            if (listItem->text().compare(lineEditDriver->text()) == 0)
            {
               QMessageBox::information(this, "Info", "Der Fahrer ist bereits in der Liste enthalten!");
               return ;
            }
      }

      driverChanged = yes;
      listWidgetDriver->addItem(lineEditDriver->text());
      lineEditDriver->setText("");
      lineEditDriver->setFocus();
   }
}

//***************************************************************************
// On Button Add Car
//***************************************************************************

void SetupDialog::on_pushButtonAddCar_clicked()
{
   if (!lineEditCar->text().isEmpty())
   {
      QListWidgetItem* listItem;

      for (int i = 0; i < listWidgetCar->count(); i++)
      {
         if ((listItem = listWidgetCar->item(i)))
            if (listItem->text().compare(lineEditCar->text()) == 0)
            {
               QMessageBox::information(this, "Info", "Dar Fahrzeug ist bereits in der Liste enthalten!");
               return ;
            }
      }

      carChanged = yes;
      listWidgetCar->addItem(lineEditCar->text());
      lineEditCar->setText("");
      lineEditCar->setFocus();
   }
}

//***************************************************************************
// On Button Add Driver Image
//***************************************************************************

void SetupDialog::on_pushButtonAddImage_clicked()
{
   int row = listWidgetDriver->currentRow();

   if (row >= 0)
   {
      QString fileName = QFileDialog::getOpenFileName(this, "Bild zuordnen",
                                              "", "Image Files (*.png *.jpg *.bmp)");

      if (!fileName.size() || !QFile::exists(fileName))
         return ;

      QPixmap pixmap(fileName);
      labelDriverImage->setPixmap(pixmap.scaled(labelDriverImage->width(),
                                                labelDriverImage->height(),
                                                Qt::KeepAspectRatio));

      driverImages[listWidgetDriver->item(row)->text()] = fileName;
   }
}

void SetupDialog::on_pushButtonRemoveDriverImage_clicked()
{
   driverImages.remove(listWidgetDriver->item(listWidgetDriver->currentRow())->text());
   labelDriverImage->clear();

}

void SetupDialog::on_pushButtonRemoveCarImage_clicked()
{
   carImages.remove(listWidgetCar->item(listWidgetCar->currentRow())->text());
   labelCarImage->clear();

}

//***************************************************************************
// On Button Add Car Image
//***************************************************************************

void SetupDialog::on_pushButtonAddCarImage_clicked()
{
   int row = listWidgetCar->currentRow();

   if (row >= 0)
   {
      QString fileName = QFileDialog::getOpenFileName(this, "Bild zuordnen",
                                              "", "Image Files (*.png *.jpg *.bmp)");

      if (!fileName.size() || !QFile::exists(fileName))
         return ;

      QPixmap pixmap(fileName);
      labelCarImage->setPixmap(pixmap.scaled(labelCarImage->width(),
                                                labelCarImage->height(),
                                                Qt::KeepAspectRatio));

      carImages[listWidgetCar->item(row)->text()] = fileName;
   }
}

//***************************************************************************
//
//***************************************************************************

void SetupDialog::on_listWidgetDriver_currentItemChanged(QListWidgetItem* current, QListWidgetItem* )
{
   if (driverImages.contains(current->text()) && QFile::exists(driverImages[current->text()]))
   {
      QPixmap pixmap(driverImages[current->text()]);
      labelDriverImage->setPixmap(pixmap.scaled(labelDriverImage->width(),
                                                labelDriverImage->height(),
                                                Qt::KeepAspectRatio));
   }
   else
      labelDriverImage->clear();
}

//***************************************************************************
//
//***************************************************************************

void SetupDialog::on_listWidgetCar_currentItemChanged(QListWidgetItem* current, QListWidgetItem* )
{
   if (!current)
      return;

   if (carImages.contains(current->text()) && QFile::exists(carImages[current->text()]))
   {
      QPixmap pixmap(carImages[current->text()]);
      labelCarImage->setPixmap(pixmap.scaled(labelCarImage->width(),
                                                labelCarImage->height(),
                                                Qt::KeepAspectRatio));
   }
   else
      labelCarImage->clear();
}

//***************************************************************************
// On Button Del Driver
//***************************************************************************

void SetupDialog::on_pushButtonDelDriver_clicked()
{
   if (listWidgetDriver->currentRow() >= 0)
   {
      driverChanged = yes;
      delete listWidgetDriver->takeItem(listWidgetDriver->currentRow());
   }
}

//***************************************************************************
// On Button Del car
//***************************************************************************

void SetupDialog::on_pushButtonDelCar_clicked()
{
   if (listWidgetCar->currentRow() >= 0)
   {
      carChanged = yes;
      delete listWidgetCar->takeItem(listWidgetCar->currentRow());
   }
}

//***************************************************************************
// On Lap Limit Change
//***************************************************************************

void SetupDialog::on_spinBoxTrainingLapLimit_valueChanged(int value)
{
   lapCountTraining = value;
}

//***************************************************************************
// On Lap Count Change
//***************************************************************************

void SetupDialog::on_spinBoxLapCount_valueChanged(int value)
{
   lapCountRace = value;
}

//***************************************************************************
// On Slot Lenght Change
//***************************************************************************

void SetupDialog::on_doubleSpinBoxSlotLength_valueChanged(double value)
{
   slotLength = value;
}

//***************************************************************************
// On Usb Device Editing Finished
//***************************************************************************

void SetupDialog::on_comboBoxDevice_currentIndexChanged(const QString text)
{
   strcpy(usbDevice, text.toLatin1());
   tell(eloAlways, "set device to '%s'", usbDevice);
}

void SetupDialog::on_comboBoxDevice_editTextChanged(const QString text)
{
   strcpy(usbDevice, text.toLatin1());
   tell(eloAlways, "set device to '%s'", usbDevice);
}

//***************************************************************************
// On Alsa Device Changed
//***************************************************************************

void SetupDialog::on_comboBoxAlsaDevice_currentIndexChanged(const QString text)
{
#ifndef Q_OS_WIN32
   int i = 0;

   while (const QAlsaSound::Device* d = QAlsaSound::getDevice(i++))
   {
      if (d->desc.indexOf(text) == -1)
         continue;

      if (d->name.indexOf("default:") == -1)
         continue;

      tell(eloDebug2, "using sound device '%s'",
           d->name.toLatin1().constData());

      strcpy(alsaDevice, d->name.toLatin1());

      break;
   }
#endif
}

//***************************************************************************
// On Course Name Editing Finished
//***************************************************************************

void SetupDialog::on_lineEditCourse_editingFinished()
{
   strcpy(courseName, lineEditCourse->text().toLatin1());
}

//***************************************************************************
// On Database Name Editing Finished
//***************************************************************************

void SetupDialog::on_lineEditDbName_editingFinished()
{
   strcpy(databaseName, lineEditDbName->text().toLatin1());
}

//***************************************************************************
// On Resource Path Editing Finished
//***************************************************************************

void SetupDialog::on_lineEditResourcePath_editingFinished()
{
   strcpy(resourcePath, lineEditResourcePath->text().toLatin1());
}

//***************************************************************************
// On Lap Race Editing Finished
//***************************************************************************

void SetupDialog::on_timeEditLapRace_editingFinished()
{
   maxTimeLapRace =
      timeEditLapRace->time().hour()*3600
      + timeEditLapRace->time().minute()*60
      + timeEditLapRace->time().second();
}

//***************************************************************************
// On Training Editing Finished
//***************************************************************************

void SetupDialog::on_timeEditTraining_editingFinished()
{
   maxTimeTraining =
      timeEditTraining->time().hour()*3600
      + timeEditTraining->time().minute()*60
      + timeEditTraining->time().second();
}

//***************************************************************************
// On Tab Widget Changed
//***************************************************************************

void SetupDialog::on_tabWidget_currentChanged(int tab)
{
   if (tab == 1)
      lineEditDriver->setFocus();
}

//***************************************************************************
// Speed Factor
//***************************************************************************

void SetupDialog::on_radioButtonFaktor1_toggled(bool checked)
{
   if (checked) speedFactor = 1;
}

void SetupDialog::on_radioButtonFaktor24_toggled(bool checked)
{
   if (checked) speedFactor = 24;
}

void SetupDialog::on_radioButtonFaktor32_toggled(bool checked)
{
   if (checked) speedFactor = 32;
}

void SetupDialog::on_radioButtonFaktor43_toggled(bool checked)
{
   if (checked) speedFactor = 43;
}

//***************************************************************************
// Jump The Gun
//***************************************************************************

void SetupDialog::on_radioButtonJumpTheGunAbort_toggled(bool checked)
{
   if (checked) abortAtJumpTheGun = true;
}

//***************************************************************************
//
//***************************************************************************

void SetupDialog::on_radioButtonJumpTheGunPenalty_toggled(bool checked)
{
   if (checked) abortAtJumpTheGun = false;
}

void SetupDialog::on_spinBoxJumpTheGunPenaltyTime_valueChanged(int value)
{
   penaltyAtJumpTheGun = value;
}

void SetupDialog::on_doubleSpinBoxFuelMax_valueChanged(double value)
{
   fuelMax = value;
}

void SetupDialog::on_doubleSpinBoxFuelPerLap_valueChanged(double value)
{
   fuelPerLap = value;
}

void SetupDialog::on_spinBoxFuelingDelay_valueChanged(int value)
{
   fuelingDelay = value;
}

void SetupDialog::on_spinBoxFuelingPerSecond_valueChanged(int value)
{
   fuelingPerSecond = value;
}

void SetupDialog::on_spinBoxFuelFactorFastLap_valueChanged(int value)
{
   fuelFactorFastLap = value;
}

void SetupDialog::on_spinBoxFuelFactorSlowLap_valueChanged(int value)
{
   fuelFactorSlowLap = value;
}

void SetupDialog::on_doubleSpinBoxAverageLap_valueChanged(double value)
{
   averageLap = value;
}

void SetupDialog::on_checkBoxFueling_stateChanged(int state)
{
   fuelingActive = state;
   groupBoxFueling->setEnabled(state);
}

void SetupDialog::on_checkBoxWithSpiExtension_stateChanged(int state)
{
   withSpiExtension = state;
}

void SetupDialog::on_spinBoxFuelPenaltyTime_valueChanged(int value)
{
   fuelPenaltyTime = value;
}

void SetupDialog::inputSignalChanged(int row, int col)
{
   QString value = tableWidgetInputSignals->item(row, col)->data(Qt::DisplayRole).toString();

   if (col == 1)
   {
      if (value == "NA")
         inputBits[row].bit = na;
      else
         inputBits[row].bit = value.toInt();
   }
   else if (col == 2)
   {
      if (value == "rising")
         inputBits[row].mode = teRising;
      else if (value == "falling")
         inputBits[row].mode = teFalling;
   }
}

void SetupDialog::soundSignalChanged(int row, int col)
{
   QString value = tableWidgetSound->item(row, col)->data(Qt::DisplayRole).toString();

   if (col == 1)
      sounds[row].sound = value;
}

void SetupDialog::on_tableWidgetSound_cellDoubleClicked(int row, int column)
{
   if (column == 1)
      playSound(QString(resourcePath) + QString("/sound/") +  sounds[row].sound);
}

void SetupDialog::outputSignalChanged(int row, int col)
{
   QString value = tableWidgetOutputSignals->item(row, col)->data(Qt::DisplayRole).toString();

   if (col == 1)
   {
      outputBits[row].ledid = toLedId(value.toLatin1());
   }
   else if (col == 2)
   {
      if (value == "NA")
         outputBits[row].bit = na;
      else
         outputBits[row].bit = value.toInt();
   }
   else if (col == 3)
   {
      outputBits[row].mode = toOutputMode(value.toLatin1());
   }
}

void SetupDialog::analogInputChanged(int row, int col)
{
   QString value = tableWidgetAnalogInputs->item(row, col)->data(Qt::DisplayRole).toString();

   if (col == 1)
   {
      if (value == "NA")
         analogBits[row].bit = na;
      else
         analogBits[row].bit = value.toInt();
   }
}

//***************************************************************************
//
//***************************************************************************

word SetupDialog::getInputMask()
{
   word mask = 0;

   for (int i = 0; i < bitInputCount; ++i)
   {
      if (inputBits[i].bit < 16)
         mask |= 1 << inputBits[i].bit;
   }

   return mask;
}

word SetupDialog::getOutputMask()
{
   word mask = 0;

   for (int i = 0; i < fctOutputCount; ++i)
   {
      if (outputBits[i].bit < 16)
         mask |= 1 << outputBits[i].bit;
   }

   return mask;
}

void SetupDialog::on_radioButtonDriverImage_toggled(bool checked)
{
   if (checked)
      driverImageMode = mdDriver;
}

void SetupDialog::on_radioButtonCarImage_toggled(bool checked)
{
   if (checked)
      driverImageMode = mdCar;
}

void SetupDialog::on_radioButtonAnimateImage_toggled(bool checked)
{
   if (checked)
      driverImageMode = mdAnimated;
}

void SetupDialog::on_spinBoxAnimationInterval_valueChanged(int value)
{
   animationInterval = value;
}
