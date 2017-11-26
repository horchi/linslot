
# if [ "$1" != "" ] ; then
#fi

export QT_SELECT=qt5

qmake
make -s

sudo mkdir -p /usr/local/share/linslot/
sudo cp -a pixmap/ sound/ /usr/local/share/linslot/
sudo chmod -R a+rX /usr/local/share/linslot
sudo cp linslot /usr/local/bin
sudo chmod 755 /usr/local/bin/linslot


# g++ -ggdb -Wall -I. com.cc common.cc -o comtest
