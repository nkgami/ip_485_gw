/**
 * ip_485_gw_recv.c
 *
 * author: nkgami
 * create: 2014-11-11
 */
#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h> 
#include <linux/if_arcnet.h> 
#include <linux/version.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "ip_485_gw_recv.h"
#include <signal.h>

int enqueue_send(const u_char *h_ip,const unsigned long ip_len);
void check_and_enqueue (unsigned char *buf, int i);
#if DEBUG && !DAEMON
void hexdump(unsigned char *p, int count);
#endif
uint32_t str_to_bin_addr(char *str);
int check_inet_addr(char *str);

char *net_addr_s;
char *net_mask_s;
uint32_t net_addr_i;
uint32_t net_mask_i;
int pd = -1;
 
void* receiver(void *pParam){
	unsigned char buf[2048];
	struct sockaddr_ll sll;
	int i;
	struct ifreq ifr;
	int ifindex;
  char *interface = network_interface;
  char message[150];

  //for ipv6
	//const struct sniff_ipv6 *ipv6;

  net_addr_s = netaddr;
  net_mask_s = netmask;

  if(check_inet_addr(net_addr_s) == -1){
		sprintf(message,"error: net_addr\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error: net_addr\n");
    #endif
    usleep(ERRTO);
		exit(1);
  }
  net_addr_i = str_to_bin_addr(net_addr_s);
  if(check_inet_addr(net_mask_s) == -1){
		sprintf(message,"error: net_mask\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error: net_mask\n");
    #endif
    usleep(ERRTO);
		exit(1);
  }
  net_mask_i = str_to_bin_addr(net_mask_s);

	pd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (pd == -1) {
		sprintf(message,"error:socket\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:socket\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);
	if (ioctl(pd, SIOCGIFINDEX, &ifr) == -1) {
		sprintf(message,"error:SIOCGIFINDEX\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:SIOCGIFINDEX\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}
	ifindex = ifr.ifr_ifindex;

	/* get hardware address */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);
	if (ioctl(pd, SIOCGIFHWADDR, &ifr) == -1) {
		sprintf(message,"error:SIOCGIFINDEX\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:SIOCGIFINDEX\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, interface, IFNAMSIZ);
  ioctl(pd,SIOCGIFFLAGS,&ifr);
  ifr.ifr_flags|=IFF_PROMISC;
  ioctl(pd,SIOCSIFFLAGS,&ifr);

  memset(&sll, 0xff, sizeof(sll));
	sll.sll_family = AF_PACKET;	/* allways AF_PACKET */
	sll.sll_protocol = htons(ETH_P_ALL);
	sll.sll_ifindex = ifindex;
	if (bind(pd, (struct sockaddr *)&sll, sizeof sll) == -1) {
		sprintf(message,"error:bind\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:bind\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}
  sprintf(message,"thread: start receiver\n");
  enq_log(message);
  #if !DAEMON
    printf("thread: start receiver\n");
  #endif

	do {
		fd_set fds;
		struct timeval t;
		FD_ZERO(&fds);	
		FD_SET(pd, &fds);
		memset(&t, 0, sizeof(t));
		i = select(FD_SETSIZE, &fds, NULL, NULL, &t);
		if (i > 0){
			recv(pd, buf, i, 0);
    }
	} while (i);

	for (;;) {
		i = recv(pd, buf, sizeof(buf), 0);
		if (i < 0) {
      sprintf(message,"error:recv\n");
      enq_log(message);
      #if !DAEMON
        fprintf(stderr,"error:recv\n");
      #endif
      usleep(ERRTO);
			exit(1);
		}
		if (i == 0){
			continue;
    }
    #if DEBUG && !DAEMON
      hexdump(buf, i);
    #endif
		check_and_enqueue(buf, i);
	}
	exit(0);
}

void check_and_enqueue(unsigned char *p, int count){
	int i;
	struct ether_header *eh;
	const struct sniff_ip *ip;
	const u_char *h_ip;
	uint32_t dst_addr;
	eh = (struct ether_header *)p;
  #if DEBUG && !DAEMON
  uint32_t src_addr;
  int j;
  #endif
	if (ntohs (eh->ether_type) == ETHERTYPE_IP){
		ip = (struct sniff_ip*)(p + SIZE_ETHERNET);
		//src_addr = ntohl(ip->ip_src.s_addr);
		dst_addr = ntohl(ip->ip_dst.s_addr);
		//send to queue
		if((dst_addr&net_mask_i) == (net_addr_i&net_mask_i)){
			h_ip = (u_char*)ip;
			#if DEBUG && !DAEMON
				printf("debug\n");
				for(j = 0;j <ntohs(ip->ip_len);j++){
					printf("%02x ",h_ip[j]);
				}
				printf("\n");
			#endif
      enq_log_ip((unsigned char*)h_ip, "que (485): ");
			enqueue_send(h_ip,ntohs(ip->ip_len));

		}
	}
}

int enqueue_send(h_ip,ip_len)
	const u_char *h_ip;
	const unsigned long ip_len;
{
  /*
	header length: 1byte (MAX:255byte)
  header type: 1byte (MAX:255types)
	next header: 1byte (IP,IPv6..)
	extend header (optional, not implemented)
	
  add CRC to the end of frame
	set the end of frame from the inter frame gap
	
  after read the data,
	1.if the real length is shorter than the length in the header
	read to the end and destruct
	2.if the real length is longer than the length in the header
	destruct because no data in the buffer
	3.if the data length is correct
	check crc
	*/

	int queue_hop = 0;
	int writecount = 0;
	struct frame_queue *tempque;
	struct frame_queue *addque;
	//simple header
	u_char framehdr[3];
	//for CRC
	unsigned int crc16 = 0xFFFFU;
	unsigned long i,k,l;
	int j;
	u_char *send_data;
	unsigned long data_len,frame_len;
	unsigned int hdr_len = 3;
  char message[200];
	
	//header setting
	framehdr[0] = hdr_len;//header length
	framehdr[1] = 1;//start hdr
	framehdr[2] = 4;//next:IPv4
	
	frame_len = hdr_len+ip_len+2;//framehdr+ip+CRC
	data_len = hdr_len+ip_len;//framehdr+ip for CRC
	send_data = (u_char*)malloc(frame_len);
	if(send_data == NULL){
		sprintf(message,"error: cannot malloc\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error: cannot malloc\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}
	//set frame_header
	for(k = 0;k < hdr_len;k++){
		send_data[k] = framehdr[k];
	}
	//set IPdata
	for(k = 0;k < ip_len;k++){
		send_data[k+hdr_len] = h_ip[k];
	}
	//add CRC
	for ( i = 0 ; i < data_len ; i++ ){
		crc16 ^= (unsigned int)send_data[i];
		for ( j = 0 ; j < 8 ; j++ ){
			if ( crc16 & 0x0001 ){
				crc16 = (crc16 >> 1) ^ 0xA001;
			}else{
				crc16 >>= 1;
			}
		}
	}
	send_data[data_len] =(u_char)(((unsigned short)crc16)&0x0ff);
	send_data[data_len+1] =(u_char)((((unsigned short)crc16)>>8)&0x0ff);
	pthread_mutex_lock(&mutex1);
	if(sendque_total == 0){
		sendque_total = 1;
		sendque_head = (struct frame_queue*)malloc(sizeof(struct frame_queue));
    if(sendque_head == NULL){
      sprintf(message,"error: cannot malloc\n");
      enq_log(message);
      #if !DAEMON
        fprintf(stderr,"error: cannot malloc\n");
      #endif
      usleep(ERRTO);
      exit(1);
		}
		sendque_head->length = frame_len;
		for(l = 0;l < frame_len;l++){
			sendque_head->data[l] = send_data[l];
		}
		sendque_head->next = NULL;
	}
	else{
		tempque = sendque_head;
		while(1){
			queue_hop++;
			if(tempque->next == NULL){
				break;
			}
			else{
				tempque = tempque->next;
			}
		}
		addque = (struct frame_queue*)malloc(sizeof(struct frame_queue));
		for(l = 0;l < frame_len;l++){
			addque->data[l] = send_data[l];
		}
		addque->next = NULL;
		addque->length = frame_len;
		tempque->next = addque;
		sendque_total++;
	}
	free(send_data);
	send_data = NULL;
	#if DEBUG && !DAEMON
		printf("enqueue_send:%d\n",queue_hop);
	#endif
	pthread_mutex_unlock(&mutex1);
	return writecount;
}

#if DEBUG && !DAEMON
void hexdump(unsigned char *p, int count)
{
	int i, j;

	for(i = 0; i < count; i += 16) {
		printf("%04x : ", i);
		for (j = 0; j < 16 && i + j < count; j++)
			printf("%2.2x ", p[i + j]);
		for (; j < 16; j++) {
			printf("   ");
		}
		printf(": ");
		for (j = 0; j < 16 && i + j < count; j++) {
			char c = toascii(p[i + j]);
			printf("%c", isalnum(c) ? c : '.');
		}
		printf("\n");
	}
}
#endif

int check_inet_addr(char *str)
{
  struct in_addr inp;
  if(inet_aton(str, &inp) == 0){
    return -1;
  }
  else{
    return 0;
  }
}
uint32_t str_to_bin_addr(char *str)
{
  struct in_addr inp;
  inet_aton(str, &inp);
  return ntohl(inp.s_addr);
}
