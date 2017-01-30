#include "netcompression.h"
#include <QUuid>
#ifdef _WIN32
#include <winsock2.h>
#elif __unix__
#include <arpa/inet.h>
#else
#error "include header that defines sockaddr_in"
#endif

size_t NetCompression::unpackUUID(const unsigned char* src, QUuid& uuid)
{
    // Use QT implementation of converting to NBO, its better
    uuid = QUuid::fromRfc4122(QByteArray::fromRawData((const char*)&src[0], PACKED_UUID_LENGTH));
    return PACKED_UUID_LENGTH;
}
size_t NetCompression::packUUID(unsigned char* buf, const QUuid& uuid)
{
    // Use QT implementation of converting from NBO, its better
    // because it will return a null uuid if it fails
    memcpy(&buf[0], uuid.toRfc4122().constData(), PACKED_UUID_LENGTH);
    return PACKED_UUID_LENGTH;
}

size_t NetCompression::packSockaddr_in(unsigned char* buf, const sockaddr_in& addr)
{
    int index = 0;
    memcpy(&buf[index], &(addr.sin_addr.s_addr), 4);
    index += sizeof(uint32_t);
    memcpy(&buf[index], &(addr.sin_port), 2);
    index += sizeof(uint16_t);
    return index;
}

size_t NetCompression::unpackSockaddr_in(const unsigned char* buf, sockaddr_in& addr)
{
    int index = 0;
    memcpy(&(addr.sin_addr.s_addr), &buf[index], 4);
    index += sizeof(uint32_t);
    memcpy(&(addr.sin_port), &buf[index], 2);
    index += sizeof(uint16_t);

    return index;
}
