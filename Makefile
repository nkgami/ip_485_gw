#author: nkgami
#create: 2014-10-18
CC=gcc
COMMON_OBJS = ip_485_gw_util.o ip_485_gw_log.o ip_485_gw_recv.o ip_485_gw_raws.o ip_485_gw_seric.o
LFLAG = -lpthread -lpcap
OPTIONS = -Wall -O3
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
	install -o root -g root -m 0644 ${NAME}_logrotate /etc/logrotate.d/${NAME}

uninstall:
	/etc/init.d/${NAME} stop
	rm -f /usr/local/sbin/${NAME}
	rm -f /etc/init.d/${NAME}
	rm -f /etc/logrotate.d/${NAME}
  
.PHONY: all clean install uninstall
