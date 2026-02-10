#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/cdefs.h>

#define ICMP_MINLEN 8

struct ip_header{
    uint8_t version_ihl;
    uint8_t service_type;
    uint16_t totalLength;
    uint16_t identification;
    uint16_t flag_and_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t ip_source;
    uint32_t ip_dest;
};

// struct icmp
// {
//   u_int8_t  icmp_type;
//   u_int8_t  icmp_code;
//   u_int16_t icmp_cksum;
//   uint16_t icmp_identifier;
//   uint16_t icmp_sequence;
// };


// unsigned short checksum(unsigned short *Data, int size)
// {
//     unsigned long somme=0;
//     while(size>1)
//     {
//         somme = somme + *Data++;
//         size = size - sizeof(unsigned short);
//     }

//     if(size)
//         somme = somme + *(unsigned char*)Data;

//     somme = (somme >> 16)+(somme & 0xffff);
//     somme = somme + (somme >> 16);

//     return (unsigned short)(~somme);
// }

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

struct ip_header initialize_header(){
    struct ip_header ip;
    struct sockaddr_in *addr_src = NULL;
    struct sockaddr_in addr_dest;
    
    addr_dest.sin_family = AF_INET;
    addr_dest.sin_port = 0;
    
    ip.version_ihl = 0x45; // represente 0100 4 for the ipv4 and 0101 5 for the header length wich is 20 bytes
    ip.service_type = 0x00;
    ip.totalLength = 0x18;
    ip.identification = 0x0;
    ip.flag_and_offset = 0x0;
    ip.ttl = 0xff;
    ip.protocol = 0x01;
    ip.checksum = 0x000;

    addr_src = get_my_ip();
    inet_aton("80.80.80.80", &(addr_dest.sin_addr));
    printf("address dest %x\n", addr_dest.sin_addr.s_addr);
    printf("address src %x\n", addr_src->sin_addr.s_addr);

    ip.ip_source = addr_src->sin_addr.s_addr;
    ip.ip_dest = addr_dest.sin_addr.s_addr;

    ip.checksum = checksum((unsigned short*)&ip, sizeof(ip));
    printf("the checksum: %x\n", ip.checksum);
    return ip;
}

int main(){
    struct sockaddr_in dest_addr;

    // ip_packet = initialize_header();

    int icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_socket < 0){
        printf("error creating socket");
    }

    struct icmp *packet = malloc(sizeof(struct icmp));
    uint8_t *msg;

    packet->icmp_type = ICMP_ECHO; 
    packet->icmp_code = 0;
    packet->icmp_identifier = 0x1234;
    packet->icmp_sequence = 0x0000;
    packet = (struct icmp*)msg;
    packet->icmp_cksum = checksum(msg, ICMP_MINLEN);
    

    dest_addr.sin_family = AF_INET;
    inet_aton("8.8.8.8", &dest_addr.sin_addr.s_addr);
    dest_addr.sin_port = 0;

    // printf("address dest %x\n", dest_addr.sin_addr.s_addr);
    // printf("address source: %x\n", ip_packet.ip_source);

    if(sendto(icmp_socket, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 1){
        printf("%s\n", strerror(errno));
    }
    return (0);
}