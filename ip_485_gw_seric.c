/**
 * ip_485_gw_seric.c
 *
 * author: nkgami
 * create: 2014-10-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "ip_485_gw_seric.h"

void *serial_control(void *pParam){
	//for serial
	struct termios oldtio;
	fd_set fds,readfs;
	int maxfd;
	unsigned char readbuf[1500];
	unsigned char read_c;
	int i = 0;
	unsigned short ip_len = 10000;
	unsigned char ip_len_1;
	unsigned char ip_len_2;
	unsigned short hdr_len = 10000;
	unsigned short recv_crc;
	struct frame_queue *temp;
	struct frame_queue *temp_s;
	struct frame_queue *addque;
	int queue_hop = 0,l;
	struct sniff_ip *ip_hdr;
	int writecount;
  char message[150];

	//for timeout
	int res,res2;
	struct timeval Timeout,Timeout2;
	
	fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
	if(fd < 0){
    sprintf(message,"fd:%d %s\n",fd,serial_device);
    enq_log(message);
    #if !DAEMON
      fprintf(stderr,"fd:%d %s\n",fd,serial_device);
    #endif
    usleep(ERRTO);
		exit(1);
	}
  maxfd = fd+1;//for select()
	tcgetattr(fd,&oldtio);//save the old settings
	//pripare for serial
	serial_init(fd);
  #if DEBUG && !DAEMON
	  printf("flush\n");//flush buffer
  #endif
	sleep(2);
	tcflush(fd,TCIOFLUSH);
	sleep(2);
	maxfd = fd+1;//for select
	FD_ZERO(&readfs);
	FD_SET(fd,&readfs);
	srand((unsigned)time(NULL));
	sprintf(message,"thread: start serial\n");
  enq_log(message);
  #if !DAEMON
    printf("thread: start serial\n");
  #endif
	while(1){
		Timeout.tv_usec = 5000;  /* micro sec */
		Timeout.tv_sec  = 0;  /* sec */
		Timeout2.tv_sec = 0;
		Timeout2.tv_usec = (rand()%60)*1000;
		memcpy(&fds,&readfs,sizeof(fd_set));
		res = select(maxfd,&fds, NULL, NULL, &Timeout);
		if(res == 0){//Timeout send etc...
			//printf("select Timeout\n");
			i = 0;//init
			memcpy(&fds,&readfs,sizeof(fd_set));
			res2 = select(maxfd,&fds,NULL,NULL,&Timeout2);
			if(res2 == 0){
				//write to RS485
				pthread_mutex_lock(&mutex1);
				if(sendque_total > 0){
					temp = sendque_head;
					writecount = write(fd,temp->data,temp->length);
					//dequeue
					if(temp->next == NULL){
						free(sendque_head);
						sendque_head = NULL;
						sendque_total = 0;
					}
					else{
						sendque_head = temp->next;
						free(temp);
						sendque_total--;
					}
					#if DEBUG && !DAEMON
						printf("dequeue-send:%d\n",writecount);
					#endif
				}
				pthread_mutex_unlock(&mutex1);
			}
			else{
				continue;
			}
		}
		if(FD_ISSET(fd,&fds)){
			read(fd,&read_c,1);
			#if DEBUG && !DAEMON
				printf("%02x ",read_c);
			#endif
			readbuf[i++] = read_c;
			if(i == 1){
				hdr_len = (unsigned short)read_c;
				#if DEBUG && !DAEMON
					printf("hdrlen:%d\n",hdr_len);
				#endif
			}
			else if(i == 2){
				#if DEBUG && !DAEMON
					printf("hdrtype:%d\n",read_c);
				#endif
			}
			else if(i == 3){
				#if DEBUG && !DAEMON
					printf("nexthdr:%d\n",read_c);
				#endif
			}
			else if(i == hdr_len+3){
				ip_len_1 = read_c;
			}
			else if(i == hdr_len+4){
				ip_len_2 = read_c;
				ip_len = ((unsigned short)(ip_len_1)<<8)+(unsigned short)(ip_len_2);
				#if DEBUG && !DAEMON
					printf("ip_len:%x\n",ip_len);
				#endif
			}
			else if(i == ip_len+hdr_len){
				#if DEBUG && !DAEMON
					printf("end of ip\n");
				#endif
			}
			else if(i == ip_len+hdr_len+2){
				#if DEBUG && !DAEMON
					printf("end of frame\n");
				#endif
				//CRC,checksum check
				recv_crc = crc(readbuf,ip_len+hdr_len);
				if(readbuf[ip_len+hdr_len]==(recv_crc&0xff) && readbuf[ip_len+hdr_len+1]==((recv_crc>>8)&0xff)){
          #if DEBUG && !DAEMON
            printf("CRC_OK\n");
          #endif
					
					//send to raw socket by queue
					pthread_mutex_lock(&mutex2);
					addque = (struct frame_queue*)malloc(sizeof(struct frame_queue));
					for(l = 0;l < ip_len;l++){
						addque->data[l] = readbuf[l+hdr_len];
					}
					addque->length = ip_len;
					addque->next = NULL;

					if(recvque_total == 0){
						pthread_cond_signal(&cond2);
						recvque_head = addque;
						recvque_total = 1;
					}
					else{
						temp_s = recvque_head;
						while(1){
							queue_hop++;
							if(temp_s->next == NULL){
								break;
							}
							else{
								temp_s = temp_s->next;
							}
						}
						temp_s->next = addque;
						recvque_total++;
					}
					#if DEBUG && !DAEMON
						printf("enqueue_recv:%d\n",queue_hop);
						printf("recvque_total:%d\n",recvque_total);
					#endif
					pthread_mutex_unlock(&mutex2);
				}
				else{
					printf("CRC_NG\n");
				}
				ip_hdr = (struct sniff_ip*)(readbuf+hdr_len);
				#if DEBUG && !DAEMON
					printf("ip_id:%d\n",ip_hdr->ip_id);
				#endif
				//reset param
				i = 0;
				ip_len = 10000;
				hdr_len = 10000;
				queue_hop = 0;
			}
		}
	}
}
