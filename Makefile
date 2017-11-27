#**************************************************************************
# Group Linslot / Linux - Slotrace Manager
# File Makefile
# Date 27.11.17 - Jörg Wendel
# This code is distributed under the terms and conditions of the
# GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
#***************************************************************************

# The Makefile name has tu be identical to linslot.pro !

QTMAKEFILE = Makefile.qt

all:
	qmake
	$(MAKE) --makefile=$(QTMAKEFILE)

clean:
	@-rm -f *.o core* *~
	@-rm -rf linslot comtest release/ debug/

distclean: clean
	@-rm -f $(QTMAKEFILE)
	@-rm -f moc_*.cpp ui_*.h
