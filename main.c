#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define ICMP_MINLEN 8

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
    // Carry the previous add.
    
    result = (result >> 16) + (result & 0xffff);
    return ~result;
}


struct sockaddr_in *get_my_ip(){
    struct ifaddrs *host_addrs = NULL;
    struct sockaddr_in *addr= NULL;

    int ret = getifaddrs(&host_addrs);
    if(ret == -1){
        printf("Error getting host addresses");
        return(NULL);
    }
    while(host_addrs){

        sa_family_t family = host_addrs->ifa_addr->sa_family;
        addr = (struct sockaddr_in*)host_addrs->ifa_addr;

        if(family == AF_INET){
            printf("host_addrs name: -%s-\n", host_addrs->ifa_name);
            if(strcmp(host_addrs->ifa_name, "wlp2s0") == 0){
                char buffer[14];
                if(!inet_ntop(AF_INET, &addr->sin_addr, buffer, 14)){
                    printf("Error converting address ton string");
                    return NULL;
                }
                printf("the address is: %s\n", buffer);
                printf("the address is %x \n\n", addr->sin_addr.s_addr);
                return addr;
            }
        }
        host_addrs = host_addrs->ifa_next;
    }
    return NULL;
}

void getDestAddr(char *argv){
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if(getaddrinfo(NULL, argv, &hints, &result) < 0){
        printf("Error resolving host address\n");
        return;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        
    }
}

unsigned long long concatenate(unsigned long long x, unsigned y) {
    unsigned long long pow = 10;
    while(y >= pow){
        pow *= 10;
    }
    return x * pow + y;        
}

unsigned long long getTimeInMs(){
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);
    unsigned int timems = time.tv_nsec / 1000000;
    int nDigits = floor(log10(timems)) + 1;
    unsigned long long timetime = 0;
    if (nDigits < 3){
        timetime = time.tv_sec * 10;
        timetime = concatenate(timetime, timems);
    }
    else
        timetime = concatenate(time.tv_sec, timems);
    return timetime;
}

void extract_message(void *pkt, size_t pkt_len, struct icmphdr *msg,
                     size_t *msg_len) {
    // Skip the IPv4 header if it's present.
    if (pkt_len > sizeof(struct ip)) {
        struct ip *ip_pkt = (struct ip *)pkt;
        if (ip_pkt->ip_v == IPVERSION) {
            int iplen = 4 * ip_pkt->ip_hl;
            pkt += iplen;
            pkt_len -= iplen;
        }
    }
    if (*msg_len < pkt_len) { // Avoid buffer overrun
        pkt_len = *msg_len;
    }
    *msg_len = pkt_len;
    memcpy(msg, pkt, *msg_len);
}


int main(int argc, char *argv[]){
    struct sockaddr_in dest_addr;
    uint8_t *msg = malloc(sizeof(struct icmphdr));
    struct icmphdr *packet = (struct icmphdr*)msg;

    if(argc  < 2){
        printf("ping: usage error: Destination address required\n");
        return 2;
    }

    if(argc > 2){
        printf("ping: %s: Name or service not known\n", argv[2]);
        return 2;
    }

    getDestAddr(argv[1]);

    int icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_socket < 0){
        printf("error creating socket: %s\n", strerror(errno));
        return(1);
    }
    
    // Create the ICMP packet
    packet->type = ICMP_ECHO; 
    packet->code = 0;
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = 0x0000;
    packet->checksum = 0;
    packet = (struct icmphdr*)msg;
    packet->checksum = checksum(msg, ICMP_MINLEN);

    printf("the checksum is: %x and pid is: %d\n", packet->checksum, packet->un.echo.id);
    
    //Create the destination address
    dest_addr.sin_family = AF_INET;
    inet_aton("8.8.8.8", &dest_addr.sin_addr);
    dest_addr.sin_port = 0;

    if(sendto(icmp_socket, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 1){
        printf("%s\n", strerror(errno));
        return(1);
    }

    char buffer[1024] = "\0";
    socklen_t lenDest = sizeof(dest_addr);

    int size = 0;
    size = recvfrom(icmp_socket, buffer, 1024, 0, (struct sockaddr *)&dest_addr, &lenDest);
    if(size < 0){
        printf("error receviving message: %s\n", strerror(errno));
        return 1;
    }
    printf("Return value %02x%02x%02x%02x and size %d\n", buffer[0], buffer[1], buffer[2], buffer[3], size);
    union {
        struct icmphdr msg;
        char msgbuf[IP_MAXPACKET];
    } u;


    size_t msg_len = sizeof(u);
    extract_message(buffer, size, &u.msg, &msg_len);

    printf("response icmp packet type: %x\n", u.msg.type);
    printf("response icmp id: %x\n", u.msg.code);

    return (0);
}