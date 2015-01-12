IP Over RS485 Gateway
====

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

If you want to set ip address to RS-485 side, use dummy interface.

`# modprobe dummy`

set same address range as config file: /etc/ip\_485\_gw.conf

example for dummy interface:

interface setting:dummy0 192.168.11.1 netmask 255.255.255.0

configfile-NETADDR:192.168.11.0

configfile-NETMASK:255.255.255.0

configfile-INTERFACE0:dummy0

When you use dummy0, you need to enable ipv4 forwarding.

## Usage
for damenon:

`# /etc/init.d/ip_485_gw start | stop | status`

to change settings, edit /etc/ip\_485\_gw.conf

for cli:

`# ./ip_485_gw path_to_config_file ex. ip_485_gw_conf`

## Network Example
![NetworkExample](https://raw.githubusercontent.com/wiki/nkgami/ip_485_gw/images/net_example.png)
Program for arduino node is here [ARIP](https://github.com/nkgami/ARIP). check the example/ in this repository.


## Licence

[MIT](https://github.com/tcnksm/tool/blob/master/LICENCE)

## Author

[nkgami](https://github.com/nkgami)
