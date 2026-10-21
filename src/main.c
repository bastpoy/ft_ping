#include "../include/header.h"

int interrupted = 0;

static int getDestAddr(char *argv, struct sockaddr_in *addrDest){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(argv, NULL, &hints, &result);
    if(ret != 0){
        fprintf(stderr, "ft_ping: unknown host\n");
        return 1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *temp = (struct sockaddr_in *)rp->ai_addr;
            memcpy(addrDest, (struct sockaddr_in *)rp->ai_addr, sizeof(struct sockaddr_in));
            inet_ntop(rp->ai_family, &(temp->sin_addr), ipstr, sizeof(ipstr));
            freeaddrinfo(result);
            return 0;
        }
    }
    freeaddrinfo(result);
    printf("ping: %s: Temporary failure in name resolution\n", argv);
    return 2;
}

void sigint_handler(int signal)
{
    if (signal == SIGINT){
        interrupted = 1;
    }
}

static void create_packet(struct icmphdr *packet, struct ping_struct ping_struct, uint8_t *msg){
    packet->type = ICMP_ECHO;
    packet->code = 0;
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = htons(ping_struct.sequence);
    packet->checksum = 0;
    packet = (struct icmphdr*)msg;
    double *timestamp = (double *)(packet + 1);
    *timestamp = get_timestamp();
    memset(msg + sizeof(struct icmphdr) + sizeof(double), 0xA5,
        56 - sizeof(double));
    packet->checksum = checksum(msg, 64);
}

static int reverse_dns(struct in_addr addr, char *host, size_t len)
{
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr   = addr;

    if (getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                    host, len, NULL, 0, NI_NAMEREQD) != 0)
        return 0;
    return 1;
}

int is_excedeed_unreach(struct icmphdr *icmp, struct icmphdr *packet, struct iphdr *ip, int size, int ip_header_len, struct ping_ctx ctx){
    struct iphdr   *in_ip   = (struct iphdr *)((char *)icmp + sizeof(struct icmphdr));
    int             in_hlen = in_ip->ihl * 4;
    struct icmphdr *in_icmp = (struct icmphdr *)((char *)in_ip + in_hlen);

    if (in_icmp->un.echo.id != packet->un.echo.id)
        return 1;

    struct in_addr src = { .s_addr = ip->saddr };
    char ip_str[INET_ADDRSTRLEN];
    char host[NI_MAXHOST];
    char from[NI_MAXHOST + INET_ADDRSTRLEN + 4];
    int  len = size - ip_header_len;

    inet_ntop(AF_INET, &src, ip_str, sizeof(ip_str));

    if (reverse_dns(src, host, sizeof(host)))
        snprintf(from, sizeof(from), "%s (%s)", host, ip_str);
    else
        snprintf(from, sizeof(from), "%s", ip_str);

    if (icmp->type == ICMP_DEST_UNREACH && icmp->code == ICMP_HOST_UNREACH)
        printf("%d bytes from %s: Destination Host Unreachable\n",
            len, from);
    else if (icmp->type == ICMP_TIME_EXCEEDED){
        printf("%d bytes from %s: Time to live exceeded\n", len, from);
    }
    if (ctx.args.verbose
    && ((icmp->type == ICMP_DEST_UNREACH && icmp->code == ICMP_HOST_UNREACH)
        || icmp->type == ICMP_TIME_EXCEEDED))
        print_verbose_unreach_excedeed(in_ip, in_icmp);
    return 0;
}

int main(int argc, char *argv[]){
    struct sockaddr_in dest_addr;
    int sizeRtt = 3;
    char targetIp[INET_ADDRSTRLEN] = "\0";
    int indexrtt = 0;
    struct ping_ctx ctx;
    int ret = init_ctx(&ctx, argc, argv);
    if (ret != 0){
        if(ret == PRINT_HELP){
            return 0;
        }
        return ret;
    }

    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = &sigint_handler;
    
    sigaction(SIGINT, &act, NULL);

    ret = getDestAddr(argv[ctx.index], &dest_addr);
    if(ret != 0){
        return ret;
    }
    inet_ntop(dest_addr.sin_family, &dest_addr.sin_addr, targetIp, sizeof(targetIp));

    uint8_t *msg = malloc(sizeof(struct icmphdr) + 56);
    struct icmphdr *packet = (struct icmphdr*)msg;
    float *rttptr = malloc(sizeof(float) * sizeRtt);

    printf("PING %s (%s): %d data bytes", argv[ctx.index], targetIp, 56);
    double timestart = get_timestamp();

    while(interrupted != 1){
        bool isReceive = false;
        print_verbose_id(ctx, packet);
        create_packet(packet, ctx.stats, msg);

        int size = sendto(ctx.socket, packet, 64, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); 
        if(size < 1){
            printf("Error sendto: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return(1);
        }
        ctx.stats.timeSend = get_timestamp();
        printf("timestart: %f\n", ctx.stats.timeSend);
        ctx.stats.packet_send += 1;
        

        while(interrupted != 1 && !isReceive){
            char buffer[1024] = "\0";
            size = recvfrom(ctx.socket, buffer, sizeof(buffer), 0, NULL, NULL);
            if(size < 0){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    ctx.stats.sequence += 1;
                    break;
                }
                if (errno == EINTR) {
                    close(ctx.socket);
                    print_result_ping(ctx.stats, rttptr, indexrtt, timestart, argv[ctx.index]);
                    free_msg_rttptr(msg, rttptr);
                    return errno;
                }
                printf("error receviving message: %s\n", strerror(errno));
                free_msg_rttptr(msg, rttptr);
                return 1;
            }
            
            struct iphdr *ip = (struct iphdr *)buffer;
            int ip_header_len = ip->ihl * 4;
            struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_header_len);
    
            if (icmp->type == ICMP_ECHOREPLY) {
                if (icmp->un.echo.id != packet->un.echo.id)
                    continue;
                double time;
    
                memcpy(&time, (char *)icmp + sizeof(struct icmphdr), sizeof(double));
                float timePacket = (get_timestamp() - time) * 1000;
                printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                        size - ip_header_len, targetIp, ntohs(icmp->un.echo.sequence), ip->ttl, timePacket);
                ctx.stats.packet_receive += 1;
                add_time_rtt(&rttptr, timePacket, indexrtt, &sizeRtt);
                isReceive = true;
            }
            else if(icmp->type == ICMP_DEST_UNREACH || 
                    icmp->type == ICMP_TIME_EXCEEDED){
                if(is_excedeed_unreach(icmp, packet, ip, size, ip_header_len, ctx)){
                    break;
                }
            }
        }
        printf("the time now %f before %f and substract %f\n", get_timestamp(), ctx.stats.timeSend, get_timestamp() - ctx.stats.timeSend);
        if(get_timestamp() - ctx.stats.timeSend < 1.0 ){
            usleep((1 - (get_timestamp() - ctx.stats.timeSend)) * 1000000);
        }
        // sleep(1);
        ctx.stats.sequence += 1;
        indexrtt += 1;
    } 
    
    print_result_ping(ctx.stats, rttptr, indexrtt, timestart, argv[ctx.index]);
    free_msg_rttptr(msg, rttptr);
    return (0);
}