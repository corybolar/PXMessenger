#ifndef UUIDCOMPRESSION_H
#define UUIDCOMPRESSION_H

#include <stddef.h>

class QUuid;
namespace UUIDCompression {
const size_t PACKED_UUID_LENGTH = 16;
QUuid unpackUUID(const unsigned char *src);
size_t packUUID(char *buf, const QUuid &uuid);
}

#endif // UUIDCOMPRESSION_H
