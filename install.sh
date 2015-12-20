#!/bin/sh

mkdir build
cd build
cmake ../
make
sudo make install
cd ../fcm
python2 setup.py build
sudo python2 setup.py install

