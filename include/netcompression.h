#ifndef NETCOMPRESSION_H
#define NETCOMPRESSION_H

#include <stddef.h>

#ifdef __WIN32
#include <winsock2.h>
#elif __unix__
#include <netinet/in.h>
#else
#error "include header for sockaddr_in"
#endif

class QUuid;
namespace NetCompression
{
const size_t PACKED_UUID_LENGTH = 16;
size_t packUUID(unsigned char* buf, const QUuid& uuid);
size_t unpackUUID(const unsigned char* src, QUuid &uuid);

const size_t PACKED_SOCKADDR_IN_LENGTH = 6;
size_t packSockaddr_in(unsigned char* buf, const sockaddr_in &addr);
size_t unpackSockaddr_in(const unsigned char* buf, sockaddr_in &addr);
}

#endif  // NETCOMPRESSION_H
