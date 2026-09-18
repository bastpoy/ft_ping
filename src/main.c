#include "../include/header.h"

int interrupted = 0;

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

int getDestAddr(char *argv, struct sockaddr_in *addrDest, char *buffer, bool verboseMode){
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
            printf("ping: %s: unknown host\n", argv);
        }
        else if (ret == -3){
            printf("ping: %s: Temporary failure in name resolution\n", argv);
        }
        else if (ret == -5){
            printf("ping: %s: No address associated with hostname\n", argv);
        }
        else{
            printf("ping: %s: No address associated with hostname\n", argv);
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
            if(verboseMode)
                printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", argv);
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

void create_packet(struct icmphdr *packet, struct ping_struct ping_struct, uint8_t *msg){
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

int main(int argc, char *argv[]){
    struct sockaddr_in dest_addr;
    struct icmphdr icmpresp;
    int sizeRtt = 3;
    char targetIp[INET_ADDRSTRLEN] = "\0";;
    char hostIp[INET_ADDRSTRLEN] = "\0";
    char destHostname[NI_MAXHOST] = "\0";
    double time;
    uint8_t ttl = 0;
    int ttl1 = 1;
    int icmp_socket = 0;
    int index = 1;
    int indexrtt = 0;
    bool verboseMode = false;
    struct ping_struct ping_struct = {
        .sequence = 1,
        .packet_send = 0,
        .packet_receive = 0,
        .packet_loss = 0,
    };
    
    int ret = check_args(argc, argv, &index, &verboseMode);
    if(ret != 0){
        return (ret);
    }

    icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_socket < 0){
        printf("error creating socket: %s\n", strerror(errno));
        return(1);
    }
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(icmp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (setsockopt(icmp_socket, IPPROTO_IP, IP_TTL, &ttl1, sizeof(ttl1)) < 0) {
        strerror(errno);
        return errno;
    }
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = &sigint_handler;
    
    sigaction(SIGINT, &act, NULL);

    ret = getDestAddr(argv[index], &dest_addr, destHostname, verboseMode);
    if(ret != 0){
        return ret;
    }
    inet_ntop(dest_addr.sin_family, &dest_addr.sin_addr, targetIp, sizeof(targetIp));
    get_my_ip(hostIp);

    uint8_t *msg = malloc(sizeof(struct icmphdr) + 56);
    struct icmphdr *packet = (struct icmphdr*)msg;
    float *rttptr = malloc(sizeof(float) * sizeRtt);

    printf("PING %s (%s) %d(%d) bytes of data\n", argv[index], targetIp, 56, 84);
    double timestart = get_timestamp();

    while(interrupted != 1){
        create_packet(packet, ping_struct, msg);

        // printf("id du paquet envoyé %d et sequence %d\n", packet->un.echo.id, packet->un.echo.sequence);
        int size = sendto(icmp_socket, packet, 64, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); 
        if(size < 1){
            printf("Error sendto: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return(1);
        }else{
            ping_struct.packet_send += 1;
        }
        char buffer[1024] = "\0";
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        size = recvfrom(icmp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender, &sender_len);
        if(size < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                ping_struct.sequence += 1;
                continue;
            }
            if (errno == EINTR) {
                close(icmp_socket);
                break;
            }
            printf("error receviving message: %s\n", strerror(errno));
            free_msg_rttptr(msg, rttptr);
            return 1;
        }
        
        struct iphdr *ip = (struct iphdr *)buffer;
        int ip_header_len = ip->ihl * 4;
        
        extract_message(buffer, &icmpresp, &time, &ttl);
        struct icmphdr *icmp_ptr = (struct icmphdr *)(buffer + ip_header_len);
        
        if(icmp_ptr->un.echo.id == packet->un.echo.id && icmp_ptr->un.echo.sequence == packet->un.echo.sequence){
            if(icmpresp.code == 1 && icmpresp.type == 3){
                printf("From %s icmp_seq=%d Destination Host Unreachable\n", hostIp, ntohs(ping_struct.sequence));
            }else if(icmpresp.code == 0 && icmpresp.type == 8 && strcmp(hostIp, targetIp)) {
                printf("From %s icmp_seq=%d Destination Host Unreachable\n", hostIp, ntohs(ping_struct.sequence));
            }
            else if(icmpresp.code == 0 && icmpresp.type == 0){
                float timePacket = (get_timestamp() - time) * 1000;
                printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f ms\n",
                     size - ip_header_len, destHostname, targetIp, ntohs(icmpresp.un.echo.sequence), ttl, timePacket);
                ping_struct.packet_receive += 1;
                add_time_rtt(&rttptr, timePacket, indexrtt, &sizeRtt);
            }
        }
        else if(extract_time_exceeded(icmp_ptr, ping_struct.sequence)){
            char routerIp[INET_ADDRSTRLEN];
            char routerHostname[NI_MAXHOST] = "\0";
            inet_ntop(AF_INET, &sender.sin_addr, routerIp, sizeof(routerIp));
            
            if (getnameinfo((struct sockaddr *)&sender, sender_len,
            routerHostname, sizeof(routerHostname),
            NULL, 0, NI_NAMEREQD) != 0) {
                strncpy(routerHostname, routerIp, sizeof(routerHostname));
            }
            
            printf("From %s (%s) icmp_seq=%d Time to live exceeded\n", routerHostname, routerIp, ping_struct.sequence);
            ping_struct.sequence += 1;
            sleep(1);
            continue;
        }
        sleep(1);
        ping_struct.sequence += 1;
        indexrtt += 1;
    }
    
    print_result_ping(ping_struct, rttptr, indexrtt, timestart, argv[index]);
    free_msg_rttptr(msg, rttptr);
    return (0);
}