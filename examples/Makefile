CXX=g++
CXXFLAGS=-O -Wall
#CXXFLAGS=-g -Wall
LDLIBS=-lcsdr++ -lcsdrx


all: fm_receiver_file_source fm_receiver_sdrplay_source \
	fm_receiver_soapy_source fm_receiver_user_input \
	dmr_receiver dstar_receiver fm_receiver_using_replace_module \
	nbfm_receiver_stdout usb_receiver ysf_receiver \
	nbfm_receiver_sdrplay_source usb_receiver_sdrplay_source

dstar_receiver: dstar_receiver.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -ldigiham -o $@

dmr_receiver: dmr_receiver.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -ldigiham -o $@

ysf_receiver: ysf_receiver.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -ldigiham -o $@

clean:
	rm -f fm_receiver_file_source fm_receiver_sdrplay_source \
	      fm_receiver_soapy_source fm_receiver_user_input \
	      dmr_receiver dstar_receiver fm_receiver_using_replace_module \
	      nbfm_receiver_stdout usb_receiver ysf_receiver \
	      nbfm_receiver_sdrplay_source usb_receiver_sdrplay_source
