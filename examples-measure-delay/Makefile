CXX=g++
CXXFLAGS=-O -Wall
LDLIBS=-lcsdr++ -lcsdrx


all: fm_receiver fm_receiver_tcp_source fm_receiver_sdrplay_source \
	fm_receiver_soapy_source usb_receiver

clean:
	rm -f fm_receiver fm_receiver_tcp_source fm_receiver_sdrplay_source \
	      fm_receiver_soapy_source usb_receiver
