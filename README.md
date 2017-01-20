<p align="center">
  <img src="https://github.com/cbpeckles/PXMessenger/raw/master/src/resources/PXMessenger_TightCrop_100px.png"/>
</p>

###P2P cross platform home or small office instant messenger.  Written in C++ with Qt.
-------------

![alt text](http://i.imgur.com/ipp5QwT.png "PXMessenger")

####Dependencies:

qt5 >= qt5.7.1

qtmultimedia

gcc/clang

libevent >= 2.0.22


####BUILD INSTRUCTIONS

git clone

cd ./PXMessenger/src

qmake

make

../PXMessenger

If compiling on Windows, the .pro file will have to be edited to point to your
installation of libevent.  Specifically the lines

```
win32 { LIBS += -L$$PWD/../../libevent-2.0.22-stable/.libs/ -levent

INCLUDEPATH += $$PWD/../../libevent-2.0.22-stable/include
DEPENDPATH += $$PWD/../../libevent-2.0.22-stable/include
}
```

####USAGE

PXMessenger is a cross platform instant messaging client that does not need a
central server.  It will automatically discover other users of the client
through the use of a multicast group.  All running clients have a UUID
associated with them for verification purposes and history.  The UUID's are
saved in an .ini file in the users home directory (See QSettings in the Qt5 docs
for more details).  This program can be run multiple times on the same computer
and login.  The Global send item will send the message to all known peers.  It
is essentially a global chat room.  

#####Settings

The multicast group that PXMessenger uses is 239.192.13.13.  This can be changed
in the settings window.

Adjustments can be made to the ports that are used for PXMessenger if firewall
rules only allow specific ones.  By default, PXMessenger allows the operating
system to choose the TCP port.  The UDP port defaults to 53723.  The UDP port
must be the same for all connected computers however the TCP port can be
whichever you prefer for each. 

The "Bloom" setting should not be needed under normal circumstances and only
resends the discovery packed to the multicast group.  This is useful if you
believe that an exisiting group of computers have missed all discovery packets.
(very rare)

PXMessenger will minimize to a tray if the system supports one and will alert
itself in the event of receiving a message.

#####Notes

Bugs are possible, please report them here under the issues tab and they will be
responded to.

Windows executable and setup included in the releases section

While this should theoretically work under Mac OSX, it has never been
tested as I do not have access to a Mac.

Artwork created by Scott Bolar.
