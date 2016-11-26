# PXMessenger
=============
###P2P cross platform home or small office instant messenger.  Written in C++ with Qt.


####Dependencies:

qt5

qtmultimedia

gcc

libevent >= 2.0.22


####BUILD INSTRUCTIONS

git clone

cd ./PXMessenger/src

qmake

make

../PXMessenger

If compiling on Windows, the .pro file will have to be edited to point to your
installation of libevent.  Specifically the lines

win32: LIBS += -L$$PWD/../../libevent-2.0.22-stable/.libs/ -levent

INCLUDEPATH += $$PWD/../../libevent-2.0.22-stable/include

DEPENDPATH += $$PWD/../../libevent-2.0.22-stable/include

####USAGE

PXMessenger is a cross platform instant messaging client that does not need a
central server.  It will automatically discover other users of the client
through the use of a multicast group.  All running clients have a UUID
associated with them for verification purposes and history.  The UUID's are
saved in an .ini file in the users home directory (See QSettings in the Qt5 docs
for more details).  This program can be run multiple times on the same computer
and login.  The Global send item will send the message to all known peers.  It
is essentially a global chat room.  

The "Bloom" setting should not be needed under normal circumstances and only
resends the discovery packed to the multicast group.  This is useful if you
believe that an exisiting group of computers have missed all discovery packets.
(very rare)

PXMessenger will minimize to a tray if the system supports one and will alert
itself in the event of receiving a message.

Bugs are possible, please report them here under the issues tab and they will be
responded to.

Windows executable and setup included in the releases section

Note: While this should theoretically work under Mac OSX, it has never been
tested as I do not have access to a Mac.
