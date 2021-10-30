# csdr extensions

Some extensions for [csdr](https://github.com/jketterl/csdr):
  - FileSource: a csdr source that reads from a file, device, pipeline (default: stdin)
  - Pipeline: a quick and easy way to create a receiver using the modules from csdr/csdrx as building blocks; see examples
  - PulseAudioWriter: a csdr writer that sends audio output directly to PulseAudio
  - SDRplaySource: a csdr source that reads I/Q samples from an SDRplay RSP device using SDRplay API directly
  - SoapySource: a csdr source that reads I/Q samples from an SDR using the SoapySDR driver [SoapySDR](https://github.com/pothosware/SoapySDR/wiki)


To build and install all the components:
```
mkdir build
cd build
cmake ..
make
sudo make install
```


To build and install only selected components (for instance 'filesource' - multiple components are separated by ';'):
```
mkdir build
cd build
cmake -DCOMPONENTS=filesource ..
make
sudo make install
```

## Examples

  - FM BC receiver: [fm_receiver_file_source.cpp](examples/fm_receiver_file_source.cpp)
  - FM BC receiver using SDRplay source: [fm_receiver_sdrplay_source.cpp](examples/fm_receiver_sdrplay_source.cpp)
  - FM BC receiver using SoapySDR source: [fm_receiver_soapy_source.cpp](examples/fm_receiver_soapy_source.cpp)
  - USB receiver: [usb_receiver.cpp](examples/usb_receiver.cpp)
  - USB receiver using SDRplay source: [usb_receiver.cpp](examples/usb_receiver_sdrplay_source.cpp)
  - NBFM receiver: [nbfm_receiver_stdout.cpp](examples/nbfm_receiver_stdout.cpp)
  - NBFM receiver using SDRplay source: [nbfm_receiver_stdout.cpp](examples/nbfm_receiver_sdrplay_source.cpp)
  - D-Star receiver: [dstar_receiver.cpp](examples/dstar_receiver.cpp)
  - DMR receiver: [dmr_receiver.cpp](examples/dmr_receiver.cpp)
  - YSF receiver: [ysf_receiver.cpp](examples/ysf_receiver.cpp)

To run the FM BC receiver example reading the I/Q stream from the 'rx_sdr' command from [rx_tools](https://github.com/rxseger/rx_tools):
```
rx_sdr -d driver=sdrplay -s 2000000 -f 90400000 -a 'Antenna C' -F CF32 - | ./fm_receiver_file_source
```

To run the D-Star receiver example:
```
rx_sdr -d driver=sdrplay -s 2000000 -f 146500000 -a 'Antenna C' -F CF32 - | ./dstar_receiver 
```

To run the DMR receiver example:
```
rx_sdr -d driver=sdrplay -s 1000000 -f 441900000 -a 'Antenna C' -F CF32 - | ./dmr_receiver 
```

To run the YSF receiver example:
```
rx_sdr -d driver=sdrplay -s 2000000 -f 444900000 -a 'Antenna C' -F CF32 - | ./ysf_receiver 
```

Since the SDRplay and SoapySDR sources connect directly to the SDRs, they are run as follows:
```
./fm_receiver_sdrplay_source
```
```
./fm_receiver_soapy_source
```


## Credits

Many thanks to Franco Spinelli, IW2DHW for testing csdrx, his very useful suggestions and insights, and for providing some of the examples in this repository.


## License

GPLv3
