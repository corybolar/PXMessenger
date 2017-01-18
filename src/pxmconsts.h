#pragma once
#ifndef PXMCONSTS_H
#define PXMCONSTS_H

#include "uuidcompression.h"

namespace PXMConsts {
const int BACKLOG = 200;
const char* const DEFAULT_MULTICAST_ADDRESS = "239.192.13.13";
const int MESSAGE_HISTORY_LENGTH = 500;
const int MIDNIGHT_TIMER_INTERVAL_MINUTES = 1;
#ifdef QT_DEBUG
const int DEBUG_PADDING = 25;
#else
const int DEBUG_PADDING = 0;
#endif
const unsigned short DEFAULT_UDP_PORT = 53273;
const int TEXT_EDIT_MAX_LENGTH = 2000;
const int MAX_HOSTNAME_LENGTH = 24;
const int MAX_COMPUTER_NAME = 36;
const char* const PORT_SEPERATOR = ":::::";
enum MESSAGE_TYPE : const uint32_t {MSG_TEXT = 			0x11111111,
                                    MSG_GLOBAL =		0x22222222,
                                    MSG_SYNC = 			0x33333333,
                                    MSG_SYNC_REQUEST = 	0x44444444,
                                    MSG_AUTH = 			0x55555555,
                                    MSG_NAME = 			0x66666666,
                                    MSG_DISOVER = 		0x77777777,
                                    MSG_ID = 			0x88888888};

const int MAX_UUID_PACKET_LENGTH = 	sizeof(MESSAGE_TYPE) +
                                    UUIDCompression::PACKED_UUID_LENGTH +
                                    MAX_HOSTNAME_LENGTH +
                                    MAX_COMPUTER_NAME +
                                    strlen(PORT_SEPERATOR) +
                                    5/*port number*/ +
                                    1/*null*/;

}

Q_DECLARE_METATYPE(PXMConsts::MESSAGE_TYPE)

#endif // PXMCONSTS_H
