#ifndef PLIST_H
#define PLIST_H

#include <netinet/in.h>
namespace plistNS
{
class plist{
public:
    plist() {}
    plist(const plist &obj) {}
    ~plist() {}

    char hname[28];
    bool isValid;
    sockaddr_in* addr;
};
};

Q_DECLARE_METATYPE(plistNS::plist);

#endif // PLIST_H
