#include "../include/header.h"

int interrupted = 0;

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

struct sockaddr_in *get_my_ip(char *buffer){
    struct ifaddrs *host_addrs = NULL;
    struct sockaddr_in *addr= NULL;

    
    int ret = getifaddrs(&host_addrs);
    void *ptrmec = host_addrs;
    if(ret == -1){
        printf("Error getting host addresses");
        return(NULL);
    }
    while(host_addrs){

        sa_family_t family = host_addrs->ifa_addr->sa_family;
        addr = (struct sockaddr_in*)host_addrs->ifa_addr;

        if(family == AF_INET){
            if(strcmp(host_addrs->ifa_name, "wlp2s0") == 0){
                if(!inet_ntop(AF_INET, &addr->sin_addr, buffer, INET_ADDRSTRLEN)){
                    printf("Error converting address too string\n");
                    freeifaddrs(ptrmec);
                    return NULL;
                }
                freeifaddrs(ptrmec);
                return addr;
            }
        }
        host_addrs = host_addrs->ifa_next;
    }
    freeifaddrs(ptrmec);
    return NULL;
}

int getDestAddr(char *argv, struct sockaddr_in *addrDest, char *buffer){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char ipstr[INET_ADDRSTRLEN];
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(argv, NULL, &hints, &result);
    if(ret != 0){
        if(ret == -2){
            printf("ping: %s: Name or service not known\n", argv);
            freeaddrinfo(result);
        }
        else if (ret == -3){
            printf("ping: %s: Temporary failure in name resolution\n", argv);
            freeaddrinfo(result);
        }
        return 2;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *temp = (struct sockaddr_in *)rp->ai_addr;
            memcpy(addrDest, (struct sockaddr_in *)rp->ai_addr, sizeof(struct sockaddr_in));
            inet_ntop(rp->ai_family, &(temp->sin_addr), ipstr, sizeof(ipstr));
            if (getnameinfo((struct sockaddr*)temp, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf,
                    sizeof(sbuf), NI_NAMEREQD) == 0){
                    memcpy(buffer, hbuf, sizeof(hbuf));
            }
            freeaddrinfo(result);
            return 0;
        }
    }
    freeaddrinfo(result);
    printf("ping: %s: Temporary failure in name resolution\n", argv);
    return 2;
}

void extract_message(void *pkt, struct icmphdr *msg, double *value, uint8_t *ttl) {
    struct ip *ipHeader = pkt;
    *ttl = ipHeader->ip_ttl;
    //je decale de la struct ip
    pkt = ipHeader + 1;
    //je prends la data contenu dans le packet icmp
    memcpy(value, pkt + 8, 8);
    //je prends le packet icmp
    memcpy(msg, pkt, 8);
}

void add_time_rtt(float **ptr, float time, int index, int *ptrSize){
    if(index >= *ptrSize){
        *ptrSize = *ptrSize + 15;
        *ptr = (float*)realloc(*ptr, (*ptrSize) * sizeof(float));
    }
    (*ptr)[index] = time;
}

void sigint_handler(int signal)
{
    if (signal == SIGINT){
        interrupted = 1;
    }
}

void create_packet(struct icmphdr *packet, struct ping_struct ping_struct, uint8_t *msg){
    packet->type = ICMP_ECHO; 
    packet->code = 0;
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = ping_struct.sequence;
    packet->checksum = 0;
    packet = (struct icmphdr*)msg;
    double *timestamp = (double *)(packet + 1);
    *timestamp = get_timestamp();
    packet->checksum = checksum(msg, 16);
}

int main(int argc, char *argv[]){
    struct sockaddr_in dest_addr;
    struct icmphdr icmpresp;
    int sizeRtt = 3;
    char targetIp[INET_ADDRSTRLEN] = "\0";;
    char hostIp[INET_ADDRSTRLEN] = "\0";
    char destHostname[NI_MAXHOST] = "\0";
    double time;
    uint8_t ttl = 0;
    int icmp_socket = 0;
    int index = 1;
    struct sigaction act;
    struct ping_struct ping_struct = {
        .sequence = 1,
        .packet_send = 0,
        .packet_receive = 0,
        .packet_loss = 0,
    };

    int ret = check_args(argc, argv, &index);
    if(ret != 0){
        return (ret);
    }

    bzero(&act, sizeof(act));
    act.sa_handler = &sigint_handler;
    
    sigaction(SIGINT , &act, NULL);
    
    ret = getDestAddr(argv[index], &dest_addr, destHostname);
    if(ret != 0){
        return ret;
    }
    inet_ntop(dest_addr.sin_family, &dest_addr.sin_addr, targetIp, sizeof(targetIp));
    get_my_ip(hostIp);
    
    uint8_t *msg = malloc(sizeof(struct icmphdr) + 8);
    struct icmphdr *packet = (struct icmphdr*)msg;
    float *rttptr = malloc(sizeof(float) * sizeRtt);

    printf("PING %s (%s) %d bytes of data\n", argv[index], targetIp, 36);
    double timestart = get_timestamp(); 

    while(!interrupted){

        icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (icmp_socket < 0){
            printf("error creating socket: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return(1);
        }
        create_packet(packet, ping_struct, msg);

        int size = sendto(icmp_socket, packet, 16, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); 
        if(size < 1){
            printf("Error sendto: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return(1);
        }else{
            ping_struct.packet_send += 1;
        }
    
        char buffer[1024] = "\0";
        socklen_t lenDest = sizeof(dest_addr);
    
        size = recvfrom(icmp_socket, buffer, 1024, 0, (struct sockaddr *)&dest_addr, &lenDest);
        if(size < 0){
            printf("error receviving message: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return 1;
        }
        extract_message(buffer, &icmpresp, &time, &ttl);

        if(icmpresp.code == 1 && icmpresp.type == 3){
            printf("From %s icmp_seq=%d Destination Host Unreachable\n", hostIp, ping_struct.sequence);
        }else if(icmpresp.code == 0 && icmpresp.type == 8 && strcmp(hostIp, targetIp)) {
            printf("From %s icmp_seq=%d Destination Host Unreachable\n", hostIp, ping_struct.sequence);
        }
        else{
            float timePacket = (get_timestamp() - time) * 1000;
            printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f ms\n",
                 size, destHostname, targetIp, icmpresp.un.echo.sequence, ttl, timePacket);
            ping_struct.packet_receive += 1;
            add_time_rtt(&rttptr, timePacket, ping_struct.sequence - 1, &sizeRtt);
        }
        close(icmp_socket);
        sleep(1);
        ping_struct.sequence += 1;
    }
    
    print_result_ping(ping_struct, rttptr, ping_struct.sequence, timestart, argv[index]);
    free_msg_rttptr(msg, rttptr);
    return (0);
}