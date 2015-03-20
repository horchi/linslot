# ***************************************************************************
#  Group Linslot / Linux - Slotrace Manager
#  File  Makefile
#  Date 26.01.08 - Jörg Wendel
#  This code is distributed under the terms and conditions of the
#  GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
# --------------------------------------------------------------------------
#  Mainly taken from example shipped with Arduino
# ***************************************************************************

# --------------------------------------------------------------------------
#  please define your system here
# --------------------------------------------------------------------------

PORT        = /dev/ttyUSB0
ARDUINOBASE = /data/devel/arduino-0010/hardware

# --------------------------------------------------------------------------
#  normally you don't need to touch ;)
# --------------------------------------------------------------------------

TARGET      = main
ARDUINO     = $(ARDUINOBASE)/cores/arduino
ARDUINOLIBS = $(ARDUINOBASE)/libraries

SRC     = $(ARDUINO)/pins_arduino.c $(ARDUINO)/wiring.c \
          $(ARDUINO)/wiring_analog.c $(ARDUINO)/wiring_digital.c \
          $(ARDUINO)/wiring_pulse.c $(ARDUINO)/wiring_serial.c \
          $(ARDUINO)/wiring_shift.c $(ARDUINO)/WInterrupts.c

CXXSRC  = $(ARDUINO)/HardwareSerial.cpp $(ARDUINO)/WRandom.cpp \
          $(ARDUINOLIBS)/EEPROM/EEPROM.cpp

MCU = atmega168
F_CPU = 16000000
FORMAT = ihex
UPLOAD_RATE = 19200

MAKEFILE = Makefile

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs

OPT = s

# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU) -DMKSKETCH
CXXDEFS = -DF_CPU=$(F_CPU)

# Place -I options here
CINCS = -I$(ARDUINO) -I$(ARDUINOLIBS)
CXXINCS = -I$(ARDUINO) -I$(ARDUINOLIBS)


# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions

# CSTANDARD = -std=gnu99
CDEBUG   = -g$(DEBUG)
CWARN    = -Wall # -Wstrict-prototypes
CTUNING  = -funsigned-char -funsigned-bitfields -fshort-enums
#CEXTRA  = -Wa,-adhlns=$(<:.c=.lst)
CPACK    = -fpack-struct
CFLAGS   = $(CPACK) $(CDEBUG) $(CDEFS) $(CINCS) -O$(OPT) $(CWARN) $(CSTANDARD) $(CEXTRA)
CXXFLAGS = $(CPACK) $(CDEFS) $(CINCS) -O$(OPT) 
#ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 
LDFLAGS  = -lm

# Programming support using avrdude. Settings and variables.
AVRDUDE_PROGRAMMER = stk500
AVRDUDE_PORT = $(PORT)
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_FLAGS = -F -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) \
  -b $(UPLOAD_RATE)

# Program settings
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AR  = avr-ar
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
MV = mv -f

# Define all object files.
OBJ = $(SRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(CXXSRC:.cpp=.lst) $(SRC:.c=.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_CXXFLAGS = -mmcu=$(MCU) -I. $(CXXFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)


# Default target.
all: build

build: elf hex

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss 
sym: $(TARGET).sym

# dependencies

$(TARGET).elf : $(TARGET).cpp
$(TARGET).hex : $(TARGET).cpp

# Program the device.  
upload: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)
xxx:
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)


# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
--change-section-address .data-0x800000 \
--change-section-address .bss-0x800000 \
--change-section-address .noinit-0x800000 \
--change-section-address .eeprom-0x810000 


coff: $(TARGET).elf
	$(COFFCONVERT) -O coff-avr $(TARGET).elf $(TARGET).cof


extcoff: $(TARGET).elf
	$(COFFCONVERT) -O coff-ext-avr $(TARGET).elf $(TARGET).cof


.SUFFIXES: .elf .hex .eep .lss .sym

.elf.hex:
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.eep:
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
	$(NM) -n $< > $@


core.a: $(OBJ)
	@for i in $(OBJ); do echo $(AR) rcs core.a $$i; $(AR) rcs core.a $$i; done

# Link: create ELF output file from library.
$(TARGET).elf: core.a
	@echo Compiling $(@)
	$(CXX) $(ALL_CFLAGS) -o $@ $(TARGET).cpp -L. core.a $(LDFLAGS)

# Compile: create object files from C++ source files.
.cpp.o:
	$(CXX) -c $(ALL_CXXFLAGS) $< -o $@ 

# Compile: create object files from C source files.
.c.o:
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 


# Compile: create assembler files from C source files.
.c.s:
	$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
.S.o:
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Target: clean project.
clean:
	@echo Cleanup!
	$(REMOVE) $(TARGET).hex $(TARGET).eep $(TARGET).cof $(TARGET).elf \
	$(TARGET).map $(TARGET).sym *.o $(TARGET).lss core.a


distclean:
	$(REMOVE) $(SRC:.c=.s) $(SRC:.c=.d) $(CXXSRC:.cpp=.s) \
	$(CXXSRC:.cpp=.d) $(OBJ) $(LST) *~

depend:
	if grep '^# DO NOT DELETE' $(MAKEFILE) >/dev/null; \
	then \
		sed -e '/^# DO NOT DELETE/,$$d' $(MAKEFILE) > \
			$(MAKEFILE).$$$$ && \
		$(MV) $(MAKEFILE).$$$$ $(MAKEFILE); \
	fi
	echo '# DO NOT DELETE THIS LINE -- make depend depends on it.' \
		>> $(MAKEFILE); \
	$(CC) -M -mmcu=$(MCU) $(CDEFS) $(CINCS) $(SRC) $(ASRC) >> $(MAKEFILE)

.PHONY:	all build elf hex eep lss sym program coff extcoff clean depend
