/**
 * ip_485_gw_pcapc.h
 *
 * author: nkgami
 * create: 2014-10-18
 */

#ifndef ___IP_485_GW_PCAPC_H___
#define ___IP_485_GW_PCAPC_H___

#include <pcap.h>
#include "ip_485_gw_util.h"

int enqueue_send(const u_char* h_ip,const unsigned long ip_len);
void receiver(u_char *, const struct pcap_pkthdr *, const u_char *);

#endif
