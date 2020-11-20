C-mnalib
========

[![Release](https://img.shields.io/github/v/tag/falkenber9/c-mnalib)](https://github.com/falkenber9/c-mnalib/releases)
[![AUR](https://img.shields.io/aur/version/c-mnalib)](https://aur.archlinux.org/packages/c-mnalib)
[![DOI](https://img.shields.io/badge/DOI-10.1109/VTCFall.2018.8690629-fcb426.svg)](https://dx.doi.org/10.1109/VTCFall.2018.8690629)
[![arXiv](https://img.shields.io/badge/arXiv-1806.06620-b31b1b.svg)](https://arxiv.org/abs/1806.06620)
[![License](https://img.shields.io/github/license/falkenber9/c-mnalib)](LICENSE.md)

<img src="logo.png" height="50" style="float: left;"/>
C-mnalib is a C library for easy access and configuration of mobile network modems, e.g., LTE modems.
The library focuses on providing access to network quality indicators and network parameters of the modems by using AT commands.



![TU Dortmund University](gfx/tu-dortmund_small.png "TU Dortmund University")
![SFB 876](gfx/SFB876_small.png "Collaborative Research Center SFB 876")
![Communication Networks Institute](gfx/CNI_small.png "Communication Networks Institute")
![DFG](gfx/DFG_small.png "DFG")



## Library Overview

### Supported Devices
Currently, two Sierra Wireless devices are supported:

* Sierra Wireless MC7455/EM7455
* Sierra Wireless EM7565

Interfacing with the modem is performed with *AT commands* over a TTY endpoint, e.g. ``/dev/ttyUSBX``.
The library is tested with ``GOBI`` drivers from the manufacturer. However, the *default* ``libqmi`` drivers, which are installed as a dependency of ModemManager, should also be OK for most functions. Please ensure coexistence with ModemManager as described below.



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

### Installation on Archlinux
On Archlinux build and install the package ``c-mnalib`` [![](https://img.shields.io/aur/version/c-mnalib)](https://aur.archlinux.org/packages/c-mnalib) from the [Arch User Repository (AUR)](https://aur.archlinux.org).
The most convenient way is the use of an [AUR Helper](https://wiki.archlinux.org/index.php/AUR_helpers), e.g. [yay](https://aur.archlinux.org/packages/yay) or [pacaur](https://aur.archlinux.org/packages/pacaur). The following example shows the installation with ``yay``.

```sh
# Install
yay -Sy c-mnalib

# Uninstall
sudo pacman -Rs c-mnalib
```



### Manually Build and Install
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
On Archlinux, the group is named ``uucp`` instead of ``dialout``.

#### ModemManager Coexistence
This software may conflict with the systemd-service `ModemManager`. Besides other iterfaces, `ModemManager` may also use AT command interface to configure the mobile broadband connection for the `NetworkManager` service and even pipe the payload-data through it.

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



## License
This software is provided under [MIT license](LICENSE.md).

## Acknowledgements
If you wish to acknowledge this library in a publication, please refer to:
```
@InProceedings{FalkenbergSliwaPiatkowskiEtal2018a,
	Author = {Robert Falkenberg and Benjamin Sliwa and Nico Piatkowski and Christian Wietfeld},
	Title = {Machine Learning Based Uplink Transmission Power Prediction for {LTE} and Upcoming {5G} Networks using Passive Downlink Indicators},
	Booktitle = {2018 IEEE 88th IEEE Vehicular Technology Conference (VTC-Fall)},
	Year = {2018},
	Address = {Chicago, USA},
	Month = aug,
	Publisher = {IEEE},
	Doi = {10.1109/VTCFall.2018.8690629},
	Eprint = {1806.06620},
	Eprinttype = {arxiv},
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
