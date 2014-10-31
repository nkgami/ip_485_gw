/**
 * ip_485_gw_util.c
 *
 * author: nkgami
 * create: 2014-10-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "ip_485_gw_util.h"

int fd = -1;//serial fd
char *network_interface;
char *serial_device;

pthread_mutex_t mutex1,mutex2;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_t tid1,tid2,tid3;

struct frame_queue *sendque_head;
struct frame_queue *recvque_head;
int sendque_total = 0;
int recvque_total = 0;

void ipv6_addr_print(u_char *ip_addr){
	unsigned int i;
	for(i = 0;i < 16;i++){
		printf("%02x:",ip_addr[i]);
	}
	printf("\n");
}

//init serial port  
void serial_init(int fd) {  
    struct termios tio;  
    memset(&tio, 0, sizeof(tio));

    cfmakeraw(&tio);
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 0;

    // speed seting 
    cfsetispeed(&tio, BAUDRATE);  
    cfsetospeed(&tio, BAUDRATE);  
    // set to interface  
    tcsetattr(fd, TCSANOW, &tio);  
  
}

unsigned short crc( unsigned const char *pData, unsigned long lNum )
{
  unsigned int crc16 = 0xFFFFU;
  unsigned long i;
  int j;
  for ( i = 0 ; i < lNum ; i++ ){
    crc16 ^= (unsigned int)pData[i];
    for ( j = 0 ; j < 8 ; j++ ){
      if ( crc16 & 0x0001 ){
        crc16 = (crc16 >> 1) ^ 0xA001;
      }else {
        crc16 >>= 1;
      }
    }
  }
  return (unsigned short)(crc16);
}

unsigned short checksum(unsigned short *buf, int bufsize)
{
	unsigned long sum = 0;
	while (bufsize > 1){
		sum += *buf;
		buf++;
		bufsize -= 2;
	}
	if( bufsize == 1 ){
		sum += *(unsigned char *)buf;
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return ~sum;
}

void sigcatch(){
	pthread_kill(tid1,SIGTERM);
	pthread_kill(tid2,SIGTERM);
	pthread_kill(tid3,SIGTERM);
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_cond_destroy(&cond2);
	exit(1);
}
