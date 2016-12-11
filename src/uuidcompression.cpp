#include "uuidcompression.h"

QUuid UUIDCompression::unpackUUID(const unsigned char *src)
{
    QUuid uuid;
    size_t index = 0;
    uuid.data1 = ntohl(*(uint32_t*)(&src[index]));
    index += 4;
    uuid.data2 = ntohs(*(uint16_t*)(&src[index]));
    index += 2;
    uuid.data3 = ntohs(*(uint16_t*)(&src[index]));
    index += 2;
    memcpy(&(uuid.data4), &src[index], 8);
    //index += 8;

    return uuid;
}
size_t UUIDCompression::packUUID(char *buf, QUuid *uuid)
{
    int index = 0;

    uint32_t uuidSectionL = htonl(uuid->data1);
    memcpy(&buf[index], &(uuidSectionL), 4);
    index += 4;
    uint16_t uuidSectionS = htons(uuid->data2);
    memcpy(&buf[index], &(uuidSectionS), 2);
    index += 2;
    uuidSectionS = htons(uuid->data3);
    memcpy(&buf[index], &(uuidSectionS), 2);
    index += 2;
    unsigned char *uuidSectionC = uuid->data4;
    memcpy(&buf[index], uuidSectionC, 8);
    index += 8;

    return index;
}
