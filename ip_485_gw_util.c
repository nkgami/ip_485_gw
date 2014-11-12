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
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "ip_485_gw_util.h"

int fd = -1;//serial fd
char *network_interface;
char *serial_device;
char *log_file;
char *netaddr;
char *netmask;
FILE *fplog;//log

pthread_mutex_t mutex1,mutex2, mutexlog;
pthread_cond_t cond2,condlog;
pthread_t tid1,tid2,tid3, tid4;

struct frame_queue *sendque_head;
struct frame_queue *recvque_head;
struct msg_queue *msgque_head;
int sendque_total = 0;
int recvque_total = 0;
int msgque_total = 0;

void get_now_str(char *s_time){
  struct tm *tmp;
  struct timeval tv;

  gettimeofday(&tv,NULL);
  tmp=localtime(&tv.tv_sec);
  sprintf(s_time,"%04d/%02d/%02d %02d:%02d:%02d.%3d",
      tmp->tm_year + 1900, tmp->tm_mon + 1,
      tmp->tm_mday, tmp->tm_hour,
      tmp->tm_min, tmp->tm_sec,
      tv.tv_usec/1000);
}

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
	pthread_kill(tid4,SIGTERM);
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutexlog);
	pthread_cond_destroy(&cond2);
	pthread_cond_destroy(&condlog);
  fclose(fplog);
	exit(1);
}

void enq_log(char *msg){
  struct msg_queue *data;
  struct msg_queue *temp;
  char msg_with_date[200];
  char time_now[50];
  char error_msg[] = "over msg length\n";
  get_now_str(time_now);
  data = (struct msg_queue*)malloc(sizeof(struct msg_queue));
  if(strlen(msg) < 150){
    sprintf(msg_with_date,"%s %s",time_now,msg);
    strcpy(data->msg, msg_with_date);
  }
  else{
    sprintf(msg_with_date,"%s %s",time_now,error_msg);
    strcpy(data->msg, msg_with_date);
  }
  data->next = NULL;
  pthread_mutex_lock(&mutexlog);
  if (msgque_total == 0){
    pthread_cond_signal(&condlog);
    msgque_head = data;
    msgque_total = 1;
  }
  else{//msg already in queue
    temp = msgque_head;
    while(1){
      if(temp->next == NULL){
        break;
      }
      else{
        temp = temp->next;
      }
    }
    temp->next = data;
    msgque_total++;
  }
  pthread_mutex_unlock(&mutexlog);
}
void enq_log_ip(unsigned char *ipmsg, char *head_msg){
  struct sniff_ip *iph;
  char msg[100];
  char src_addr[20];
  char dst_addr[20];
  char *tmp_addr;
  iph = (struct sniff_ip *)ipmsg;
  tmp_addr = inet_ntoa(iph->ip_src);
  strcpy(src_addr, tmp_addr);
  tmp_addr = inet_ntoa(iph->ip_dst);
  strcpy(dst_addr, tmp_addr);
  sprintf(msg,"%s id:%d src:%s -> %s\n",head_msg,ntohs(iph->ip_id),src_addr,dst_addr);
  enq_log(msg);
}
