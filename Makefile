#author: nkgami
#create: 2014-10-18
CC=gcc
COMMON_OBJS = ip_485_gw_pcapc.o ip_485_gw_raws.o ip_485_gw_seric.o ip_485_gw_util.o
LFLAG = -lpthread -lpcap
OPTIONS = -Wall
NAME=ip_485_gw

.c.o :
		gcc $(OPTIONS) -c $<

all: ip_485_gw

ip_485_gw: ip_485_gw.o $(COMMON_OBJS)
		$(CC) $(OPTIONS) $(COMMON_OBJS) ip_485_gw.o -o ip_485_gw $(LFLAG)

clean: 
	rm -f *.o ip_485_gw *~

install: all
	install -o root -g root -m 0755 ${NAME} /usr/local/sbin/${NAME}
	install -o root -g root -m 0755 ${NAME}_initsh /etc/init.d/${NAME}

uninstall:
	/etc/init.d/${NAME} stop
	rm -f /usr/local/sbin/${NAME}
	rm -f /etc/init.d/${NAME}

.PHONY: all clean install uninstall
