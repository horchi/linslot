#//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File  main.cpp
// Date 26.01.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Programm part for Arduino-Diecimila
//***************************************************************************

//***************************************************************************
// Notice
//***************************************************************************

/*
    int      - 2 bytes
    long     - 4 bytes
    pointer  - 2 bytes
*/

//***************************************************************************
// Declarations
//***************************************************************************

typedef unsigned int word;
typedef unsigned long dword;

enum Misc
{
   fail          = -1,
   success       = 0,
   na            = -1,
   TB            = 1,
   yes           = 1,
   no            = 0,

   maxCmd        = 20,
   cmdEnd        = ';',
   sizeGcBuffer  = 100,
   maxInputCache = 30,
   bounceTime    = 200
};

//***************************************************************************
// Includes
//***************************************************************************

#include <WProgram.h>
#include <EEPROM/EEPROM.h>
#include "../ioservice.hpp"

//***************************************************************************
// Settings
//***************************************************************************

#define GHOSTCAR                    // comment out if no GHOST-CAR used 

//***************************************************************************
// Declarations
//***************************************************************************

enum IoDefinition
{
   firstExtendedOut   = 16,

   // the SPI bus

   bitShiftRegSlave   = 10,
   bitShiftRegDataOut = 11,
   bitShiftRegDataIn  = 12,
   bitShiftRegClock   = 13,

   // the bytes in the EEPROM

   eepWithSpiExtension = 1,
};

enum GhostCarMode
{
   gcmOff,
   gcmRecord,
   gcmReplay
};

// used for the cache

struct IoValue       // 9 bytes
{
   byte command;                 // { cDigitalIn | cAnalogIn }
   unsigned long value1;         // { time       | volt      }
   unsigned long value2;         // { value      | ampere    }
};

struct GcValue
{
   byte volt;
   byte ampere;
};

//***************************************************************************
// General
//***************************************************************************

//***************************************************************************
// Send Command
//   we have to send the data as binary, ascii is to big and 
//   so to slow for oure purpose
//***************************************************************************

void sendCommand(byte command, const byte* buf = 0, byte size = 0)
{  
   //serialWrite(Ios::protocol | (command & Ios::commandMask));
   serialWrite(command);
   serialWrite(size);

   for (int i = 0; i < size; i++)
      serialWrite(buf[i]);
}

//**************************************************************
// Set/Clear Bit
//**************************************************************

void setBit(byte bit, volatile unsigned int& value)
{
   value |= (1 << bit);
}

void clearBit(byte bit, volatile unsigned int& value)
{
   value &= ~(1 << bit);
}

//***************************************************************************
// Debug
//***************************************************************************

void debug(const char* msg, unsigned long value = 0)
{
   Ios::DebugValue data;

   strncpy(data.string, msg, 49);
   data.string[49] = 0;
   data.value = value;

   sendCommand(Ios::cDebug, (byte*)&data, sizeof(Ios::DebugValue));
}

void fatal(const char* msg)
{
   const char* fatal = "Fatal ";
   char buf[60+TB];

   strcpy(buf, fatal);
   strncpy(buf + strlen(fatal), fatal, 60-strlen(fatal));
   buf[60] = 0;

   debug(buf);
}

//***************************************************************************
// class FifoCache
//***************************************************************************

class FifoCache
{
   public:

      // declarations

      struct Node       // 4 + 9 bytes
      {
         Node* prev;
         IoValue data;
         Node* next;
      };

      FifoCache()   { first = last = 0; cnt = 0; }
      ~FifoCache()  { flush(); }

      // interface

      byte count()      { return cnt; }
      byte full()       { return cnt >= maxInputCache; }     // due to a max of 1kB SRAM :(
      void flush()      { while (cnt) pop(); }

      void push(unsigned long value1, unsigned long value2, byte command)
      {
         Node* n = (Node*)malloc(sizeof(Node));

         if (!n)
         {
            fatal("memory exceeded!");
            return ;
         }

         n->data.value1 = value1;
         n->data.value2 = value2;
         n->data.command = command;

         n->prev = last;
         n->next = 0;

         if (last)  last->next = n;
         last = n;
         if (!first) first = last;

         cnt++;
      }

      IoValue pop()
      { 
         IoValue v; 

         if (!first)
         {
            cnt = 0;
            v.value1 = 0;
            v.value2 = 0;
            return v;
         }

         v = first->data;

         Node* tmp = first;

         if (first->next)
         {
            first = first->next;
            first->prev = 0;
         }

         free(tmp);
         cnt--;

         return v; 
      }

   private:

      // data

      byte cnt;
      Node* first;
      Node* last;
};

//**************************************************************
// Globals
//**************************************************************

volatile byte callMeanwhile = false;
volatile unsigned int timerLoadValue = 0;

FifoCache inputCache = FifoCache();           // cache for digital input
unsigned long lastInputValue = 0xFFFFFFFF;    // remember last input states to avoid bouncing
unsigned long lastMsec;                       // remember last input time
unsigned int outputValue = 0;                 // value for output via shift-register

char ghostcarPinU = na;
char ghostcarPinI = na;
char gcScaleLoad = 100;
char gcControlScaleLoad = 10;
char gcPwmOut = na;
byte gcMode = gcmOff;
GcValue gcOutValues[sizeGcBuffer];            // 100 byte!
byte gcBufferTail = 0;
byte gcBufferHead = 0;
byte incFactor = 0;
byte decFactor = 5;
word bitsInput = 0;
word bitsOutput = 0;

//***************************************************************************
// Prototypes
//***************************************************************************

unsigned char setupTimer2();
void setupSpiBus();

//**************************************************************
// Setup
//**************************************************************

void setup()
{
   // initialize the serial interface

   Serial.begin(57600); // 19200, 38400, 57600);

   if (EEPROM.read(eepWithSpiExtension))
      setupSpiBus();

   // init interrupt stuff

   timerLoadValue = setupTimer2();     // set interrupt to 1kHz

   // init some variables 

   outputValue = 0;
}


//***************************************************************************
// Setup SPI Bus
//***************************************************************************

void setupSpiBus()
{
   byte clr;

   // setup SPI register control pins

   pinMode(bitShiftRegClock, OUTPUT);
   pinMode(bitShiftRegDataOut, OUTPUT);
   pinMode(bitShiftRegDataIn, INPUT);
   pinMode(bitShiftRegSlave, OUTPUT);
   digitalWrite(bitShiftRegSlave, HIGH);  // disable device

   // Now we set the SPI Control register (SPCR) to the binary value 01010000. 
   // In the control register each bit sets a different functionality. The eighth 
   // bit disables the SPI interrupt, the seventh bit enables the SPI, the sixth 
   // bit chooses transmission with the most significant bit going first, the fifth
   // bit puts the Arduino in Master mode, the fourth bit sets the data clock idle 
   // when it is low, the third bit sets the SPI to sample data on the rising edge
   // of the data clock, and the second and first bits set the speed of the SPI to 
   // system speed / 4 (the fastest). After setting our control register up we read 
   // the SPI status register (SPSR) and data register (SPDR) in to the junk clr 
   // variable to clear out any spurious data from past runs:

   SPCR = (1 << SPE) | (1 << MSTR);

   // SPI als Master, High-Bits zuerst, SCK ist HIGH wenn inaktiv 
   // SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL);
   // maximale Geschwindigkeit: F_CPU / 2 
   // SPSR |= (1 << SPI2X);

   clr = SPSR;
   clr = SPDR;
   delay(10);
}

//***************************************************************************
// Setup Timer 2
//    Configures the 8 Bit Timer2 to generate an interrupt
//    at the specified frequency.
//***************************************************************************

unsigned char setupTimer2()
{
   const double timeClockFreq = 250000.0;       // 2MHz for /64 prescale from 16MHz
   const double irqFreq = 1000.0;               // irq freq 1kHz

   byte result;                                 // The timer load value.

   // Calculate the timer load value
   // The 257 really should be 256 but i got better results with 257.

   result = (byte)((256.0-(timeClockFreq/irqFreq))+0.5);

   // Timer2 Settings: Timer Prescaler /8, mode 0
   // Timer clock = 16MHz/64 = 250kHz

   TCCR2A = 0;
   TCCR2B = 1<<CS22 | 0<<CS21 | 0<<CS20;  // prescale /64

   TIMSK2 = 1<<TOIE2;  // Timer2 Overflow Interrupt Enable
   TCNT2 = result;     // load the timer for its first cycle

   return result;
}

//***************************************************************************
// Timer2 overflow interrupt vector handler (called every 1ms)
//***************************************************************************

ISR(TIMER2_OVF_vect) 
{
   callMeanwhile = true;

   // Capture the current timer value. This is how much error we
   // have due to interrupt latency and the work in this function

   // int latency = TCNT2;

   // Reload the timer and correct for latency.

   TCNT2 = TCNT2 + timerLoadValue;
}

//***************************************************************************
// Meanwhile
//
//  meanwhile() have to be called in *each* programm loop to make shure the 
//    call cycle will be maintained !
//
//  meanwhile() is controlled by the 'callMeanwhile' flag set by the 
//    ISR (interrupt service routine), due to this it should be run every 1ms. 
//    The better way (for safer cycles) would be to put this stuff directly
//    into the ISR or call meanwhile from there. 
//    But it's not possible since serial data goes lost during the ISR :(
//***************************************************************************

void meanwhile()
{
   static unsigned long lastInputTimes[32];            // time for each bit used to avoid bouncing

   if (!callMeanwhile)
      return ;

   callMeanwhile = false;

   // digitalWrite(7, HIGH);                    // to messure the length

   lastMsec = millis();

#ifdef GHOSTCAR

   static char gcScale = 0;
   static char gcControlScale = 0;

   if (gcMode == gcmRecord)
   {
      if (!gcScale--)
      {
         byte volt;
         byte ampere = 0;
      
         volt = (analogRead(ghostcarPinU) * 255L) / 1024L;
      
         if (ghostcarPinI != na)
            ampere = analogRead(ghostcarPinI);
      
         if (!inputCache.full())
            inputCache.push(volt, ampere, Ios::cAnalogIn);
      
         gcScale = gcScaleLoad;
      }
   }
   else if (gcMode == gcmReplay)
   {
      static byte gcSollAmp = 0;
      static byte gcLastVolt = 0;
      static long pSoll = 0;

      if (!gcScale-- && gcBufferTail != gcBufferHead)
      {
         // write value only if not at end of list
         // last value will hold until sync signal 
         // detected by PC

         byte gcSollVolt = 0;

         gcSollAmp = gcOutValues[gcBufferTail].ampere;
         gcSollVolt = gcOutValues[gcBufferTail].volt;
         
         if (gcSollVolt)
            analogWrite(gcPwmOut, gcSollVolt);
         else
            digitalWrite(gcPwmOut, 0);

         gcLastVolt = gcSollVolt;
         pSoll = gcSollVolt * gcSollAmp;

         gcBufferTail = (gcBufferTail + 1) % sizeGcBuffer;

         gcScale = gcScaleLoad;
      }

      else if (gcControlScaleLoad != na && !gcControlScale--)
      {       
         int amp = analogRead(ghostcarPinI);
         // int diff = (int)gcSollAmp - amp;
         long pIst = gcLastVolt * amp;
         long vDiff = (pSoll - pIst) / amp;

         if (vDiff > 0)
            vDiff *= (int)incFactor;
         else
            vDiff *= (int)decFactor;

         // int v = (int)gcSollVolt + diff;
         int v = (int)gcLastVolt + vDiff;

         if (v < 0)
            v = 0;
         else if (v > 254)
            v = 254;

//          if (v < gcSollVolt-40 || v > gcSollVolt+40)
//             debug("v", diff);

         if (v)
            setBit(28-firstExtendedOut, outputValue);
         else
         {
            gcControlScaleLoad = 20;
            clearBit(28-firstExtendedOut, outputValue);
         }

         analogWrite(gcPwmOut, v);
         gcLastVolt = v;

         gcControlScale = gcControlScaleLoad;
      }
   }

#endif

   // check io state's

   unsigned long mask, b;
   unsigned long inputValue = 0;
   unsigned long value = lastInputValue;
   byte changes = 0;
   byte bit;
   
   // --------------------------------
   // Support Internal IO (byte 0+1)
   //   - start with 2, bit 0+1 used by RS-232 !

   for (char bit = 2; bit < 14; bit++)
   {
      if (bitsInput & (1 << bit))
         inputValue |= ((digitalRead(bit) ? 1 : 0) << bit);
   }

   if (EEPROM.read(eepWithSpiExtension))
   {
      // --------------------------------
      // Support External IO (byte 2+3)
  
      digitalWrite(bitShiftRegSlave, LOW);          // enable device

      // shift all bits in/out

      for (byte n = 0; n < 2; n++)
      {
         // start the transmission

         if (!n)
            SPDR = (outputValue >> 8);               // heigh byte
         else
            SPDR = outputValue & 0x00FF;             // low byte
         
         while (!(SPSR & (1 << SPIF)))               // wait the end of the transmission
            ;

         unsigned long v = SPDR;

         if (!n)
            inputValue |= (v << 24);                 // heigh byte
         else
            inputValue |= (v << 16);                 // low byte
      }

      digitalWrite(bitShiftRegSlave, HIGH);          // disable device
   }

   // auswerten

   mask = 1UL;

   for (bit = 0; bit < 32; bit++)
   {
      b = (inputValue & mask);

      if (((lastInputValue & mask) ^ b) && (lastMsec - lastInputTimes[bit] > bounceTime))
      {
         // bit changed -and- last change older than 'bounceTime'

         lastInputTimes[bit] = lastMsec;
         changes++;

         if (b)
            value |= mask;
         else
            value &= ~mask;
      }

      mask = mask << 1;
   }

   if (changes && !inputCache.full())
   {
      lastInputValue = value;
      inputCache.push(lastInputValue, lastMsec, Ios::cDigitalIn);
   }

   // digitalWrite(7, LOW);    // to messure the lenght
}

//**************************************************************
// Send Pending IO
//**************************************************************

void sendPengingIo()
{
   IoValue v;

   while (inputCache.count())
   {
      meanwhile();

      v = inputCache.pop();

      if (v.command == Ios::cDigitalIn)
      {
         Ios::DigitalInput data;
         
         data.value = v.value1;
         data.time = v.value2;

         sendCommand(Ios::cDigitalIn, (byte*)&data, sizeof(Ios::DigitalInput));
      }
      else if (v.command == Ios::cAnalogIn)
      {
         Ios::AnalogInput data;

         data.volt = v.value1;
         data.ampere = v.value2;

         sendCommand(Ios::cAnalogIn, (byte*)&data, sizeof(Ios::AnalogInput));
      }
   }
}

//**************************************************************
// Command GETTIME
//**************************************************************

void cmdGettime()
{
   Ios::DigitalInput data;
         
   data.value = 0;
   data.time = millis();
   
   sendCommand(Ios::cBoardTime, (byte*)&data, sizeof(Ios::DigitalInput));
}

//**************************************************************
// Command GETINPUTS
//**************************************************************

void cmdGetInputs()
{
   Ios::DigitalInput data;
         
   data.value = lastInputValue;
   data.time = lastMsec;
   
   sendCommand(Ios::cDigitalIn, (byte*)&data, sizeof(Ios::DigitalInput));
}

//**************************************************************
// Command DO
//**************************************************************

void cmdDigitalOutBit(const byte* buffer)
{
   Ios::DigitalOutputBit digOut;

   memcpy(&digOut, buffer, sizeof(Ios::DigitalOutputBit));

   if (EEPROM.read(eepWithSpiExtension))
   {
      if (digOut.bit >= firstExtendedOut)
      {
         digOut.bit -= firstExtendedOut;
         
         if (digOut.state)
            setBit(digOut.bit, outputValue);
         else
            clearBit(digOut.bit, outputValue);
      }
      else if (bitsOutput & (1 << digOut.bit))
         digitalWrite(digOut.bit, digOut.state);
   }
   else
   {
      if (bitsOutput & (1 << digOut.bit))
         digitalWrite(digOut.bit, digOut.state);
   }
}

//**************************************************************
// Command AO
//**************************************************************

void cmdAnalogOut(const byte* buffer)
{
   Ios::AnalogOutput analogOut;

   memcpy(&analogOut, buffer, sizeof(Ios::AnalogOutput));

   if (analogOut.value)
      analogWrite(analogOut.bit, analogOut.value);
   else
      digitalWrite(analogOut.bit, LOW);
}

//**************************************************************
// Command 'Record - Ghost Car'
//**************************************************************

void cmdRecordGhostCar(const byte* buffer)
{
   Ios::RecordGhostCar rgc;

   memcpy(&rgc, buffer, sizeof(Ios::RecordGhostCar));

   gcScaleLoad = rgc.cycle;     // should at least 100ms !

   ghostcarPinU = rgc.voltBit;
   ghostcarPinI = rgc.ampereBit;

   gcMode = ghostcarPinU != na ? gcmRecord : gcmOff;

   if (gcMode == gcmRecord)
      inputCache.flush();
}

//**************************************************************
// Command 'Start - Ghost Car'
//**************************************************************

void cmdStartGhostCar(const byte* buffer)
{
   Ios::GhostCarStart sgc;

   memcpy(&sgc, buffer, sizeof(Ios::GhostCarStart));

   gcMode = gcmReplay;

   gcScaleLoad = sgc.cycle;
   gcPwmOut = sgc.bit;
   ghostcarPinI = sgc.ampereBit;
   gcControlScaleLoad = sgc.controlCycle; 
   incFactor = sgc.incFactor;
   decFactor = sgc.decFactor;

   pinMode(gcPwmOut, OUTPUT);
}

//**************************************************************
// Command 'Stop - Ghost Car'
//**************************************************************

void cmdStopGhostCar()
{
   gcMode = gcmOff;
   digitalWrite(gcPwmOut, LOW);
}

//**************************************************************
// Command 
//**************************************************************

void cmdGhostCarValue(const byte* buffer)
{
	int i = (gcBufferHead + 1) % sizeGcBuffer;

	if (i != gcBufferTail) 
   {
      gcOutValues[gcBufferHead].volt   = ((Ios::GhostCarValue*)buffer)->volt;
      gcOutValues[gcBufferHead].ampere = ((Ios::GhostCarValue*)buffer)->ampere;

		gcBufferHead = i;
	}
   else
   {
      sendCommand(Ios::cGhostCarBufferFull);
   }
}

//***************************************************************************
// 
//***************************************************************************

void cmdGhostCarFlush()
{
   gcBufferTail = gcBufferHead = 0;
}

//***************************************************************************
// Command 'Setup IO'
//***************************************************************************

void cmdSetupIo(const byte* buffer)
{
   bitsInput = ((Ios::SetupIo*)buffer)->bitsInput;
   bitsOutput = ((Ios::SetupIo*)buffer)->bitsOutput;
   
   // bit 0,1 reserved for RS-232

   for (char bit = 2; bit < 14; bit++)
   {
      if (bitsInput & (1 << bit))
      {
         pinMode(bit, INPUT);
         digitalWrite(bit, HIGH);
      }

      else if (bitsOutput & (1 << bit))
      {
         pinMode(bit, OUTPUT);
         digitalWrite(bit, LOW);
      }
   }

   // debug("set input mask to", bitsInput);
   // debug("set output mask to", bitsOutput);

   byte withSpi = ((Ios::SetupIo*)buffer)->withSpiExtension;

   if (withSpi != EEPROM.read(eepWithSpiExtension))
   {
      EEPROM.write(eepWithSpiExtension, withSpi);
      debug("Wrote EEPROM!", withSpi);
   }
}

//**************************************************************
// Command Parser
//**************************************************************

char getCommand(const byte*& line)
{
   byte cmdLine[maxCmd+TB];
   byte size;
   byte cmd;
   int count = 0;
   int c;

   line = 0;
   cmd = serialRead();

//    if ((cmd & Ios::protocolMask) != Ios::protocol)
//       return fail;

//    cmd &= Ios::commandMask;

   while ((c = serialRead()) < 0)
      meanwhile();

   size = c;

   while (count < size && count < maxCmd)
   {
      meanwhile();

      if ((c = serialRead()) >= 0)
         cmdLine[count++] = c;
   }

   line = cmdLine;

   return cmd;
}

//**************************************************************
// Main Loop
//**************************************************************

void loop()
{
   const byte* line;

   // Command Dispatcher

   if (Serial.available())
   {
      char command = getCommand(line);

      switch (command)
      {
         case Ios::cGetTime:        cmdGettime();            break;
         case Ios::cGetInputs:      cmdGetInputs();          break;
         case Ios::cDigitalOutBit:  cmdDigitalOutBit(line);  break;
         case Ios::cAnalogOut:      cmdAnalogOut(line);      break;
         case Ios::cRecordGhostCar: cmdRecordGhostCar(line); break;
         case Ios::cStartGhostCar:  cmdStartGhostCar(line);  break;
         case Ios::cGhostCarValue:  cmdGhostCarValue(line);  break;
         case Ios::cStopGhostCar:   cmdStopGhostCar();       break;
         case Ios::cSetupIo:        cmdSetupIo(line);        break;
         case Ios::cGhostCarFlush:  cmdGhostCarFlush();      break;
      }
   }

   // Digital IO

   if (inputCache.count())
      sendPengingIo();
}

//***************************************************************************
// Main
//***************************************************************************

int main()
{
	init();
	setup();

	for (;;)
   {
      meanwhile();
		loop();
   }

	return 0;
}
