#include <peerlist.h>

peerClass::peerClass(QObject *parent) : QObject(parent)
{

}
/**
 * @brief 		use qsort to sort the peerlist structs alphabetically by their hostname
 * @param len	number of peers, this value is held in the peersLen member of Window
 */
void peerClass::sortPeers(int len)
{
    qsort(peers, len, sizeof(peerlist), qsortCompare);
}
/**
 * @brief 		Compare function for the qsort call, could be a one liner
 * @param a		pointer to a member of peers (struct peerlist)
 * @param b		pointer to a member of peers (struct peerlist)
 * @return 		0 if equal, <1 if a before b, >1 if b before a
 */
int peerClass::qsortCompare(const void *a, const void *b)
{
    peerlist *pA = (peerlist *)a;
    peerlist *pB = (peerlist *)b;

    return pA->hostname.compare(pB->hostname, Qt::CaseInsensitive);
}
