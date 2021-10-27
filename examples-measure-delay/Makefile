CXX=g++
CXXFLAGS=-O -Wall
LDLIBS=-lcsdr++ -lcsdrx


all: fm_receiver fm_receiver_tcp_source fm_receiver_sdrplay_source fm_receiver_soapy_source usb_receiver

fm_receiver: fm_receiver.cpp pipeline.cpp

fm_receiver_tcp_source: fm_receiver_tcp_source.cpp pipeline.cpp

fm_receiver_sdrplay_source: fm_receiver_sdrplay_source.cpp pipeline.cpp

fm_receiver_soapy_source: fm_receiver_soapy_source.cpp pipeline.cpp

usb_receiver: usb_receiver.cpp pipeline.cpp

clean:
	rm -f fm_receiver fm_receiver_tcp_source fm_receiver_sdrplay_source fm_receiver_soapy_source usb_receiver
