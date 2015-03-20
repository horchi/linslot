//***************************************************************************
// Group SqlLite
// File sqlite.cc
// Date 20.12.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlite.hpp>

//***************************************************************************
// Object
//***************************************************************************

SqliteDb::SqliteDb(const char* name)
{ 
   firstFetch = true;
   db = 0;

   dbName = (char*)malloc(strlen(name)+1);
   sprintf(dbName, "%s", name);
}

SqliteDb::~SqliteDb()
{ 
   close();
   clearResults();
   
   if (dbName) free(dbName);
}

int SqliteDb::result(int status)
{
   if (status == SQLITE_DONE || status == SQLITE_ROW)
      return 0;

   return status;
}

//***************************************************************************
// Open/Close
//***************************************************************************

int SqliteDb::open()
{
   int rc;
   
   if ((rc = sqlite3_open(dbName, &db)) != SQLITE_OK)
   {
      close();

      return -1;
   }
   
   return 0;
}

int SqliteDb::close()
{
   if (db) 
      sqlite3_close(db);   

   return 0;
}

//***************************************************************************
// Clear Results
//***************************************************************************

void SqliteDb::clearResults()
{
   Result* r;
   Field* f;

   while ((r = getFirstResult()))
   {
      results.remove(r);

      while ((f = (Field*)r->fields.getFirst()))
      {
         r->fields.remove(f);

         free(f->name);
         free(f->value);

         delete f;
      }

      delete r;
   }
}

//***************************************************************************
// Get Field Of
//***************************************************************************

SqliteDb::Field* SqliteDb::getFieldOf(const char* name, Result* aResult)
{
   Field* f;

   if (!aResult)
      aResult = getFirstResult();

   if (!aResult)
      return 0;

   for (f = (Field*)aResult->fields.getFirst(); f; f = (Field*)aResult->fields.getNext())
   {
      if (strcasecmp(f->name, name) == 0)
         return f;
   }

   return 0;
}

//***************************************************************************
// Get Value Of
//***************************************************************************

const char* SqliteDb::getValueOf(const char* name, Result* aResult)
{
   Field* f = getFieldOf(name, aResult);

   if (f)
      return f->value;

   return 0;
}

//***************************************************************************
// Get Integer Value Of
//***************************************************************************

int SqliteDb::getIntValueOf(const char* name, Result* aResult)
{
   const char* v = getValueOf(name, aResult);

   if (v)
      return atoi(v);

   return 0;
}

//***************************************************************************
// Fetch
//***************************************************************************

int SqliteDb::fetch(void* obj, int count, char** value, char** name)
{
   if (((SqliteDb*)obj)->isFirstFetch())
      ((SqliteDb*)obj)->beforeFetch();
   
   ((SqliteDb*)obj)->atData(count, value, name);
   
   return 0;
}

//***************************************************************************
// Get Insert Row Id
//***************************************************************************

long long SqliteDb::getInsertRowId()
{
   return sqlite3_last_insert_rowid(db);
}

//***************************************************************************
// Before Fetch
//***************************************************************************

int SqliteDb::beforeFetch()
{
   clearResults();
   firstFetch = false;

   return 0;
}

//***************************************************************************
// At Data
//***************************************************************************

int SqliteDb::atData(int count, char** value, char** name)
{
   Result* r = new Result;
   Field* f;

   results.append(r);

   for (int i = 0; i < count; i++)
   {
      f = new Field;
      r->fields.append(f);

      f->name = (char*)malloc(strlen(name[i])+1);
      f->value = (char*)malloc(strlen(value[i] ? value[i] : "NULL") +1);

      sprintf(f->name, "%s", name[i]);
      sprintf(f->value, "%s", value[i] ? value[i] : "NULL");
   }

   return 0;
}

//***************************************************************************
// Execute
//***************************************************************************

int SqliteDb::execute(const char* sql)
{
   int status;
   char* errMsg = 0;

   firstFetch = true;

   // printf("::execute '%s'\n", sql);

   if ((status = sqlite3_exec(db, sql, fetch, this, &errMsg)) != SQLITE_OK)
   {
      if (status != SQLITE_DONE && status != SQLITE_ROW)
      {
         fprintf(stderr, "Error: statement '%s', result '%s' (%d)\n", 
                 sql, errMsg, status);
         
         sqlite3_free(errMsg);
      }
   }

   return status;
}

//***************************************************************************
// Prepare
//***************************************************************************

int SqliteDb::prepare(const char* sql, sqlite3_stmt* &sqlStatement)
{
   return result(sqlite3_prepare(db, sql, strlen(sql), &sqlStatement, NULL));
}

//***************************************************************************
// Finalize
//***************************************************************************

int SqliteDb::finalize(sqlite3_stmt* sqlStatement)
{
   return result(sqlite3_finalize(sqlStatement));
}

//***************************************************************************
// Step
//***************************************************************************

int SqliteDb::step(sqlite3_stmt* sqlStatement)
{
   return result(sqlite3_step(sqlStatement));
}

//***************************************************************************
// Steps
//***************************************************************************

int SqliteDb::steps(sqlite3_stmt* sqlStatement)
{
   const char* value;
   const char* name;
   Result* r;
   Field* f;
   int cols;

   clearResults();
   cols = sqlite3_column_count(sqlStatement);

   while (sqlite3_step(sqlStatement) == SQLITE_ROW)
   {
      r = new Result;
      results.append(r);
      
      for (int i = 0; i < cols; i++)
      {       
         name = sqlite3_column_name(sqlStatement, i);
         value = (char*)sqlite3_column_text(sqlStatement, i);

         if (!value || !name)
            continue;

         f = new Field;
         r->fields.append(f);

         f->name = (char*)malloc(strlen(name)+1);
         f->value = (char*)malloc(strlen(value ? value : "NULL") +1);

         sprintf(f->name, "%s", name);
         sprintf(f->value, "%s", value ? value : "NULL");
      }
   }

   return getResultCount() ? 0 : -1;
}

//***************************************************************************
// Reset
//***************************************************************************

int SqliteDb::reset(sqlite3_stmt* sqlStatement)
{
   return result(sqlite3_reset(sqlStatement));
}

//***************************************************************************
// Bindings
//***************************************************************************

int SqliteDb::bindText(sqlite3_stmt* sqlStatement, int index, const char* value)
{
   return result(sqlite3_bind_text(sqlStatement, index, value, strlen(value), SQLITE_STATIC));
}

int SqliteDb::bindInt(sqlite3_stmt* sqlStatement, int index, int value)
{
   return result(sqlite3_bind_int(sqlStatement, index, value));
}

int SqliteDb::bindDouble(sqlite3_stmt* sqlStatement, int index, double value)
{
   return result(sqlite3_bind_double(sqlStatement, index, value));
}

int SqliteDb::bindNull(sqlite3_stmt* sqlStatement, int index)
{
   return result(sqlite3_bind_null(sqlStatement, index));
}

//***************************************************************************
// Last Error
//***************************************************************************

const char* SqliteDb::lastError()
{
   static const char* msg = sqlite3_errmsg(db);

   return msg ? msg : "";
}

//***************************************************************************
// Run
//***************************************************************************

int SqliteDb::show()
{
   Result* r;
   Field* f;
   
   if ((r = (Result*)results.getFirst()))
   {
      for (f = (Field*)r->fields.getFirst(); f; f = (Field*)r->fields.getNext())
         printf("%s\t| ", f->name);
      
      printf("\n");
   }
   
   for (r = (Result*)results.getFirst(); r; r = (Result*)results.getNext())
   {
      for (f = (Field*)r->fields.getFirst(); f; f = (Field*)r->fields.getNext())
         printf("%s\t| ", f->value);
      
      printf("\n");
   }
   
   return 0;
}
