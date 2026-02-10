
la structure ICMP est la suivante:

struct icmp
{
    u_int8_t  icmp_type; => le type de requete pour nous c'est une demande echo
    u_int8_t  icmp_code; => le code va avec le type et ce sera 0 dans notre cas
    u_int16_t icmp_cksum; => somme de controle permettant de verifier la validite du paquet
};