#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

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

int main(){
    struct sockaddr_in dest_addr;
    uint8_t *msg = malloc(sizeof(struct icmphdr));
    struct icmphdr *packet = (struct icmphdr*)msg;

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

    struct timeval start;
    
    gettimeofday(&start, NULL);
    printf("time %lu\n", start.tv_sec);
    printf("time %lu\n", start.tv_usec);
    
    if(sendto(icmp_socket, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 1){
        printf("%s\n", strerror(errno));
        return(1);
    }
    return (0);
}