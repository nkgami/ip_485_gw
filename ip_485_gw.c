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
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include "ip_485_gw_util.h"
#include "ip_485_gw_recv.h"
#include "ip_485_gw_seric.h"
#include "ip_485_gw_raws.h"
#include "ip_485_gw_log.h"
#include "ip_485_gw.h"

//for daemon
#define PID_FILE "/var/run/ip_485_gw.pid"
void write_pid();
void daemonize();

//for config
void load_config(char *path, char *conf_name, char *conf);

int main(argc, argv)
    int argc;
    char *argv[];
{
  sendque_total = 0;
  recvque_total = 0;
  
  serial_device = (char *)(malloc(sizeof(char) * CONF_MAX));
  network_interface = (char *)(malloc(sizeof(char) * CONF_MAX));
  log_file = (char *)(malloc(sizeof(char) * CONF_MAX));

	if (argc < 2) {
    #if !DAEMON
		fprintf(stderr, "usage:command [path to config file]\n");
    #endif
    exit(1);
	}
	if(SIG_ERR == signal(SIGINT,sigcatch)){
    #if !DAEMON
		fprintf(stderr,"failed to set signal handler\n");
    #endif
		exit(1);
	}
  load_config(argv[1],"SERIAL",serial_device);
  load_config(argv[1],"INTERFACE0",network_interface);
  load_config(argv[1],"LOG_FILE",log_file);

  #if !DAEMON
	printf("serial device:%s\n",serial_device);
	printf("network_interface:%s\n",network_interface);
	printf("log_file:%s\n",log_file);
  #endif

  #if DAEMON
  //start daemon
  daemonize();
  write_pid();
  #endif

	//set mutex
	pthread_mutex_init(&mutex1, NULL);
	pthread_mutex_init(&mutex2, NULL);
  pthread_mutex_init(&mutexlog, NULL);
  pthread_cond_init(&cond2, NULL);
  pthread_cond_init(&condlog, NULL);

	//create threads
	pthread_create(&tid1, NULL, serial_control, NULL);
	pthread_create(&tid2, NULL, receiver, NULL);
	pthread_create(&tid3, NULL, raw_socket, NULL);
	pthread_create(&tid4, NULL, logger, NULL);
  
	//wait for threads
	pthread_join(tid1,NULL);
	pthread_join(tid2,NULL);
	pthread_join(tid3,NULL);
	pthread_join(tid4,NULL);
   
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
  pthread_mutex_destroy(&mutexlog);
	pthread_cond_destroy(&cond2);
  pthread_cond_destroy(&condlog);
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

void load_config(char *path, char *conf_name, char *conf){
  int i = 0, j = 0;
  char str[STR_MAX], param[STR_MAX];
  FILE *fin;

  if ((fin = fopen(path, "r")) == NULL) {
    #if !DAEMON
		fprintf(stderr, "error: could not open config file:%s\n",path);
    #endif
    exit(1);
  }

  for(;;) {
    if (fgets(str, STR_MAX, fin) == NULL) {
      /* EOF */
      fclose(fin);
      #if !DAEMON
		  fprintf(stderr, "error: could not find %s in %s\n",conf_name, path);
      #endif
      exit(1);
    }
    if (!strncmp(str, conf_name, strlen(conf_name))) {
      while (str[i++] != '=') {
        ;
      }
      while (str[i] != '\n') {
        param[j++] = str[i++];
      }
      param[j] = '\0';
      strcpy(conf, param);
      //printf("param:[%s]\n", param);
      fclose(fin);
      return;
    }
  }
  fclose(fin);
  exit(1);
}
