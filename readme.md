# ft_ping

couche 3 du modele osi au meme niveau que le protocole Ip

echo request    type 8
echo reply      type 0


le paulor

## ICMP PAQUET

entete ip   20 octets
entete imcp 8 octets
Payload     56 octet

le payload contient le timestamp de l'envoi du paquet.

* type=00 et code=00 : echo response
* type=03 et code=01 : host unreachable
* type=11 et code=00 : time excedeed

[ outer IP header ][ ICMP type 11 | code 0 | cksum | unused ][ original IP header ][ first 8 bytes of your original ICMP echo ]

* getaddrinfo
* getnameinfo
* inet_ntop
* inet_ntoa
* sento
* recvfrom
* socket
