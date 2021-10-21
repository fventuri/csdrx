CXX=g++
CXXFLAGS=-O -Wall
LDLIBS=-lcsdr++ -lcsdrx


all: fm_receiver usb_receiver

fm_receiver: fm_receiver.cpp pipeline.cpp

usb_receiver: usb_receiver.cpp pipeline.cpp

clean:
	rm -f fm_receiver usb_receiver
