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
        length=ntohs(*tmp16); // network -> local
    }
    int base=10; //bytes
    int lengthHeader=6;

    if (type==PTYPE_DATA) {
        base+=2; // champ length
        lengthHeader+=2;
    }
    if (tr==0) {
        base+=length;
        if (length>0) base+=4; // crc2
    }
    if ((int) len<base) return E_NOMEM;

    // DECODE BEGINS
    pkt->type=(*data)>>6; // ok
    pkt->tr=((*data)>>5) & 1; // ok
    pkt->window=((*data)<<3)>>3; // ok
    uint8_t prevTr=pkt->tr; // ok
    pkt_set_tr(pkt, 0); // pour crc1

    int inc=0; // decalage d'usage si champ longueur
    if (pkt->type==PTYPE_DATA) {
        inc=2;
        uint16_t srcLength;
        memcpy(&srcLength, data+1, sizeof(uint16_t));
        pkt_set_length(pkt, ntohs(srcLength));

    }
    else pkt->length=0;

    memcpy(&(pkt->seqnum), data+1+inc, sizeof(uint8_t));

    memcpy(&(pkt->timestamp), data+2+inc, sizeof(uint32_t));

    
    uint32_t srcCrc1;
    memcpy(&srcCrc1, data+6+inc, sizeof(uint32_t));
    pkt_set_crc1(pkt, ntohl(srcCrc1));
    unsigned char header[lengthHeader];
    memcpy(header, data, lengthHeader);
    if (prevTr==1) header[0]-=32;
    uint32_t crc1= (uint32_t) crc32(0L, Z_NULL, 0);
    crc1 = crc32(crc1, header, lengthHeader);
    if (crc1!=pkt_get_crc1(pkt)) return E_CRC;
    pkt_set_tr(pkt, prevTr);
    if (pkt->tr==0) {
        if (pkt->length>0) {
            memcpy(pkt->payload, data+10+inc, pkt->length);
            
            uint32_t srcCrc2;
            memcpy(&srcCrc2, data+10+inc+pkt->length, sizeof(uint32_t));
            pkt_set_crc2(pkt, ntohl(srcCrc2));
            
            unsigned char *crc2Support=(unsigned char *) pkt->payload;
            uint32_t crc2= (uint32_t) crc32(0L, Z_NULL, 0);
            crc2 = crc32(crc2, crc2Support, pkt->length);
            if (crc2!=pkt_get_crc2(pkt)) return E_CRC;
        }
    }
    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    /* check de la taille */
    int base=10; //bytes
    int lengthHeader=6;
    if (pkt->type==PTYPE_DATA) {
        lengthHeader+=2;
        base+=2;
    }
    if (pkt->tr==0) {
        base+=(pkt->length);
        if (pkt->length>0) base+=4;
    }
    if ((int) *len<base) return E_NOMEM;

    /* encodage */
    *buf=(pkt->type << 6) | (pkt->tr << 5) | (pkt->window);
    int inc=0;
    if (pkt->type==PTYPE_DATA) {
        uint16_t nlength=htons(pkt->length);
        memcpy(buf+1, &nlength, 2);
        inc=2;
    }
    *(buf+1+inc)=pkt->seqnum;
    memcpy(buf+2+inc, &(pkt->timestamp), 4);


    unsigned char header[lengthHeader];
    memcpy(header, buf, lengthHeader);
    if (pkt->tr==1) header[0]-=32;
    uint32_t crc1= (uint32_t) crc32(0L, Z_NULL, 0);
    crc1 = crc32(crc1, header, lengthHeader);
    crc1=htonl(crc1);
    memcpy(buf+6+inc, &crc1, 4);

    if (pkt->tr!=0) {
        *len=10+inc;
        return PKT_OK;
    }
    memcpy(buf+10+inc, pkt->payload, pkt->length);
    if (pkt->length==0) {
        *len=10+inc;
        return PKT_OK;
    }

    unsigned char *crc2Support=(unsigned char *) pkt->payload;
    uint32_t crc2=(uint32_t) crc32(0L, Z_NULL, 0);
    crc2=crc32(crc2, crc2Support, pkt->length);
    crc2=htonl(crc2);
    memcpy(buf+10+inc+pkt->length, &crc2, sizeof(uint32_t));
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
    if (pkt==NULL) printf("br null\n");
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
    if (pkt->tr!=0) return 0;
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
    if (pkt->tr>1) return E_TR;
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
    if (length>512) return E_LENGTH;
    memcpy(pkt->payload, data, length);
    pkt_set_length(pkt, length);
    return PKT_OK;
}

ssize_t predict_header_length(const pkt_t *pkt)
{

    return (pkt->type==PTYPE_DATA) ? 8 : 6;
}


/*int main() {
    const char content[]= {0x5c, 0x00, 0x0b, 0x7b, 0x17, 0x00, 0x00, 0x00, 0x4e, 0xa0, 0x77, 0xdb, 0x68, 0x65, 0x6c, 0x6c,
                     0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x0d, 0x4a, 0x11, 0x85};
    char next[27]={0};
    

    //printf("%x\n", content[0]);
    //pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt);
    pkt_t *test=pkt_new();
    pkt_decode(content, 27, test);
    // 5c : 1011100
    printf("type %d\n", test->type); //ok
    printf("tr %d\n", test->tr); //ok
    printf("window %d\n", test->window); //ok
    printf("length %x\n", test->length); //ok
    printf("seqnum %x\n", test->seqnum);
    printf("timestamp %x\n", test->timestamp);
    printf("crc1 %x\n", test->crc1);
    printf("payload : %s\n", test->payload);
    printf("crc2 : %x\n",test->crc2);





    printf("wtf\n");
    unsigned int *len = malloc(sizeof(unsigned int));
    printf("wtf\n");

    *len=27;
    printf("I am here\n");
    printf("%d\n", pkt_encode(test, next, (size_t *) len));
    printf("hé\n");
    for (int i=0; i<27; i++) {
        printf("it %d\n", i);
        printf("init %x\n", content[i]);
        printf("final %x\n", next[i]);
        printf("\n");
    }
    printf("ptn\n");
}*/

/*int main() {
    pkt_t *pktAck=pkt_new();
    pkt_set_type(pktAck, PTYPE_DATA);
    pkt_set_tr(pktAck, 0);
    pkt_set_window(pktAck, 28);
    pkt_set_seqnum(pktAck, 0x7b); // i sert d'ack pour tous les paquets envoyés
    pkt_set_timestamp(pktAck, 0x17);
    pkt_set_payload(pktAck, "hello world", 0xb); //42+16=57

    unsigned int *len = malloc(sizeof(unsigned int));
    *len=57;
    char next[57]={0};
    int res=pkt_encode(pktAck, next, (size_t *) len);
    printf("code : %d\n", res);
    pkt_t *test=pkt_new();
    pkt_decode(next, 57, test);
    printf("compare the packets : \n");
    //printf("%d\n", pkt_get_length(test));

    printf("crc1 %x\n", pkt_get_crc1(test));
    printf("length %x\n", pkt_get_length(test));
    printf("seqnum %x\n", pkt_get_seqnum(test));
    printf("window %x\n", pkt_get_window(test));
    printf("crc2 %x\n", pkt_get_crc2(test));
    printf("payload : %s\n", pkt_get_payload(test));

}*/