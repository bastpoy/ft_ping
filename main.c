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

int getDestAddr(char *argv, struct sockaddr_in *addrDest, char *buffer){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char ipstr[INET_ADDRSTRLEN];
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // IPv4 ou IPv6
    hints.ai_socktype = SOCK_STREAM; // Une seule famille de socket
    
    int ret = getaddrinfo(argv, NULL, &hints, &result);
    if(ret != 0){
        printf("ping: %s: Name or service not known\n", argv);
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
    return 0;
}

double get_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
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


int main(int argc, char *argv[]){
    struct sockaddr_in dest_addr;
    uint8_t *msg = malloc(sizeof(struct icmphdr) + 8);
    struct icmphdr *packet = (struct icmphdr*)msg;
    char ipstr[INET_ADDRSTRLEN];
    char destHostname[NI_MAXHOST];
    struct icmphdr icmpresp;
    double time;
    uint8_t ttl = 0;
    int icmp_socket = 0;
    uint16_t sequence = 0x0001;

    if(argc  < 2){
        printf("ping: usage error: Destination address required\n");
        return 2;
    }

    if(argc > 2){
        printf("ping: %s: Name or service not known\n", argv[2]);
        return 2;
    }
    
    int ret = getDestAddr(argv[1], &dest_addr, destHostname);
    if(ret != 0){
        return ret;
    }
    inet_ntop(dest_addr.sin_family, &dest_addr.sin_addr, ipstr, sizeof(ipstr));
    printf("PING %s (%s) %d bytes of data\n", argv[1], ipstr, 36);
    while(sequence < 2){

        icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (icmp_socket < 0){
            printf("error creating socket: %s\n", strerror(errno));
            return(1);
        }
        
        packet->type = ICMP_ECHO; 
        packet->code = 0;
        packet->un.echo.id = getpid();
        packet->un.echo.sequence = sequence;
        packet->checksum = 0;
        packet = (struct icmphdr*)msg;
        double *timestamp = (double *)(packet + 1);
        *timestamp = get_timestamp();
        packet->checksum = checksum(msg, 16);
        int size = sendto(icmp_socket, packet, 16, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); 
        if(size < 1){
            printf("Error sendto: %s\n", strerror(errno));
            return(1);
        }
    
        char buffer[1024] = "\0";
        socklen_t lenDest = sizeof(dest_addr);
    
        size = recvfrom(icmp_socket, buffer, 1024, 0, (struct sockaddr *)&dest_addr, &lenDest);
        if(size < 0){
            printf("error receviving message: %s\n", strerror(errno));
            return 1;
        }
        extract_message(buffer, &icmpresp, &time, &ttl);
        printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f\n",
             size, destHostname, ipstr, icmpresp.un.echo.sequence, ttl, (get_timestamp() - time) * 1000);
        
        sequence += 1;
        close(icmp_socket);
        sleep(1);
    }

    printf("--- %s ping statistics ---\n", argv[1]);    

    free(msg);
    return (0);
}