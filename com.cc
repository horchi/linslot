//***************************************************************************
// File
// Date tt.mm.jj - #1
// 
//--------------------------------------------------------------------------
//
//***************************************************************************

//***************************************************************************
// HOETO build
//***************************************************************************

// g++ -ggdb -I. com.cc common.cc -o comtest

//***************************************************************************
// Includes
//***************************************************************************

#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.hpp"

//***************************************************************************
// Globals
//***************************************************************************

int fd = 0;

//***************************************************************************
// To Bin Str
//***************************************************************************

const char* toBinStr(unsigned int value, char* mask)
{
   const int bytes = 4;
   const int bits = bytes * 8;
   int bit = 1;

   for (int i = 0, b = 0; b < bits; i++, b++)
   {
      if (!(b % 8) && b)
      {
         mask[bits+bytes-i-2] = ':';
         i++;
      }
      
      mask[bits+bytes-i-2] = (value & bit) ? '1' : '0';     
      bit = bit << 1;
   }

   mask[bits+bytes-1] = 0;

   return mask;
}

//***************************************************************************
// Read Command
//***************************************************************************

int readCmd(byte* line, int& size)
{
   int count = 0;
   int res;

   // for protocol

   line[0] = PROTOCOL;
   count++;
   
   while ((res = ::read(fd, line+1, 1)) <= 0)
   {
      if (res < 0)
         return fail;
      
      usleep(100);
   }

   count++;

   switch (line[1])
   {
      case cDigitalIn: size = sizeof(DigitalInput); break;
      case cAnalogIn:  size = sizeof(AnalogInput);  break;
      case cBoardTime: size = sizeof(DigitalInput); break;
      default: return fail;
   }

   tell(eloDebug, "Got command (0x%X) expected size is (%d)", line[1], size);

   while (count < size)
   {
      res = ::read(fd, line+count, size-count);

      tell(eloDebug2, "::read() result was (%d)", res);

      if (res <= 0)
         usleep(100);
      else
         count += res;
   }
      
   return line[1];
}

//***************************************************************************
// Look
//***************************************************************************

int look()
{
   byte b;

   if (::read(fd, &b, 1) <= 0)
      return fail;

   if (b != PROTOCOL)
   {
      tell(eloAlways, "Protocol violation, skipping byte [0x%x]!", b);
      return fail;
   }

   return success;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   struct termios oldtio, newtio;
   int isRunning = true;
   int command;
   byte line[100];
   int size;
   char tmp[1000];
   char buf[100];

   AnalogInput* analog;
   DigitalInput* digital;
   DigitalInput* boardTime;

   char* device = "/dev/ttyUSB0";

   if ((fd = open(device, O_RDWR | O_NOCTTY)) < 0)
   {
      perror(device); 
      return fail;
   }

   theEloquence = eloAlways; // eloDebug;

   // configure serial line

   tcgetattr(fd, &oldtio);
   bzero(&newtio, sizeof(newtio));
   newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
   // don't set ICRNL for iflag
   // don't set PARMRK iflag
   newtio.c_iflag = IGNPAR;
   newtio.c_oflag = 0;
   newtio.c_lflag = 0;
   tcflush(fd, TCIFLUSH);
   tcsetattr(fd, TCSANOW, &newtio);

   // first ask about the boardtime

   write(fd, cmdGetTime, strlen(cmdGetTime));
   tell(eloAlways, "-> [%s]", cmdGetTime);

   // set ghost car 

   if (argc >= 2 && argv[1][0] == 'r')
      sprintf(tmp, "%s:0=100;", cmdGhostCar);
   else
      sprintf(tmp, "%s:-1=100;", cmdGhostCar);

   write(fd, tmp, strlen(tmp));
   tell(eloAlways, "-> [%s]", tmp);

   // send to analog out

   if (argc >= 2 && argv[1][0] == 's')
   {
      char c[100];
      
      tell(eloAlways, "send mode");

      while (isRunning)
      {
         fgets(c, 80, stdin);

         sprintf(tmp, "AO%d=%d;", 5, atoi(c));
         write(fd, tmp, strlen(tmp));
         tell(eloAlways, "-> [%s]", tmp);
      }  

      tell(eloAlways, "END");

      isRunning = false;
      goto EXIT;
   }

   // receive loop

   while (isRunning)
   { 
      if (look() != success)
      {
         usleep(10000);
         tell(eloDebug2, "looping");

         continue;
      }

      if ((command = readCmd(line, size)) == fail)
      {
         tell(eloAlways, "readCmd() failed");
         continue;
      }

      switch (command)
      {
         case cDigitalIn: 
         {
            digital = (DigitalInput*)line; 

            toBinStr(digital->value, buf);
            tell(eloAlways, "<- DigitalIn: (%s) (0x%X) at (%u)", buf, digital->value, digital->time);

            break;
         }
         case cAnalogIn:
         {
            analog = (AnalogInput*)line;

            tell(eloAlways, "<- AnalogIn: (%dV/%dA)", analog->volt, analog->ampere);
            break;
         }
         case cBoardTime:
         {
            boardTime = (DigitalInput*)line; 

            tell(eloAlways, "<- BoardTime: (%dms)", boardTime->time);
            break;
         }
      }
   }      

 EXIT:

   // switch GC recording off

   sprintf(tmp, "%s:-1=100;", cmdGhostCar);
   write(fd, tmp, strlen(tmp));
   tell(eloAlways, "-> [%s]", tmp);

   // restore the old settings and close

   tcsetattr(fd, TCSANOW, &oldtio);
   ::close(fd);

   return 0;
}
