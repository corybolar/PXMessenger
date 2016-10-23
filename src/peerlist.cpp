#include <peerlist.h>

peerClass::peerClass(QWidget *parent)
{

}
void peerClass::sortPeers(int len)
{
    qsort(peers, len, sizeof(peerlist), qsortCompare);
}
int peerClass::qsortCompare(const void *a, const void *b)
{
    peerlist *pA = (peerlist *)a;
    peerlist *pB = (peerlist *)b;
    char temp1[28] = {};
    char temp2[28] = {};

    strcpy(temp1, (pA->hostname.toStdString().c_str()));
    strcpy(temp2, (pB->hostname.toStdString().c_str()));
    int temp1Len = strlen(temp1);
    int temp2Len = strlen(temp2);
    for(int k = 0; k < temp1Len; k++)
    {
        temp1[k] = tolower(temp1[k]);
    }
    for(int k = 0; k < temp2Len; k++)
    {
        temp2[k] = tolower(temp2[k]);
    }
    return strcmp(temp1, temp2);
    /*
    if( strcmp(temp1, temp2) < 0 )
    {
        return -1;
    }
    if( strcmp(temp1, temp2) > 0 )
    {
        return 1;
    }
    */

}
