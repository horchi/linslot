
# if [ "$1" != "" ] ; then
#fi

qmake
make -s

sudo mkdir -p /usr/local/share/linslot/
sudo cp -a pixmap/ sound/ /usr/local/share/linslot/
sudo chmod -R a+rX /usr/local/share/linslot
sudo cp linslot /usr/local/bin
sudo chmod 755 /usr/local/bin/linslot


# g++ -ggdb -Wall -I. com.cc common.cc -o comtest
