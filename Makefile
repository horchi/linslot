#**************************************************************************
# Group Linslot / Linux - Slotrace Manager
# File Makefile
# Date 27.11.17 - Jörg Wendel
# This code is distributed under the terms and conditions of the
# GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
#***************************************************************************

RES_DEST   = /usr/local/share/linslot
BIN_DEST   = /usr/local/bin
TARGET     = linslot
QT_VERSION = qt5

# The Makefile name has to be identical to linslot.pro !

QTMAKEFILE = Makefile.qt

all:
	$(shell QT_SELECT=$(QT_VERSION) qmake)
	$(MAKE) --makefile=$(QTMAKEFILE)

clean:
	@-rm -f *.o core* *~
	@-rm -rf linslot comtest release/ debug/

dclean: distclean

distclean: clean
	@-rm -f $(QTMAKEFILE)
	@-rm -f moc_*.cpp ui_*.h

install:
	if ! test -d $(RES_DEST)/pixmap; then \
		mkdir -p $(RES_DEST)/pixmap; \
	fi
	if ! test -d $(RES_DEST)/sound; then \
		mkdir -p $(RES_DEST)/sound; \
	fi
	chmod -R a+rX $(RES_DEST);
	install --mode=644 -D pixmap/* $(RES_DEST)/pixmap/
	install --mode=644 -D sound/* $(RES_DEST)/sound/
	install --mode=755 -D $(TARGET) $(BIN_DEST)/
