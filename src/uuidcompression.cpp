#include "uuidcompression.h"
#include <QUuid>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

QUuid UUIDCompression::unpackUUID(const unsigned char* src)
{
    QUuid uuid;
    size_t index = 0;
    static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");
    uuid.data1 = ntohl(*(reinterpret_cast<const uint32_t*>(&src[index])));
    index += 4;
    static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
    uuid.data2 = ntohs(*(reinterpret_cast<const uint16_t*>(&src[index])));
    index += 2;
    uuid.data3 = ntohs(*(reinterpret_cast<const uint16_t*>(&src[index])));
    index += 2;
    memcpy(&(uuid.data4), &src[index], 8);
    // index += 8;

    return uuid;
}
size_t UUIDCompression::packUUID(char* buf, const QUuid& uuid)
{
    size_t index = 0;

    uint32_t uuidSectionL = htonl(uuid.data1);
    memcpy(&buf[index], &(uuidSectionL), 4);
    index += 4;
    uint16_t uuidSectionS = htons(uuid.data2);
    memcpy(&buf[index], &(uuidSectionS), 2);
    index += 2;
    uuidSectionS = htons(uuid.data3);
    memcpy(&buf[index], &(uuidSectionS), 2);
    index += 2;
    memcpy(&buf[index], &uuid.data4, 8);
    index += 8;

    return index;
}
