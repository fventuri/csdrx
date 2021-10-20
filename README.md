# csdr extensions

Some extensions for [csdr](https://github.com/jketterl/csdr):
  - FileSource
  - PulseAudioWriter

To build and install all the components:
```
mkdir build
cd build
cmake ..
make
sudo make install
```

To build and install all selected components (for instance 'filesource' - multiple components are separated by ';'):
```
mkdir build
cd build
cmake -DCOMPONENTS=filesource ..
make
sudo make install
```
