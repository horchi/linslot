//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iow.cc
// Date 05.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <unistd.h>
#endif

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/ioctl.h>

#include <iow.hpp>

//***************************************************************************
// Object
//***************************************************************************

Iow::Iow()
   : IoInterface()
{
   numIoWarriors = 0;
   devHandle = 0;
 
   out[0] = 0xFF;
   out[1] = 0xFF;
}

//***************************************************************************
// Clear
//***************************************************************************

void Iow::clear()
{
   unsigned int i;

   for (i = 0; i < numIoWarriors; i++)
   {
      if (IoWarriors[i].fdIO != -1)
         ::close(IoWarriors[i].fdIO);
   }

   memset(IoWarriors, 0, sizeof(IoWarriors));
   numIoWarriors = 0;
}

//***************************************************************************
// Get Device By Handle
//***************************************************************************

Iow::IowDevice* Iow::GetDeviceByHandle(IowHandle iowHandle)
{
   unsigned int i;

   for (i = 0; i < numIoWarriors; i++)
      if ((IowDevice *) iowHandle == &IoWarriors[i])
         return &IoWarriors[i];

   return 0;
}

//***************************************************************************
// Open Device
//***************************************************************************

int Iow::open(const char* dev)
{
   char ifname[80];         // The filename for the io interface
   iowarrior_info info;     // the info for the device
   int i;
   unsigned int j;
   int fd;
   int ioctlret;

   clear();

   if (!dev)
      dev = deviceName;
   else
      strcpy(deviceName, dev);

   if (!deviceName || !*deviceName)
      return fail;

   // First step : open the interface 0 of all IOWarriors found

   for (i = 0; i < maxDevices*2; i++)
   {
      sprintf(ifname, "%s%d", deviceName, i);
      fd = ::open(ifname, O_RDWR);

      if (fd != -1)
      {
         // now get the info for this device

         ioctlret = ioctl(fd, IOW_GETINFO, &info);

         if (ioctlret != -1 && info.if_num == 0)
         {
            IoWarriors[numIoWarriors].dev_num = i;
            IoWarriors[numIoWarriors].info = info;
            IoWarriors[numIoWarriors].fdIO = fd;
            IoWarriors[numIoWarriors].lastValue = 0xFFFFFFFF;
            IoWarriors[numIoWarriors].readTimeoutVal = 0xFFFFFFFF;
            IoWarriors[numIoWarriors].writeTimeoutVal = 0xFFFFFFFF;
            numIoWarriors++;
         }
         else
            ::close(fd);
      }
   }

   // now look for interface1

   for (i = 0; i < maxDevices*2; i++)
   {
      sprintf(ifname, "%s%d", deviceName, i);
      fd = ::open(ifname, O_RDWR);

      if (fd != -1)
      {
         // now get the info for this device

         ioctlret = ioctl(fd, IOW_GETINFO, &info);

         if (ioctlret != -1 && info.if_num == 1)
         {
            for (j = 0; j < numIoWarriors; j++)
            {
               if (IoWarriors[j].info.product == info.product)
               {
                  if (info.serial[0] == '\0' ?
                     i - 1 == IoWarriors[j].dev_num : strcmp(IoWarriors[j].info.serial, info.serial) == 0)
                  {
                     fd = -1;
                     break;
                  }
               }
            }
         }
      }

      if (fd != -1)
         ::close(fd);
   }

   devHandle = GetDeviceHandle(1);

   // Init all inputs  to 1
   //      all outputs to last value!

   if (devHandle)
   {
      // check number of IO-Warrier in system
      
      if (!getNumDevs())
      {
         tell(eloAlways, "No IO device found, aborting!");
         return fail;
      }
      
      tell(eloDetail, "%d IO/Warrior in system", getNumDevs());

      writeByte(0xFF, 0);       // Inputs always FF
      writeByte(0x00, 1);       // Outputs to 00
   }

   return devHandle == 0 ? fail : success;
}

//***************************************************************************
// Re Open
//***************************************************************************

int Iow::reopen(const char* dev)
{
   if (isOpen())
      close();

   if (open(dev) != success)
   {
      tell(eloAlways, "Failed to open device '%s<N>'", notNull(deviceName));
      return fail;
   }

   writeByte(0xFF, 0);       // Inputs always FF
   writeByte(out[1], 1);     // last output value

   return success;
}

//***************************************************************************
// Flush Input Stack
//***************************************************************************

int Iow::flush()
{
   unsigned int tmp;

   if (!isOpen())
      return fail;

   // flush input stack

   while (ReadImmediate(&tmp))
      ;

   return done;
}

//***************************************************************************
// Get Device Handle
//***************************************************************************

Iow::IowHandle Iow::GetDeviceHandle(unsigned int numDevice)
{
   if (numDevice >= 1 && numDevice <= numIoWarriors)
      return (IowHandle) &IoWarriors[numDevice - 1];
   else
      return 0;
}

//***************************************************************************
// Close Device
//***************************************************************************

int Iow::close()
{ 
   if (devHandle)
   {
      // on close set all I/O ports back to 1

      writeByte(0xFF, 0);
      writeByte(0xFF, 1);
   }

   devHandle = 0;
   clear();

   return done;
}

//***************************************************************************
// Set Timeout
//***************************************************************************

int Iow::setTimeout(unsigned int timeout)
{
   IowDevice* dev;

   if (!isOpen())
      return fail;

   dev = GetDeviceByHandle(devHandle);

   if (dev)
   {
      dev->readTimeout.tv_sec = (time_t)(timeout / 1000);
      dev->readTimeout.tv_usec = (long) ((timeout % 1000) * 1000);
      dev->readTimeoutVal = timeout;
   }

   return dev != 0 ? success : fail;
}

//***************************************************************************
// Set Write Timeout
//***************************************************************************

int Iow::setWriteTimeout(unsigned int timeout)
{
   IowDevice* dev;

   if (!isOpen())
      return fail;

   dev = GetDeviceByHandle(devHandle);

   if (dev)
   {
      dev->writeTimeout.tv_sec = (time_t)(timeout / 1000);
      dev->writeTimeout.tv_usec = (long)((timeout % 1000) * 1000);
      dev->writeTimeoutVal = timeout;
   }

   return dev != 0 ? success : fail;
}

//***************************************************************************
// Write Bit
//***************************************************************************

int Iow::writeBit(int bit, int value)
{
   unsigned int byte = bit < 8 ? out[0] : out[1];

   tell(eloDebug, "Debug: Write bit (%d) value (%d)", bit, value);

   setBitTo(byte, bit < 8 ? bit : bit-8, value);

   return writeByte(byte, bit < 8 ? 0 : 1);
}

//***************************************************************************
// Read Out Bit
//***************************************************************************

int Iow::readOutBit(int bit)
{
   unsigned int byte = bit < 8 ? out[0] : out[1];
   char buf[100+TB];
   
   tell(eloDebug, "Debug: Out  byte is '%s'", toBinStr(byte, buf));

   return isBit(byte, bit);
}

//***************************************************************************
// Look Byte
//***************************************************************************

int Iow::lookByte(int byteNum, unsigned char* byte)
{
   IoValue value;

   // QMutexLocker locker(&mutex);

   if (ReadImmediate(&value.dword))
   {
      *byte = value.bytes[byteNum];

      return success;
   }

   return fail;
}

//***************************************************************************
// Read
//***************************************************************************

int Iow::readValue(unsigned int* value)
{
   Iow::Report24 rep;
   int ret;

   ret = Read((char*)&rep, sizeof(Iow::Report24));

   if (ret == sizeof(Iow::Report24))
   {
      *value = rep.Value;
      
      return cmdDigitalInput;
   }

   else if (ret < 0)
   {
      reopen();
   }

   return fail;
}

//***************************************************************************
// Write
//***************************************************************************

int Iow::writeByte(unsigned char byte, int byteNum)
{
   Iow::Report rep;
   char buf[8+1];

   // Init report

   rep.ReportID = 0;
   rep.Value = 0xFFFF;
   rep.Bytes[byteNum] = byte;
   out[byteNum] = byte;

   unsigned char old;

   tell(eloDebug, "Debug: Writing to byte %d", byteNum);

   if (byteNum == 0)
   {
      if (lookByte(1, &old) == success)
         rep.Bytes[1] = old;

      out[1] = old;
   }
   else
   {
      if (lookByte(0, &old) == success)
         rep.Bytes[0] = old;

      out[0] = old;
   }

   // QMutexLocker locker(&mutex);
   
   tell(eloDebug, "Debug: Writing '%s' to port %d", toBinStr(rep.Bytes[0], buf), 0);
   tell(eloDebug, "Debug: Writing '%s' to port %d", toBinStr(rep.Bytes[1], buf), 1);
   
   return Write((char*)&rep, sizeof(Iow::Report)) == sizeof(Iow::Report);
}

//***************************************************************************
// Read
//***************************************************************************

int Iow::Read(char* buffer, int length)
{
   IowDevice *dev;
   timeval tv;
   fd_set rfds;
   int fd;
   int count;
   int result, size, extra;

   if (!isOpen())
      return fail;

   dev = GetDeviceByHandle(devHandle);

   if (!dev || !buffer)
      return -1;

   fd = dev->fdIO;
   size = dev->info.packet_size;
   extra = 1;
      
   for (count = 0; count + size + extra <= length; )
   {
      tv = dev->readTimeout;
      FD_ZERO(&rfds);      // clear the read-descriptor for select
      FD_SET(fd, &rfds);   // add the filedescriptor to read-descriptor

      if (dev->readTimeoutVal != 0xFFFFFFFF)
         result = select(fd + 1, &rfds, 0, 0, &tv);
      else
         result = select(fd + 1, &rfds, 0, 0, 0);

      if (result > 0 && FD_ISSET(fd, &rfds))
      {
         int res;

         *buffer = '\0';
         buffer += extra;

         // there should be some data ready for reading now

         res = read(fd, buffer, size);

         if (res < 0)
         {
            printf("Error: Read failed, system error was '%s' (%d)\n", 
                   strerror(errno), errno);

            return -1;
         }
         else if (res != size)
            break;

         buffer += size;
         count += size + extra;
      }

      else
      {
         // if we encountered a timeout then do not try to read several reports

         if (!result)
            break;
      }
   }

   return count;
}

//***************************************************************************
// Read Immediate
//***************************************************************************

bool Iow::ReadImmediate(unsigned int* value)
{
   IowDevice *dev;
   timeval tv;
   fd_set rfds;
   int result;
   Report report;

   if (!isOpen())
      return fail;

   dev = GetDeviceByHandle(devHandle);

   if (dev)
   {
      if (!value)
         return fail;

      tv.tv_sec = 0;
      tv.tv_usec = 0;
      FD_ZERO(&rfds);            // clear the read-descriptor for select
      FD_SET(dev->fdIO, &rfds);  // add the filedescriptor to read-descriptor
      result = select(dev->fdIO + 1, &rfds, 0, 0, &tv);

      if (result > 0 && FD_ISSET(dev->fdIO, &rfds))
      {
         memset(&report, 0, sizeof(report));

         // there should be some data ready for reading now

         if (read(dev->fdIO, &report.Value, dev->info.packet_size) == (int)dev->info.packet_size)
         {
            dev->lastValue = report.Value;
            *value = report.Value;

            return success;
         }
      }
   }

   *value = dev->lastValue;

   return fail;
}

//***************************************************************************
// Write
//***************************************************************************

int Iow::Write(char* buffer, int length)
{
   IowDevice *dev;
   int fd;
   int count;
   int siz, extra;

   if (!isOpen())
      return fail;

   dev = GetDeviceByHandle(devHandle);

   if (!buffer)
      return 0;

   if (!dev)
      return fail;

   fd = dev->fdIO;
   siz = dev->info.packet_size;
   extra = 1;
   
   for (count = 0; count + siz + extra <= length; count += siz + extra)
   {
      buffer += extra;
      
      if (ioctl(fd, IOW_WRITE, buffer) != 0)
         break;
      
      buffer += siz;
   }
   
   return count;
}
