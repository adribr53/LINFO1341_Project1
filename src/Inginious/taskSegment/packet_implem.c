#include "packet_interface.h"
#include "stdlib.h"
#include "zlib.h"
#include "string.h"
#include <arpa/inet.h>
/* Extra #includes */
/* Your code will be inserted here */


struct __attribute__((__packed__)) pkt {
    unsigned int type:2; // syntaxe pour faire tenir un entier non signé sur 2 bits
    unsigned int tr:1;
    unsigned int window:5;
    uint16_t length;
    uint8_t seqnum;
    uint32_t timestamp;
    uint32_t crc1;
    char payload[512]; // pas efficace, mais simple
    uint32_t crc2;
};

pkt_t* pkt_new()
{
    pkt_t *toR= (pkt_t *) malloc(sizeof(pkt_t));
    return toR;
}

void pkt_del(pkt_t *pkt)
{
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    // CHECK SIZE
    int type=(*data)>>6;
    int tr=((*data)>>5) & 1;
    int length=0;
    if (type==PTYPE_DATA) {
        uint16_t *tmp16=(uint16_t *) (data+1);
        length=*tmp16;
    }
    int base=10; //bytes
    if (type==PTYPE_DATA) base+=2; // champ length
    if (tr==0) {
        base+=length;
        if (length>0) base+=4; // crc2
    }
    if ((int) len<base) return E_NOMEM;


    // DECODE BEGINS
    pkt->type=((*data)>>6); // on get le type
    if (pkt->type==0) return E_TYPE; // packet invalide

    pkt->tr=((*data)>>5) & 1; // on lit le 6eme bit
    if (pkt->type!=PTYPE_DATA && pkt->tr!=0) return E_TR; // required ?

    pkt->window=((*data)<<3)>>3; // lecture des 5 derniers bits

    int inc=0; // decalage d'usage si champ longueur
    if (pkt->type==PTYPE_DATA) {
        inc=2;
        uint16_t *tmp16=(uint16_t *) (data+1);
        pkt->length=ntohs(*tmp16); // ?
    }
    else pkt->length=0;


    pkt->seqnum=*(data+1+inc); // ok

    uint32_t *tmp32=(uint32_t *) (data+2+inc);
    pkt->timestamp=ntohl(*tmp32); // network byte-order -> host order

    tmp32=(uint32_t *) (data+6+inc);
    pkt->crc1=ntohl(*tmp32);

    /*check du crc1*/
    uint8_t *dataCrc=(uint8_t *) data; // warning sinon
    uint32_t crc = crc32(0L, Z_NULL, 0);
    int i;
    for (i=0; i<=5+inc; i++) {
        if (i==0) {
            uint8_t  tmp8=(*dataCrc)-32;
            crc = crc32(crc, &tmp8, 1);
        } else crc = crc32(crc, dataCrc+i, 1);
    }
    if (crc!=pkt->crc1) return E_CRC;


    if (pkt->tr==0) {
        if (pkt->length>0) {
            memcpy(pkt->payload, data+10+inc, pkt->length);
            tmp32=(uint32_t *)(data+10+inc+pkt->length);
            pkt->crc2=ntohl(*tmp32);
            crc = crc32(0L, Z_NULL, 0);
            int j;
            for (j=0; j<pkt->length; j++) {
                crc = crc32(crc, dataCrc+j+10+inc, 1);
            }
            if (crc!=pkt->crc2) return E_CRC;
        }
    }

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    /* check de la taille*/
    int base=10; //bytes
    if (pkt->type==PTYPE_DATA) base+=2;
    if (pkt->tr==0) {
        base+=(pkt->length);
        if (pkt->length>0) base+=4;
    }
    if ((int) *len<base) return E_NOMEM;


    *buf=(pkt->type << 6) | (pkt->tr << 5) | (pkt->window); // ok
    int inc=0;
    if (pkt->type==PTYPE_DATA) {
        memcpy(buf+1, &(pkt->length), 2);
        inc=2;
    }
    *(buf+1+inc)=pkt->seqnum;

    memcpy(buf+2+inc, &(pkt->timestamp), 4);
    memcpy(buf+6+inc, &(pkt->crc1), 4);
    if (pkt->tr!=0) {
        *len=10+inc;
        return PKT_OK;
    }
    memcpy(buf+10+inc, pkt->payload, pkt->length);
    if (pkt->length==0) {
        *len=10+inc;
        return PKT_OK;
    }

    memcpy(buf+10+inc+pkt->length, &(pkt->crc2), 4);
    *len=16+pkt->length;
    return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    if (pkt->tr!=0)return 0;
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
    if (pkt->tr!=0) return NULL;
    return pkt->payload;
}



pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if (type==0) return E_TYPE;
    pkt->type=type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    if (pkt->type!=PTYPE_DATA && tr!=0) return E_TR;
    pkt->tr=tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if (window > 31) return E_WINDOW;
    pkt->window=window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    if (pkt->seqnum >= pkt->window) return E_SEQNUM;
    pkt->seqnum=seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if (length>512) return E_LENGTH;
    pkt->length=length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    pkt->timestamp=timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1=crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2=crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{
    memcpy(pkt->payload, data, length);
    pkt->length=length;
    return PKT_OK;
}

ssize_t predict_header_length(const pkt_t *pkt)
{
    if (pkt->type!=PTYPE_DATA) return 0;
    return pkt->length;
}

