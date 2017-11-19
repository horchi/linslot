//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File arduino.cc
// Date 16.11.07 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <qglobal.h>

#ifndef Q_OS_WIN32
#  include <unistd.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <arduino.hpp>

//***************************************************************************
// Object
//***************************************************************************

Arduino::Arduino()
   : IoInterface()
{
   fdDevice = 0;
   opened = no;
   readTimeout = 60;
   writeTimeout = 60;
   outValue = 0;
   messageSize = 0;
   *message = 0;
   tvNow(&tvBoardStartTime);

#ifndef Q_OS_WIN32
   bzero(&oldtio, sizeof(oldtio));
#endif
}

Arduino::~Arduino()
{
   close();
}

//***************************************************************************
// Settings
//***************************************************************************

int Arduino::setTimeout(unsigned int timeout)
{
   readTimeout = timeout / 1000;
   return success;
}

int Arduino::setWriteTimeout(unsigned int timeout)
{
   writeTimeout = timeout / 1000;
   return success;
}

//***************************************************************************
// Is Connected
//***************************************************************************

int Arduino::connected()
{
   int stat;

   if (ioctl(fdDevice, TIOCMGET, &stat) != 0)
   {
      // tell(eloAlways, "Warning: ioctl failed due to '%m'");
      return no;
   }

   if (stat & TIOCM_CD)
      return no;

   return yes;
}

//***************************************************************************
// Open Device
//***************************************************************************

int Arduino::open(const char* dev)
{
#ifndef Q_OS_WIN32
   struct termios newtio;
#endif

   if (!dev)
      dev = deviceName;
   else
      strcpy(deviceName, dev);

   if (!deviceName || !*deviceName)
      return fail;

   tell(eloAlways, "Try to open io device '%s'", deviceName);

   deviceMutex.lock();

#ifndef Q_OS_WIN32

	// open serial line with 8 data bits, no parity, 1 stop bit

   if ((fdDevice = ::open(deviceName, O_RDWR | O_NOCTTY)) < 0)
   {
      deviceMutex.unlock();
      fdDevice = 0;

      tell(eloAlways, "Error: Opening '%s' failed", deviceName);

      return fail;
   }

   // configure serial line

   tcgetattr(fdDevice, &oldtio);
   bzero(&newtio, sizeof(newtio));

   newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
   // don't set ICRNL for iflag!
   // don't set PARMRK iflag!
   newtio.c_iflag = IGNPAR;
   newtio.c_oflag = 0;
   newtio.c_lflag = 0;
   tcflush(fdDevice, TCIFLUSH);
   tcsetattr(fdDevice, TCSANOW, &newtio);

#else

   // now the real pain with the windows serial stuff

   COMMCONFIG config;
   COMMTIMEOUTS timeouts;
	DWORD flags = 0;
   unsigned long confSize = sizeof(COMMCONFIG);

   // flags += FILE_FLAG_OVERLAPPED; // wenn event-driven

   fdDevice = CreateFileA(deviceName, GENERIC_READ|GENERIC_WRITE,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, 0,
                          OPEN_EXISTING, flags, 0);

   if (fdDevice == INVALID_HANDLE_VALUE)
   {
      fdDevice = 0;
      deviceMutex.unlock();
      tell(eloAlways, "Opening serial port failed");
      return fail;
   }

   config.dwSize = confSize;

   GetCommConfig(fdDevice, &config, &confSize);
   GetCommState(fdDevice, &(config.dcb));

   config.dcb.fBinary = TRUE;
   config.dcb.fAbortOnError = FALSE;
   config.dcb.fNull = FALSE;

   // baud rate

   config.dcb.BaudRate = CBR_57600;

   // flow control off

   config.dcb.fOutxCtsFlow = FALSE;
   config.dcb.fRtsControl = RTS_CONTROL_DISABLE;
   config.dcb.fInX = FALSE;
   config.dcb.fOutX = FALSE;

   // parity

   config.dcb.Parity = 0;
   config.dcb.fParity = FALSE;

   // data bits, stop bits

   config.dcb.ByteSize = 8;
   config.dcb.StopBits = ONESTOPBIT;

   // timeouts

   // Setting 0 indicates that timeouts are not used
   // for read nor write operations;
   // however read() and write() functions will
   // still block. Set -1 to provide   // non-blocking behaviour (read() and write()
   // will return immediately).

   timeouts.ReadIntervalTimeout = 5;
   timeouts.ReadTotalTimeoutConstant = 5;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutMultiplier = -1;
	timeouts.WriteTotalTimeoutConstant = 0;

   // apply configuration

   SetCommConfig(fdDevice, &config, sizeof(COMMCONFIG));
   SetCommTimeouts(fdDevice, &timeouts);

#endif

   deviceMutex.unlock();
   setWriteTimeout(1000);
   setTimeout(1000);

   opened = yes;
   tell(eloAlways, "Serial port opened successfully, handle is (%d)", fdDevice);

   return success;
}

//***************************************************************************
// Close Device
//***************************************************************************

int Arduino::close()
{
   // restore the old settings and close

   if (fdDevice)
   {
      tell(eloDebug, "Debug: Closing connection");

      QMutexLocker locker(&deviceMutex);

#ifndef Q_OS_WIN32

      tcsetattr(fdDevice, TCSANOW, &oldtio);
      ::close(fdDevice);

#else

      CloseHandle(fdDevice);

#endif

      fdDevice = 0;
   }

   opened = no;

   return success;
}

//***************************************************************************
// Re Open
//***************************************************************************

int Arduino::reopen(const char* dev)
{
   close();

   return open(dev);
}

//***************************************************************************
// Flush
//***************************************************************************

int Arduino::flush()
{
   byte command;

   if (fdDevice)
   {

#ifdef Q_OS_WIN32

      FlushFileBuffers(fdDevice);

#endif

      while (look(command) == success)
         ;
   }

   return success;
}

//***************************************************************************
// Setup Io
//***************************************************************************

int Arduino::initIoSetup(word bitsInput, word bitsOutput, byte withSpi)
{
   char buf[50];
   char buf1[50];
   SetupIo sio;
   int state;

   flush();

   if ((state = initBoardTime()) != success)
      tell(eloAlways, "Initializing bord time failed!");

   recordGhostCar(na, na);                     // switch off ghostcar recording

   sio.bitsInput = bitsInput;
   sio.bitsOutput = bitsOutput;
   sio.withSpiExtension = withSpi;

   sendCommand(cSetupIo, &sio, sizeof(sio));

   tell(eloDebug, "Setup done with input mask '%s'; output mask '%s'",
        toBinStr(sio.bitsInput, buf, 16),
        toBinStr(sio.bitsOutput, buf1, 16));

   sendCommand(cGetInputs);

   tell(eloAlways, "Initializing io device %s", state == success ? "succeeded" : "failed");

   return state;
}

//***************************************************************************
// Init Board Time
//***************************************************************************

int Arduino::initBoardTime()
{
   byte command = cNone;
   int cnt = 0;

   // ask about the boardtime

   tell(eloAlways, "Requesting actual board time");

   while (command != cBoardTime && cnt < 5)
   {
      sendCommand(cGetTime);
      usleep(10000);

      time_t timeoutAt = time(0) + 3;

      while (look(command) != success)
      {
         if (time(0) > timeoutAt)
            break;

         usleep(10000);
      }

      if (command == cBoardTime)
      {
         unsigned int boardTime;
         DigitalInput* boardTm;
         boardTm = (DigitalInput*)message;

         tell(eloAlways, "<- BoardTime: (%ums)", boardTm->time);

         boardTime = boardTm->time;
         tvNow(&tvBoardStartTime);
         tvBoardStartTime = subMs2Tv(tvBoardStartTime, boardTime);
      }

      else if (command != cNone)
      {
         tell(eloDebug, "Debug: Got unexpected command %d, ignoring", command);
      }

      cnt++;
   }

   return command == cBoardTime ? success : fail;
}

//***************************************************************************
// Write IO Bit
//***************************************************************************

int Arduino::writeBit(int bit, int state)
{
   setBitTo(outValue, bit, state);

   if (!fdDevice || bit == na)
      return fail;

   tell(eloDebug, "Debug: Write bit (%d) value (%d)",
        bit, state);

   DigitalOutputBit digital;
   digital.bit = bit;
   digital.state = state;

   sendCommand(cDigitalOutBit, &digital, sizeof(DigitalOutputBit));

   return success;
}

//***************************************************************************
// Read Out Bit
//
//  - Return the actual Value of the given 'out' Bit,
//    using the cached value of last write!
//***************************************************************************

int Arduino::readOutBit(int bit)
{
   char buf[100+TB];

   tell(eloDebug2, "Debug: out value is '%s'", toBinStr(outValue, buf));

   return isBit(outValue, bit);
}

//***************************************************************************
// Analog Write
//***************************************************************************

void Arduino::analogWrite(int bit, byte value)
{
   if (fdDevice)
   {
      AnalogOutput analogOut;

      analogOut.bit = bit;
      analogOut.value = value;

      sendCommand(cAnalogOut, &analogOut, sizeof(AnalogOutput));
   }
}

//***************************************************************************
// Record Ghost Car
//***************************************************************************

void Arduino::recordGhostCar(char vBit, char iBit)
{
   if (fdDevice)
   {
      RecordGhostCar rgc;

      rgc.cycle = getGcScale();
      rgc.voltBit = vBit;
      rgc.ampereBit = iBit;

      sendCommand(cRecordGhostCar, &rgc, sizeof(RecordGhostCar));
   }
}

//***************************************************************************
// Start/Stop Ghost Car
//***************************************************************************

void Arduino::startGhostCar(int pwmBit, int iBit)
{
   if (fdDevice)
   {
      GhostCarStart gcs;

      gcs.cycle = getGcScale();
      gcs.bit = pwmBit;
      gcs.ampereBit = iBit;
      gcs.controlCycle = 4;        // TODO
      gcs.incFactor = 4;           // TODO
      gcs.decFactor = 1;           // TODO

      sendCommand(cStartGhostCar, &gcs, sizeof(GhostCarStart));
   }
}

void Arduino::stopGhostCar()
{
   if (fdDevice)
   {
      tell(eloDebug, "send cStopGhostCar");
      sendCommand(cStopGhostCar);
   }
}

//***************************************************************************
// Start Ghost Car
//***************************************************************************

void Arduino::writeGhostCarValue(byte volt, byte ampere)
{
   GhostCarValue value;

   value.volt = volt;
   value.ampere = ampere;

   sendCommand(cGhostCarValue, &value, sizeof(GhostCarValue));
}

//***************************************************************************
//
//***************************************************************************

void Arduino::flushGhostCar()
{
   sendCommand(cGhostCarFlush);
}

//***************************************************************************
// Read Value
//***************************************************************************

int Arduino::look(byte& command)
{
   byte cmd = cNone;

   *message = 0;
   messageSize = 0;
   command = cNone;

   if (!fdDevice)
      return fail;

   if (_look() != success)
      return ignore;           // no input data pending

   tell(eloDebug2, "Debug: look() succeeded, reading command");

   if ((cmd = receiveCommand()) == cNone)
      return fail;

   command = cmd;

   return success;
}

//***************************************************************************
// Send Command
//***************************************************************************

int Arduino::sendCommand(byte command, void* line, byte size)
{
   QMutexLocker locker(&deviceMutex);

   if (!fdDevice)
   {
      tell(eloAlways, "Warning io not opened, can't write command (0x%x)!", command);
      return done;
   }

   tell(eloDebug2, "-> (0x%x)", command);

   // byte cmd = protocol | (command & commandMask);

#ifndef Q_OS_WIN32

   ::write(fdDevice, &command, 1);
   ::write(fdDevice, &size, 1);

   if (line)
      ::write(fdDevice, line, size);

#else

	DWORD written = 0;

   WriteFile(fdDevice, &command, 1, &written, 0);
   WriteFile(fdDevice, &size, 1, &written, 0);

   if (line)
      WriteFile(fdDevice, line, size, &written, 0);

#endif

   return done;
}

//***************************************************************************
// Read
//***************************************************************************

int Arduino::read(void* buf, unsigned int count)
{
#ifndef Q_OS_WIN32

   return ::read(fdDevice, buf, count);

#else

   DWORD readCount = 0;
   DWORD errorMask = 0;
   COMSTAT comStat;

   if (!ClearCommError(fdDevice, &errorMask, &comStat))
      return fail;

   if (comStat.cbInQue <= 0)
      return 0;

   if (!ReadFile(fdDevice, buf, count, &readCount, 0))
   {
      tell(eloAlways, "Error: Read failed!");
      return fail;
   }

   if (readCount)
      tell(eloAlways, "got %d bytes", readCount);

   return readCount;

#endif
}

//***************************************************************************
// Look
//***************************************************************************

int Arduino::_look()
{
   byte b;

   QMutexLocker locker(&deviceMutex);

   if (read(&b, 1) <= 0)
      return fail;

//    if ((b & protocolMask) != protocol)
//    {
//       tell(eloAlways, "Info: Protocol violation, skipping byte [0x%x]!", b);
//       return fail;
//    }

   command = b; //  & 0x0F;

   return success;
}

//***************************************************************************
// Read Command
//***************************************************************************

byte Arduino::receiveCommand()
{
   int count = 0;
   int res;

   QMutexLocker locker(&deviceMutex);

   // get size

   while ((res = read(&messageSize, 1)) <= 0)
   {
      if (res < 0)
      {
         tell(eloAlways, "Error: read failed!");
         return cNone;
      }

      usleep(100);
   }

   tell(eloDebug, "Debug: Got command (0x%X) expected size is (%d)",
        command, messageSize);

   // get message

   while (count < messageSize)
   {
      res = read(message+count, messageSize-count);

      tell(eloDebug2, "Debug: read() result was (%d)", res);

      if (res <= 0)
         usleep(100);
      else
         count += res;
   }

//    for (int i = 0; i < count; i++)
//       tell(eloDebug2, "Debug: got byte (0x%d)", message[i]);

   tell(eloDebug2, "Debug: read() (%d) bytes", count);

   return command;
}
