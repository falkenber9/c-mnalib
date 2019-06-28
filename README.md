C-mnalib
========
<img src="logo.png" height="50" style="float: left;"/>
C-mnalib is a C library for easy access and configuration of mobile network modems, e.g., LTE modems.
The library focuses on providing access to network quality indicators and network parameters of the modems by using AT commands.

The project is currently in beta development stage. Please contact <robert.falkenberg@tu-dortmund.de> for further information about this library.

## Library Overview

### Supported Devices
* Sierra Wireless (via AT commands) with GOBI drivers from the manufacturer.
    * mc7455/em7455
    * em7565

### Dependencies
This is based on Archlinux packages. Other distributions may slightly differ.

For the library:

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

For the scripts:

* autossh     (provides automatic reverse tunnels)

## Setup Instructions

### Build and Install
Clone this respository in a working directory and build it using ``cmake``. Make sure you have installed the required dependencies.
```
git checkout https://gitlab.kn.e-technik.tu-dortmund.de/dnt/c-mnalib.git
cd c-mnalib
mkdir build
cd build
cmake ../
make
# To install
sudo make install
```

#### Install Prefix
In this example, C-mnalib is build in directory `build` and installed into directory `package`.
```
cmake -B build -S c-mnalib.git -DCMAKE_INSTALL_PREFIX='./package'
make install -C build
```

### Configuration

#### Permissions
Please ensure the user has the right permissions to access the `dialout` device:
```
sudo usermod -a -G dialout <YOUR-USERNAME>
```

#### ModemManager Coexistence
This software may conflict with the systemd-service `ModemManager`. Besides other iterfaces, `ModemManager` may also use AT command interface to configure the mobile broadband connection for the `NetworkManager` service and pipe the payload-data through it.

C-mnalib, however, requires exclusive access to the AT command interface.
To avoid conflicts, either disable `ModemManager` service (1), or configure it to ignore the device controlled by C-mnalib (2).

For (1):
```
sudo systemctl stop ModemManager
sudo systemctl disable ModemManager
```

For (2) add the following file:
```
/etc/udev/rules.d/99-ModemManagerBlacklist.rules
--------------------------------------------------------
# Rules to let ModemManager ignore the TTY (AT) interface of certain modems

SUBSYSTEM!="tty", GOTO="mm_usb_device_blacklist_end"

# Sierre Wireless
# em7455
ATTRS{idVendor}=="1199" ATTRS{idProduct}=="9071", ENV{ID_MM_DEVICE_IGNORE}="1"
# em7565
ATTRS{idVendor}=="1199" ATTRS{idProduct}=="9091", ENV{ID_MM_DEVICE_IGNORE}="1"

LABEL="mm_usb_device_blacklist_end"

```

## Development

### Compile with Debug Symbols:

Release mode optimized but adding debug symbols, useful for profiling :
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ...
```
Debug mode with NO optimization and adding debug symbols :
```
cmake -DCMAKE_BUILD_TYPE=Debug ...
```

## Usage Examples
...


## License
This software is provided under [MIT license](LICENSE.md).

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
