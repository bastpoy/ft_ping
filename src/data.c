#include "../include/header.h"

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

// when time excedeed it return the icmp paquet with the source computer infos
//
bool extract_time_exceeded(struct icmphdr *icmp, int expected_seq)
{
    if(icmp->type != ICMP_TIME_EXCEEDED)
        return false;
    
    struct iphdr *original_ip = (struct iphdr *)((uint8_t *)icmp + sizeof(struct icmphdr));
    
    int original_ip_len = original_ip->ihl * 4;
    
    struct icmphdr *original_icmp = (struct icmphdr *)((uint8_t *)original_ip + original_ip_len);
    
    if(original_icmp->type != ICMP_ECHO)
        return false;
    if(original_icmp->un.echo.id != getpid())
        return false;
    if(ntohs(original_icmp->un.echo.sequence) != expected_seq)
        return false;
    return true;
}