
rm -rf *.o linslot comtest Makefile *~ rm moc_*.cpp ui_*.h release/ debug/ Makefile*
qmake
make -s

# g++ -ggdb -Wall -I. com.cc common.cc -o comtest
