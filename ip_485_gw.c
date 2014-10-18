/**
 * ip_485_gw.c
 *
 * author: nkgami
 * create: 2014-10-18
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/stat.h>
#include "ip_485_gw_util.h"
#include "ip_485_gw_pcapc.h"
#include "ip_485_gw_seric.h"
#include "ip_485_gw_raws.h"

//for daemon
#define PID_FILE "/var/run/ip_485_gw.pid"
void write_pid();
void daemonize();

int main(argc, argv)
    int argc;
    char *argv[];
{
	pthread_t tid1,tid2,tid3;
  sendque_total = 0;
  recvque_total = 0;

	if (argc < 3) {
		fprintf(stderr, "usage:command [serial device] [network interface]\n");
		exit(1);
	}
	if(SIG_ERR == signal(SIGINT,sigcatch)){
		fprintf(stderr,"failed to set signal handler\n");
		exit(1);
	}
	serial_device = argv[1];
	network_interface = argv[2];
	
  //start daemon
  daemonize();
  write_pid();

  //start threads

	//set mutex
	pthread_mutex_init(&mutex1, NULL);
	pthread_mutex_init(&mutex2, NULL);

	//create threads
	pthread_create(&tid1, NULL, serial_control, NULL);
	pthread_create(&tid2, NULL, pcap_control, NULL);
	pthread_create(&tid3, NULL, raw_socket, NULL);
   
	//wait for threads
	pthread_join(tid1,NULL);
	pthread_join(tid2,NULL);
	pthread_join(tid3,NULL);
   
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_cond_destroy(&cond2);
	return 0;
}

void write_pid(){
  FILE *fp;
  fp = fopen(PID_FILE, "w");
  fprintf(fp, "%d", getpid());
  fclose(fp);
}

void daemonize(){
  pid_t pid, sid;

  /* change directory to / */
  if( chdir("/") < 0 )  exit(EXIT_FAILURE);

  /* start child process */
  pid = fork();
  if( pid < 0 ) exit(EXIT_FAILURE);

  /* stop parent process */
  if( pid > 0 ) exit(EXIT_SUCCESS);

  /* start new session */
  sid = setsid();
  if( sid < 0 ) exit(EXIT_FAILURE);

  /* reset file cleate mask */
  umask(0);

  /* close stdin,stdou,stderr */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  return;
}
