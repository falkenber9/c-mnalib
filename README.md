c-mnalib
========
<img src="logo.png" height="50" style="float: left;"/>
C-mnalib is a C library for easy access and configuration of mobile network modems, e.g., LTE modems.
The library focuses on providing access to network quality indicators and network parameters of the modems by using AT commands.

The project is currently in beta development stage. Please contact <robert.falkenberg@tu-dortmund.de> for further information about this library.

## Library Overview

### Supported Devices
* Sierra Wireless MC7455 (via AT commands) with GOBI drivers from the manufacturer.
* Sierra Wireless MC7565 (via AT commands) with GOBI drivers from the manufacturer.

### Dependencies
This is based on Archlinux packages. Other distributions may slightly differ.

#### Library:

* base-devel
    * cmake
    * pkg-config
    * gcc
    * binutils
    * ...
* glib2
* curl
* libcurl-compat
* libgudev


#### Scripts:

* autossh     (for automatic reverse tunnels)

### Conflicts
This software conflicts with the systemd-service ModemManager. ModemManager uses the AT command interface to configure the mobile broadband connection for the NetworkManager service and pipes the payload-data through it.
However, c-mnalib requires exclusive access to the AT command interface.
Check for ModemManager via
```
systemctl status ModemManager
```
and disable it
```
sudo systemctl stop ModemManager
sudo systemctl disable ModemManager
```


## Installation
Clone this respository in a working directory and build using cmake. Make sure you have installed the required dependencies.
```
git checkout <URL of this GIT repository>
cd c-mnalib
mkdir build
cd build
cmake ../
make
## Optional
sudo make install
```

### Compile with Debug Symbols (for Easier Core-dump Analysis):

Release mode optimized but adding debug symbols, useful for profiling :
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ...
```
Debug mode with NO optimization and adding debug symbols :
```
cmake -DCMAKE_BUILD_TYPE=Debug ...
```


## License
The software is released under MIT license.

## Acknowledgements
If you wish to acknowledge this library in a publication, please refer to:
```
@InProceedings{FalkenbergSliwaPiatkowskiEtal2018a,
	Author = {Robert Falkenberg and Benjamin Sliwa and Nico Piatkowski and Christian Wietfeld},
	Title = {Machine Learning Based Uplink Transmission Power Prediction for LTE and Upcoming 5G Networks using Passive Downlink Indicators},
	Booktitle = {2018 IEEE 88th IEEE Vehicular Technology Conference (VTC-Fall)},
	Year = {2018},
	Address = {Chicago, USA},
	Month = {Aug},
	Url = {https://arxiv.org/abs/1806.06620}
}
```

## Supplemental Notes

### Checking Mobile Quota
#### T-Mobile Germany
* Browser: http://datapass.de
* CURL: http://datapass.de/home?continue=true
* Via T-Mobile API (json results):
    * Source: http://www.b.shuttle.de/hayek/hayek/jochen/wp/blog-de/2016/04/08/wenn-einem-programmierer-beim-abruf-von-pass-telekom-de-langweilig-wird/
```
# curl --user-agent 'Mozilla/4.0' http://pass.telekom.de/api/service/generic/v1/status | jq .usedVolumeStr
"482,76_MB"
```

