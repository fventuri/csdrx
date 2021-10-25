CXX=g++
#CXXFLAGS=-O -Wall
CXXFLAGS=-g -Wall
LDLIBS=-lcsdr++ -lcsdrx


all: fm_receiver_file_source fm_receiver_sdrplay_source

fm_receiver_file_source: fm_receiver_file_source.cpp pipeline.cpp

fm_receiver_sdrplay_source: fm_receiver_sdrplay_source.cpp pipeline.cpp

clean:
	rm -f fm_receiver_file_source fm_receiver_sdrplay_source
