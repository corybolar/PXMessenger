/** @file pxmconsts.h
 * @brief various constants used throughout pxmessenger
 */

#ifndef PXMCONSTS_H
#define PXMCONSTS_H

#include "netcompression.h"

#include <stdint.h>
#include <string.h>

#include <QMetaType>
#include <QPalette>

static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

namespace PXMConsts
{
const char DEFAULT_MULTICAST_ADDRESS[]       = "239.192.13.13";
const int MESSAGE_HISTORY_LENGTH             = 500;
#ifdef QT_DEBUG
const size_t DEBUG_PADDING = 23;
#else
const size_t DEBUG_PADDING = 0;
#endif
const unsigned short DEFAULT_UDP_PORT = 53273;
const int TEXT_EDIT_MAX_LENGTH        = 2000;
const size_t MAX_HOSTNAME_LENGTH      = 24;
const size_t MAX_COMPUTER_NAME_LENGTH        = 36;
const char AUTH_SEPERATOR[]           = ":::";
enum MESSAGE_TYPE : const uint32_t {
    MSG_TEXT         = 0x11111111,
    MSG_GLOBAL       = 0x22222222,
    MSG_SYNC         = 0x33333333,
    MSG_SYNC_REQUEST = 0x44444444,
    MSG_AUTH         = 0x55555555,
    MSG_NAME         = 0x66666666,
    MSG_DISOVER      = 0x77777777,
    MSG_ID           = 0x88888888,
    MSG_TYPING 	     = 0x99999999,
    MSG_TEXTENTERED  = 0x21132398,
    MSG_ENDTEXTENTERED  = 0x29932398
};
//This works around a compiler warning in pxmserver when building a reply
//message for udp discover packets.  See updReceive in pxmserver.cpp
constexpr size_t ct_strlen(const char* s) noexcept
{
    return *s ? 1 + ct_strlen(s + 1) : 0;
}
const size_t MAX_AUTH_PACKET_LEN =
    sizeof(MESSAGE_TYPE) +
    NetCompression::PACKED_UUID_LENGTH +
    MAX_HOSTNAME_LENGTH +
    MAX_COMPUTER_NAME_LENGTH +
    strlen(AUTH_SEPERATOR) +
    5 /*port number*/ +
    strlen(AUTH_SEPERATOR) +
    strlen("001.001.001") + /*App Version*/
    strlen(AUTH_SEPERATOR) +
    5 /*ssl port number*/ +
    1 /*null*/;
}
Q_DECLARE_METATYPE(PXMConsts::MESSAGE_TYPE)

#endif  // PXMCONSTS_H
