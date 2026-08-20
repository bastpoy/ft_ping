#ifndef _HEADER_H_
#define _HEADER_H_
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define ICMP_MINLEN 8

struct ping_struct{
    uint16_t sequence;
    unsigned int packet_send;
    unsigned int packet_receive;
    struct timeval firstPacketSend;
    float packet_loss;
};

//calc rtt
float find_min(float *array, int size);
float find_avg(float *array, int size);
float find_max(float *array, int size);
float find_mean_deviation(float *array,  int size);

//utils
void print_help();
int check_args(int argc, char *argv[], int *index, bool *verboseMode);
void print_result_ping(struct ping_struct ping_struct, float *rttptr, int sequence, double timeStart, char *arg);
double get_timestamp();
void free_msg_rttptr(uint8_t *msg, float *rttptr);

#endif