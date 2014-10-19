IP Over RS485 Gateway
====

Overview

## Description
IP Over RS485 Gateway Program

## Requirement
Linux and gcc (to build this program)

Debian or Ubuntu (to use init script)

## Install
to build this program, just do:

`# make`

to build without demonize, change define DAEMON to 0 in ip\_485\_gw\_util.h

for debian or ubuntu, install init scripts (daemon build only):

`# make install`

uninstall:

`# make uninstall`

## Usage
for damenon:

`# /etc/init.d/ip_485_gw start | stop | status`

to change the serial device or ip interface, edit /etc/init.d/ip\_485\_gw
and change 'INETINTERFACE' and 'SERIALDEVICE' section

for cli:

`# ./ip_485_gw /dev/ttyUSB0 eth0`

## Licence

[MIT](https://github.com/tcnksm/tool/blob/master/LICENCE)

## Author

[nkgami](https://github.com/nkgami)
