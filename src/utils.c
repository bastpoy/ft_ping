#include "../include/header.h"

void print_help(){
    printf("Usage\n  ping [options] <destination>\n\nOptions:\n  <destination>\t\tdns name or ip address\n"
        "  -h\t\t\tprint help and exit\n"
        "  -v\t\t\tverbose output\n\n"
        "For more details see ping(8).\n");
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

int check_args(int argc, char *argv[], int *index, bool *verboseMode){
    if(argc < 2){
        printf("ping: usage error: Destination address required\n");
        return 2;
    }
    else if(argc == 2){
        if(argv[1][0] == '-' && strlen(argv[1]) == 2 && !detect_help_parameter(argv[1])){
            printf("ping: usage error: Destination address required\n");
            return 1;
        }
        else if(detect_help_parameter(argv[1])){
            print_help();
            return 2;
        }
        return 0;
    }
    else if(argc > 3){
        print_help();
        return 2;
    }
    else{
        for(int i = 1; i < argc; i++){
            if(detect_help_parameter(argv[i])){
                print_help();
                return 2;
            }
            else if(!strcmp(argv[i], "-v")){
                if(*verboseMode == true){
                    printf("ping: usage error: Destination address required\n");
                    return 1;
                }
                if(i == 1)
                    *index = 2;
                else
                    *index = 1;
                *verboseMode = true;
            }
        }
        if(*verboseMode == true){
            printf("ping: sock4.fd: 3 (socktype: SOCK_DGRAM), sock6.fd: 4 (socktype: SOCK_DGRAM), hints.ai_family: AF_UNSPEC\n\n");
            return 0;
        }
        printf("ping: %s: Name or service not known\n", argv[2]);
        return 2;
    }
    return 0;
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
        printf("rtt min/avg/max/mdev = %0.3f/%0.3f/%0.3f/%0.3f ms\n", 
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