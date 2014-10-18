/**
 * ip_485_gw_pcapc.c
 *
 * author: nkgami
 * create: 2014-10-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <string.h>
#include <netinet/if_ether.h>
#include "ip_485_gw_pcapc.h"

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
	
	//header setting
	framehdr[0] = hdr_len;//header length
	framehdr[1] = 1;//start hdr
	framehdr[2] = 4;//next:IPv4
	
	frame_len = hdr_len+ip_len+2;//framehdr+ip+CRC
	data_len = hdr_len+ip_len;//framehdr+ip for CRC
	send_data = (u_char*)malloc(frame_len);
	if(send_data == NULL){
		fprintf(stderr,"cannot malloc\n");
		exit(EXIT_FAILURE);
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
			fprintf(stderr,"cannot malloc\n");
			exit(EXIT_FAILURE);
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
	#if DEBUG
		printf("enqueue_send:%d\n",queue_hop);
	#endif
	pthread_mutex_unlock(&mutex1);
	return writecount;
}

void receiver(userdata, h, p)
	u_char *userdata;              /* in this sample, userdata is NULL */
	const struct pcap_pkthdr *h;   /* structure h containing time stamp */
					/* and packet size, and etc ... */
	const u_char *p;               /* body of packet data */
{
	int i;
	struct ether_header *eh;
	const struct sniff_ip *ip;
	const u_char *h_ip;
	uint32_t dst_addr;
	eh = (struct ether_header *)p;
	u_char net_addr[4];
  uint32_t net_addr_i;
  #if DEBUG
  uint32_t src_addr;
  int j;
  #endif
  //for ipv6
	//const struct sniff_ipv6 *ipv6;

	net_addr[0] = 192;
	net_addr[1] = 168;
	net_addr[2] = 11;
	net_addr[3] = 0;
	
	for(i = 0;i< 4;i++){
		net_addr_i = (net_addr_i<<8)+net_addr[i];
    	}
	if (ntohs (eh->ether_type) == ETHERTYPE_IP){
		ip = (struct sniff_ip*)(p + SIZE_ETHERNET);
		//src_addr = ntohl(ip->ip_src.s_addr);
		dst_addr = ntohl(ip->ip_dst.s_addr);
		//send to queue
		if(dst_addr>>8 == net_addr_i>>8){
			h_ip = (u_char*)ip;
			#if DEBUG
				printf("debug\n");
				for(j = 0;j <ntohs(ip->ip_len);j++){
					printf("%02x ",h_ip[j]);
				}
				printf("\n");
			#endif
			enqueue_send(h_ip,ntohs(ip->ip_len));
		}
	}
	/*/
  /for ipv6
	else if (ntohs(eh->ether_type) == ETHERTYPE_IPV6){
		printf("IPv6\n");
		ipv6 = (struct sniff_ipv6*)(p+SIZE_ETHERNET);
		printf("src_addr: ");
		ipv6_addr_print((unsigned char*)(&(ipv6->ip_src)));
		printf("dst_addr: ");
		ipv6_addr_print((unsigned char*)(&(ipv6->ip_dst)));
	}*/
}

void *pcap_control(void *pParam){
	printf("network interface:%s\n",network_interface);
	char *device;
	pcap_t *pd;
	int snaplen = 2000;
	int pflag = 0;
	int timeout = 1000;
	char ebuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 localnet, netmask;
	pcap_handler callback;
	//struct bpf_program fcode;
	/* specify network interface capturing packets */
	device = network_interface;
	/* open network interface with on-line mode */
	memset(ebuf,0,sizeof(ebuf));
	printf("OPEN\n");
	if ((pd = pcap_open_live(device, snaplen, !pflag, timeout, ebuf)) == NULL) {
		fprintf(stderr, "Can't open pcap deivce\n");
		exit(1);
	}

	/* get informations of network interface */
	if (pcap_lookupnet(device, &localnet, &netmask, ebuf) < 0) {
		fprintf(stderr, "Can't get interface informartions\n");
		exit(1);
    	}
	/* setting and compiling packet filter */
	
	/* set call back function for output */
	/* in this case output is print-out procedure for ethernet addresses */
	callback = receiver;
	
	/* loop packet capture util picking 1024 packets up from interface. */
	/* after 1024 packets dumped, pcap_loop function will finish. */
	/* argument #4 NULL means we have no data to pass call back function. */
	printf("start capture\n");
	if (pcap_loop(pd,0, callback, NULL) < 0) {
		(void)fprintf(stderr, "pcap_loop: error occurred\n");
		exit(1);
	}

	/* close capture device */
	printf("pcap_stoped\n");
	pcap_close(pd);
	exit(0);
}
