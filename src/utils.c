#include "../include/header.h"

void print_help(){
    printf("Usage\n  ping [options] <destination>\n\nOptions:\n  <destination>\t\tdns name or ip address\n"
        "  -h\t\t\tprint help and exit\n");
}

bool detect_help_parameter(char *argv){
    if(argv[0] == '-'){
        for(int i = 1; i < (int)strlen(argv); i++){
            if(argv[i] == 'h' || argv[i] == '?'){
                return true;
            }
        }
    }
    return false;
}

double get_timestamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

void print_result_ping(struct ping_struct ping_struct, float *rttptr, int sequence, double timeStart, char *arg){
    printf("\n--- %s ping statistics ---  \n", arg);
    float packet_loss = ((float)(ping_struct.packet_send - ping_struct.packet_receive) / (float)ping_struct.packet_send) * 100.0 ;
    printf("%d packets transmitted, %d received, %0.0f%% packet loss, time %0.2fms\n", 
        ping_struct.packet_send, ping_struct.packet_receive, packet_loss, (get_timestamp() - timeStart) * 1000);
    if(ping_struct.packet_receive > 0)
        printf("round-trip min/avg/max/mdev = %0.3f/%0.3f/%0.3f/%0.3f ms\n", 
            find_min(rttptr, sequence), find_avg(rttptr, sequence), find_max(rttptr, sequence), find_mean_deviation(rttptr, sequence));    
}

uint16_t checksum(void *addr, size_t len) {
    uint16_t *word = addr;
    uint32_t result = 0;

    for (long unsigned int i = 0; i < len / sizeof(uint16_t); i++) {
        result += *(word + i);
    }
    if (len % 2 == 1) {
        result += *((uint8_t *)addr + len - 1);
    }
    result = (result >> 16) + (result & 0xffff);
    
    result = (result >> 16) + (result & 0xffff);
    return ~result;
}

void free_msg_rttptr(uint8_t *msg, float *rttptr){
    free(msg);
    free(rttptr);
}