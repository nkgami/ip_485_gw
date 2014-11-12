IP Over RS485 Gateway
====

## Description
IP Over RS485 Gateway Program

## Requirement
Linux and gcc (to build this program)
Debian or Ubuntu (to use init script)
module dummy

## Install
to build this program, just do:

`# make`

to build without demonize, change define DAEMON to 0 in ip\_485\_gw\_util.h

for debian or ubuntu, install init scripts (daemon build only):

`# make install`

uninstall:

`# make uninstall`

You also need dummy interface to use this program correctly.

`# modprobe dummy`

set same address as config file: /etc/ip\_485\_gw.conf

## Usage
for damenon:

`# /etc/init.d/ip_485_gw start | stop | status`

to change settings, edit /etc/ip\_485\_gw.conf

for cli:

`# ./ip_485_gw (path_to_config_file ex. ip_485_gw_conf)`

## Licence

[MIT](https://github.com/tcnksm/tool/blob/master/LICENCE)

## Author

[nkgami](https://github.com/nkgami)
