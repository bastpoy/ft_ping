#include "../include/header.h"

void print_help(){
    printf("Usage\n  ping [options] <destination>\n\nOptions:\n"  
        "<destination>\t\tdns name or ip address\n"
        "  -h\t\t\tprint help and exit\n"
        "  --ttl=N\t\tspecify N as time-to-live\n");
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

void print_verbose_id(struct ping_ctx ctx, struct icmphdr *packet){
    if(ctx.stats.sequence == 0){
        if(ctx.args.verbose){
            printf(" id 0x%x = %d", packet->un.echo.id, packet->un.echo.id);
        }
        printf("\n");
    }
}

void print_verbose_unreach_excedeed(struct iphdr *in_ip, struct icmphdr *in_icmp)
{
    int            hlen = in_ip->ihl * 4;
    unsigned char *p    = (unsigned char *)in_ip;
    char           src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    int            i;

    inet_ntop(AF_INET, &in_ip->saddr, src, sizeof(src));
    inet_ntop(AF_INET, &in_ip->daddr, dst, sizeof(dst));

    /* 1. raw hex of the 20-byte header, grouped two bytes by two bytes */
    printf("IP Hdr Dump:\n ");
    for (i = 0; i < (int)sizeof(struct iphdr); i++)
        printf("%02x%s", p[i], (i % 2) ? " " : "");
    printf("\n");

    /* 2. decoded fields */
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
    printf(" %1x  %1x  %02x %04x %04x",
           in_ip->version, in_ip->ihl, in_ip->tos,
           ntohs(in_ip->tot_len), ntohs(in_ip->id));
    printf("   %1x %04x",
           (ntohs(in_ip->frag_off) & 0xe000) >> 13,
           ntohs(in_ip->frag_off) & 0x1fff);
    printf("  %02x  %02x %04x", in_ip->ttl, in_ip->protocol, ntohs(in_ip->check));
    printf(" %s  %s ", src, dst);

    /* IP options, if any (header longer than 20 bytes) */
    for (i = sizeof(struct iphdr); i < hlen; i++)
        printf("%02x", p[i]);
    printf("\n");

    /* 3. ICMP header of the original echo request */
    printf("ICMP: type %u, code %u, size %u, id 0x%04x, seq 0x%04x\n",
           in_icmp->type, in_icmp->code,
           ntohs(in_ip->tot_len) - hlen,
           ntohs(in_icmp->un.echo.id),
           ntohs(in_icmp->un.echo.sequence));
}

double get_timestamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
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