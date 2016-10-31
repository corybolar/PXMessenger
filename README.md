# PXMessenger
=============
###P2P cross platform home or small office instant messenger.  Written in C++ with Qt.
###Now with comments!


####Dependencies:

qt5

qtmultimedia

gcc


####BUILD INSTRUCTIONS

git clone

cd ./PXMessenger/src

qmake

make

../PXMessenger

####USAGE

PXMessenger is a cross platform instant messaging client that does not need a
central server.  It will automatically discover other users of the client
through the use of a multicast group.  All running clients have a UUID
associated with them for verification purposes and history.  The UUID's are
saved in an .ini file in the users home directory (See QSettings in the Qt5 docs
for more details).  This program can be run multiple times on the same computer
and login.  The Global send item will send the message to all known peers.  It
is essentially a global chat room.  

PXMessenger will minimize to a tray if the system supports one and will alert
itself in the event of receiving a message.

Bugs are possible, please report them here under the issues tab and they will be
responded to.

##UNDER HEAVY DEVELOPMENT

Windows executable included in the releases section
