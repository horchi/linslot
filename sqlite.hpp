//***************************************************************************
// Group SqlLite
// File sqlite.cc
// Date 20.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#ifndef _SQLITE_H_
#define _SQLITE_H_

//***************************************************************************
// Include
//***************************************************************************

#include <sqlite3.h>

#include <common.hpp>
#include <list.hpp>

//***************************************************************************
// Class SqliteDb
//***************************************************************************

class SqliteDb
{
   public:

      typedef sqlite3_stmt Statement;

      struct Field
      {
         char* name;
         char* value;
      };

      struct Result
      {
         List fields;
         int getFieldCount()   { return fields.getCount(); }
      };

      SqliteDb(const char* name);
      ~SqliteDb();

      int open();
      int close();
      int show();
      int atData(int count, char** value, char** name);
      int beforeFetch();

      int execute(const char* sql);
      int prepare(const char* sql, sqlite3_stmt* &sqlStatement);
      int finalize(sqlite3_stmt* sqlStatement);
      int step(sqlite3_stmt* sqlStatement);
      int steps(sqlite3_stmt* sqlStatement);
      int reset(sqlite3_stmt* sqlStatement);

      int bindText(sqlite3_stmt* sqlStatement, int index, const char* value);
      int bindInt(sqlite3_stmt* sqlStatement, int index, int value);
      int bindDouble(sqlite3_stmt* sqlStatement, int index, double value);
      int bindNull(sqlite3_stmt* sqlStatement, int index);

      void clearResults();
      Result* getFirstResult()   { return (Result*)results.getFirst(); }
      Result* getNextResult()    { return (Result*)results.getNext(); }
      int getResultCount()       { return results.getCount(); }

      Field* getFieldOf(const char* name, Result* aResult = 0);
      const char* getValueOf(const char* name, Result* aResult = 0);
      int getIntValueOf(const char* name, Result* aResult = 0);
      long long getInsertRowId();

      int isFirstFetch()         { return firstFetch; }

      // lib status

      const char* lastError();

   protected:

      int result(int status);
      static int fetch(void* obj, int count, char** value, char** name);

      // data

      int firstFetch;
      List results;
      char* dbName;
      sqlite3* db;
};

//***************************************************************************
#endif // _SQLITE_H_
