/**
 * ip_485_gw_log.c
 *
 * author: nkgami
 * create: 2014-10-31
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ip_485_gw_log.h"

void *logger(void *pParam){
  struct msg_queue *temp;
	while (1){
		pthread_mutex_lock(&mutexlog);
		while (msgque_total == 0){
			pthread_cond_wait(&condlog,&mutexlog);
		}
		temp = msgque_head;
    if ((fplog = fopen(log_file,"a")) == NULL) {
    #if !DAEMON
      fprintf(stderr, "failed to open file\n");
    #endif
      sleep(1);
      continue;
    }
    fputs(temp->msg,fplog);
    fclose(fplog);
    if (temp->next == NULL){
      free(msgque_head);
      msgque_head = NULL;
      msgque_total = 0;
    }
    else{
      msgque_head = msgque_head->next;
      free(temp);
      msgque_total--;
    }
    pthread_mutex_unlock(&mutexlog);
  }
}
