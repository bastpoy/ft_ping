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
#include <getopt.h>

#define ICMP_MINLEN 8

struct ping_struct{
    uint16_t sequence;
    unsigned int packet_send;
    unsigned int packet_receive;
    struct timeval firstPacketSend;
    float packet_loss;
};

struct arguments {
    bool verbose;
    int ttl;
};

struct addr_src_host {

};

struct ping_ctx {
    int                 socket;
    int                 index;
    struct arguments    args;
    struct ping_struct  stats;
};

//calc rtt
float find_min(float *array, int size);
float find_avg(float *array, int size);
float find_max(float *array, int size);
float find_mean_deviation(float *array,  int size);

//Initializing
int init_ctx(struct ping_ctx *ctx, int argc, char **argv);
int check_args(int argc, char *argv[], struct ping_ctx *ctx);

//utils
void print_help();
uint16_t checksum(void *addr, size_t len);
void print_result_ping(struct ping_struct ping_struct, float *rttptr, int sequence, double timeStart, char *arg);
double get_timestamp();
void free_msg_rttptr(uint8_t *msg, float *rttptr);

// data

void extract_message(void *pkt, struct icmphdr *msg, double *value, uint8_t *ttl);
void add_time_rtt(float **ptr, float time, int index, int *ptrSize);
bool extract_time_exceeded(struct icmphdr *icmp, int expected_seq);

#endif