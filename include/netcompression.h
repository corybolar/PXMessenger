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
/*!
 * \brief packUUID
 * Compresses and packs a UUID by converting it into Network Byte Order.
 * \param buf supplied buffer to pack into
 * \param uuid uuid to compress
 * \return the size of the packed UUID that was added to buf
 */
size_t packUUID(unsigned char* buf, const QUuid& uuid);
/*!
 * \brief unpackUUID
 * Convert a UUID that was in Network Byte Order back to normal format
 * \param src a buffer where a packed UUID is stored
 * \param uuid a supplied UUID to store the unpacked UUID to
 * \return the size of the unpacked UUID that was read from src
 */
size_t unpackUUID(const unsigned char* src, QUuid &uuid);

const size_t PACKED_SOCKADDR_IN_LENGTH = 6;
/*!
 * \brief packSockaddr_in
 * Pack a sockaddr_in structure into a supplied buffer.  Only the in_addr_t and
 * in_port_t are packed.  IPv4 only!
 * \param buf supplied buffer to store data to
 * \param addr sockaddr_in to pack
 * \return the size of the packed sockaddr_in that was stored in buf
 */
size_t packSockaddr_in(unsigned char* buf, const sockaddr_in &addr);
/*!
 * \brief unpackSockaddr_in
 * Unpacks a sockaddr_in structure that was packed using packSockaddr_in.  IPv4
 * only!
 * \param buf supplied buffer which contains the packed sockaddr_in
 * \param addr a supplied sockaddr_in structure to store the result to
 * \return the size of the unpacked sockaddr_in that was unpacked from buf
 */
size_t unpackSockaddr_in(const unsigned char* buf, sockaddr_in &addr);
}

#endif  // NETCOMPRESSION_H
