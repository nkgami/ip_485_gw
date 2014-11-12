/**
 * ip_485_gw_raws.c
 *
 * author: nkgami
 * create: 2014-10-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#include "ip_485_gw_raws.h"

void *raw_socket(void *pParam){
	struct frame_queue *temp;
	
	int sockfd,on = 1;
	struct sniff_ip *iph;
	int len,iph_len;
	struct sockaddr_in dst_sin;
	ssize_t size;
	int i;
	unsigned char *data;
	unsigned short recv_sum,calc_sum;
  char message[150];

	//socket init
	sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
	if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		sprintf(message,"error:socket\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:socket\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		sprintf(message,"error:setsockopt\n");
    enq_log(message);
    #if !DAEMON
		  fprintf(stderr,"error:setsockopt\n");
    #endif
    usleep(ERRTO);
		exit(1);
	}
  sprintf(message,"thread: start raw_socket\n");
  enq_log(message);
  #if !DAEMON
    printf("thread: start raw_socket\n");
  #endif
	while(1){
		pthread_mutex_lock(&mutex2);
		while (recvque_total==0){
			pthread_cond_wait(&cond2,&mutex2);
		}
		temp = recvque_head;
		//raw_socket_send
		iph=(struct sniff_ip*)(temp->data);
		recv_sum = iph->ip_sum;
		iph_len = IP_HL(iph)*4;
		iph->ip_sum = (unsigned short)0x00;
		calc_sum = checksum((unsigned short*)iph,iph_len);
		#if DEBUG && !DAEMON
			printf("%d:%d\n",recv_sum,calc_sum);
		#endif
		if(calc_sum == recv_sum){
			#if DEBUG && !DAEMON
				printf("IP hdr checksum:OK\n");
			#endif
			//create data for sending
			dst_sin.sin_addr.s_addr = (iph->ip_dst).s_addr;
			dst_sin.sin_family = AF_INET;
			dst_sin.sin_port = htons(0);
			len = temp->length;
			data = (unsigned char*)malloc(len);
			if(data == NULL){
				sprintf(message,"error: cannot malloc\n");
        enq_log(message);
        #if !DAEMON
				  fprintf(stderr,"error: cannot malloc\n");
        #endif
        usleep(ERRTO);
				exit(1);
			}
			for (i = 0;i < len;i++){
				data[i] = temp->data[i];
			}

			//sending
			if ((size = sendto(sockfd, (void *)data, len, 0,&dst_sin, sizeof(dst_sin))) == -1){
				sprintf(message,"error: sendto");
        enq_log(message);
        #if !DAEMON
          fprintf(stderr,"error: sendto");
        #endif
        usleep(ERRTO);
				exit(1);
			}
      enq_log_ip((unsigned char*)data,"send(eth): ");
			free(data);
			data = NULL;
		}
		else{
			#if DEBUG && !DAEMON
				printf("IP hdr checksum:NG\n");
			#endif
		}

		//dequeue
		if(temp->next == NULL){
			free(recvque_head);
			recvque_head = NULL;
			recvque_total = 0;
		}
		else{
			recvque_head = recvque_head->next;
			free(temp);
			recvque_total--;
		}
		#if DEBUG && !DAEMON
			printf("dequeue-recv\n");
			printf("recvque_total:%d\n",recvque_total);
		#endif
		pthread_mutex_unlock(&mutex2);
	}
}
