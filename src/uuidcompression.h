#ifndef UUIDCOMPRESSION_H
#define UUIDCOMPRESSION_H

#include <QUuid>
#ifdef __WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace UUIDCompression {
const size_t PACKED_UUID_LENGTH = 16;
QUuid unpackUUID(const unsigned char *src);
size_t packUUID(char *buf, QUuid *uuid);
}

#endif // UUIDCOMPRESSION_H
