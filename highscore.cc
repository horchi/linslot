//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File highscore.cc
// Date 23.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <stdio.h>

#include <common.hpp>
#include <highscore.hpp>

//***************************************************************************
// Object
//***************************************************************************

HighscoreDialog::HighscoreDialog()
{
   db = 0;

   setupUi(this);

   tableWidgetRaces->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidgetRaces->setEditTriggers(QAbstractItemView::NoEditTriggers);
   tableWidgetLaps->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidgetLaps->setEditTriggers(QAbstractItemView::NoEditTriggers);
   tableWidgetBestOf->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidgetBestOf->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

HighscoreDialog::~HighscoreDialog()
{

}

//***************************************************************************
//
//***************************************************************************

void HighscoreDialog::on_tableWidgetRaces_currentCellChanged(int currentRow, int /*currentColumn*/,
                                                             int previousRow, int /*previousColumn*/)
{
   if (currentRow != previousRow && currentRow >= 0)
      fillLaps(tableWidgetRaces->item(currentRow, 0)->type());
}


//***************************************************************************
// Fill Widgets
//***************************************************************************

int HighscoreDialog::fill()
{
   fillRaces();
   fillBestOf();

   return 0;
}

int HighscoreDialog::fillBestOf()
{
   db->execute("SELECT "                                                \
               "r.RACE_ID, "                                            \
               "d.NAME AS Fahrer, "                                     \
               "l.LAP_TIME AS Zeit, "                                   \
               "r.DATE AS Datum, "                                      \
               "r.LAP_LENGTH AS Länge, "                                \
               "r.COURSE AS Strecke "                                   \
               "from races AS r, "                                      \
               "laps AS l, "                                            \
               "drivers AS d "                                          \
               "WHERE l.RACE_ID=r.RACE_ID and "                         \
               "d.DRIVER_ID=l.DRIVER_NR and "                           \
               "l.LAP_TIME IS NOT NULL "                                \
               "ORDER BY l.LAP_TIME;");

   fillTableWidget(tableWidgetBestOf, db, 1);

   return 0;
}

int HighscoreDialog::fillRaces()
{
   db->execute("SELECT "                 \
               "RACE_ID, "               \
               "DATE AS Datum, "         \
               "LAPS AS Runden, "        \
               "LAP_LENGTH AS Länge, "   \
               "COURSE AS Strecke "      \
               "from races ORDER BY DATE DESC;");

   fillTableWidget(tableWidgetRaces, db, 1);

   return 0;
}

int HighscoreDialog::fillLaps(int raceId)
{
   sqlite3_stmt* sqlSelectLaps;
   char sql[1000];
   char driver1[100];
   char driver2[100];

   // get driver names

   sprintf(sql, "SELECT d.NAME from drivers AS d, races AS r where d.DRIVER_ID=r.DRIVER1 and r.RACE_ID=%d;", raceId);
   db->execute(sql);
   // free(sql);
   strcpy(driver1, db->getValueOf("NAME"));

   sprintf(sql, "SELECT d.NAME from drivers AS d, races AS r where d.DRIVER_ID=r.DRIVER2 and r.RACE_ID=%d;", raceId);
   db->execute(sql);
   // free(sql);
   strcpy(driver2, db->getValueOf("NAME"));

   // build statement

   sprintf(sql, "SELECT "                                             \
           "a.lap_nr AS Runde, "                                        \
           "a.lap_time AS '%s', "                                       \
           "b.lap_time AS '%s' "                                        \
           "from laps AS a INNER JOIN laps AS b ON "                    \
           "a.race_id=b.race_id and "                                   \
           "a.lap_nr=b.lap_nr and "                                     \
           "b.race_id=? and "                                           \
           "a.driver_nr=1 and "                                         \
           "b.driver_nr=2 ORDER BY a.LAP_NR;",
           driver1, driver2);

   // execute statement

   db->prepare(sql, sqlSelectLaps);
   db->bindInt(sqlSelectLaps, 1, raceId);
   db->steps(sqlSelectLaps);
   db->reset(sqlSelectLaps);
   db->finalize(sqlSelectLaps);
   // free(sql);

   // and fill table widget

   fillTableWidget(tableWidgetLaps, db);

   return 0;
}

int HighscoreDialog::fillTableWidget(QTableWidget* widget, SqliteDb* db, int startCol)
{
   SqliteDb::Result* r;
   SqliteDb::Field* f;
   QStringList header;

   int col;
   int row = 0;
   int id = 0;

   if (!db)
      return -1;

   widget->clear();
   widget->setRowCount(db->getResultCount());

   if (!db->getResultCount())
      return 0;

   // header

   if ((r = db->getFirstResult()))
   {
      if (!(r->getFieldCount()-startCol))
         return 0;

      widget->setColumnCount(r->getFieldCount()-startCol);
      col = 0;

      for (f = (SqliteDb::Field*)r->fields.getFirst(); f; f = (SqliteDb::Field*)r->fields.getNext())
      {
         if (col >= startCol)
            header << f->name;   // #TODO UTF8

         col++;
      }
   }

   widget->setHorizontalHeaderLabels(header);
   widget->horizontalHeaderItem(1)->font().setBold(true);

   // values

   for (r = db->getFirstResult(); r; r = db->getNextResult())
   {
      col = 0;

      for (f = (SqliteDb::Field*)r->fields.getFirst(); f;
           f = (SqliteDb::Field*)r->fields.getNext())
      {
         if (!col)
            id = atoi(f->value);

         if (col >= startCol)
            widget->setItem(row, col-startCol, new QTableWidgetItem(f->value, id));

         col++;
      }

      row++;
   }

   return 0;
}
