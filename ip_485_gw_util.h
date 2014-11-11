/**
 * ip_485_gw_util.h
 *
 * author: nkgami
 * create: 2014-10-18
 */

#ifndef ___IP_485_GW_UTIL_H___
#define ___IP_485_GW_UTIL_H___

#define _GNU_SOURCE

#define DEBUG 0
#define DAEMON 1

#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

//for Ethernet
#define SIZE_ETHERNET 14
#define IP_ICMP 0x01
#define IP_IGMP 0x02
#define IP_TCP 0x06
#define IP_UDP 0x11

#define MAX_LENGTH 1500

//115200bps
#define BAUDRATE B115200

#define BUFFSIZE 2000
#define COULDNOTFORK -1
  
#define FALSE 0
#define TRUE 1

#define ERRTO 200000

extern int fd;//serial fd
extern FILE *fplog;//log
extern char *network_interface;
extern char *serial_device;
extern char *log_file;
extern char *log_file;

extern pthread_mutex_t mutex1,mutex2;
extern pthread_mutex_t mutexlog;
extern pthread_cond_t cond2;
extern pthread_cond_t condlog;

extern struct frame_queue *sendque_head;
extern struct frame_queue *recvque_head;
extern int sendque_total;
extern int recvque_total;

extern struct msg_queue *msgque_head;
extern int msgque_total;

//threads
extern pthread_t tid1,tid2,tid3,tid4;

struct sniff_ip {
        u_char ip_vhl;      /* version << 4 | header length >> 2 */
        u_char ip_tos;      /* type of service */
        u_short ip_len;     /* total length */
        u_short ip_id;      /* identification */
        u_short ip_off;     /* fragment offset field */
    #define IP_RF 0x8000        /* reserved fragment flag */
    #define IP_DF 0x4000        /* dont fragment flag */
    #define IP_MF 0x2000        /* more fragments flag */
    #define IP_OFFMASK 0x1fff   /* mask for fragmenting bits */
        u_char ip_ttl;      /* time to live */
        u_char ip_p;        /* protocol */
        u_short ip_sum;     /* checksum */
        struct in_addr ip_src;
        struct in_addr ip_dst; /* source and dest address */
};
    #define IP_HL(ip)       (((ip)->ip_vhl) & 0x0f)
    #define IP_V(ip)        (((ip)->ip_vhl) >> 4)

//for IPv6
struct sniff_ipv6{
	u_char ip_vc;
	u_char ip_cl;
	u_short ip_l2;
	u_short ip_len;
	u_char ip_nexthdr;
	u_char ip_hopl;
	u_char ip_src[16];
	u_char ip_dst[16];
};
	#define IPv6_V(ipv6)	(((ipv6)->ip_vc)>>4)
	#define IPv6_TC(ipv6)	(((((ipv6)->ip_vc)<<4)+(((ipv6)->ip_cl)>>4)))
	#define IPv6_FL(ipv6)	((((ipv6)->ip_cl)<<16)+((ipv6)->l2))

void ipv6_addr_print(u_char *ip_addr);
//for end of IPv6


//frame queue
struct frame_queue{
	int length;
	unsigned char data[2000];
	struct frame_queue *next;
};

//log message queue
struct msg_queue{
  char msg[200];
  struct msg_queue *next;
};

//threads
void* serial_control(void* pParam);
void* receiver(void* pParam);
void* raw_socket(void* pParam);
void* logger(void* pParam);

//init serial port  
void serial_init(int fd);

unsigned short crc( unsigned const char *pData, unsigned long lNum );

unsigned short checksum(unsigned short *buf, int bufsize);

void sigcatch();

void enq_log(char *msg);
void enq_log_ip(unsigned char *ipmsg, char *head_msg);

#endif
