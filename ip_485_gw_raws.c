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

	//socket init
	sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
	if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("socket");
		exit(1);
	}
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
		exit(1);
	}

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
		#if DEBUG
			printf("%d:%d\n",recv_sum,calc_sum);
		#endif
		if(calc_sum == recv_sum){
			#if DEBUG
				printf("IP hdr checksum:OK\n");
			#endif
			//create data for sending
			dst_sin.sin_addr.s_addr = (iph->ip_dst).s_addr;
			dst_sin.sin_family = AF_INET;
			dst_sin.sin_port = htons(0);
			len = temp->length;
			data = (unsigned char*)malloc(len);
			if(data == NULL){
				fprintf(stderr,"cannot malloc\n");
				exit(EXIT_FAILURE);
			}
			for (i = 0;i < len;i++){
				data[i] = temp->data[i];
			}

			//sending
			if ((size = sendto(sockfd, (void *)data, len, 0,&dst_sin, sizeof(dst_sin))) == -1){
				perror("sendto");
				exit(1);
			}
			free(data);
			data = NULL;
		}
		else{
			#if DEBUG
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
		#if DEBUG
			printf("dequeue-recv\n");
			printf("recvque_total:%d\n",recvque_total);
		#endif
		pthread_mutex_unlock(&mutex2);
	}
}
