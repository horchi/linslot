//***************************************************************************
// Group Lin-Slot / Race Manager
// File highscore.hpp
// Date 23.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _HIGHSCORE_H_
#define _HIGHSCORE_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <QDialog>

#include <ui_highscore.h>

#include <sqlite.hpp>

//***************************************************************************
// Class LinslotWindow
//***************************************************************************

class HighscoreDialog : public QDialog, public Ui::HighscoreDialog
{
      Q_OBJECT
      
   public:
      
      // object
      
      HighscoreDialog();
      ~HighscoreDialog();


      void setDb(SqliteDb* aDb) { db = aDb; }
      int fill();
      int fillBestOf();
      int fillRaces();
      int fillLaps(int raceId);
      int fillTableWidget(QTableWidget* widget, SqliteDb* db, int startCol = 0);

   protected:

      // data

      SqliteDb* db;

   private slots:

      void on_tableWidgetRaces_currentCellChanged(int currentRow, int currentColumn, 
                                                  int previousRow, int previousColumn);
      
};

//***************************************************************************
#endif // _HIGHSCORE_H_
