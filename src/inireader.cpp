#include "inireader.h"

IniReader::IniReader()
{

}

int IniReader::doPlease(QString filename)
{
    const char *ini_name = filename.toStdString().c_str();
    std::ifstream infile(ini_name);
    if(!infile.good())
    {
        createIni(ini_name);
    }

    //QUuid uuid = checkUUIDs(ini_name);
    //qDebug() << uuid.toString();
    return 1;
}
void IniReader::resetUUID(int num, QUuid uuid, const char* ini_name)
{
    dictionary *ini;
    char key[15];

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return;
    }
    //iniparser_dump(ini, stderr);

    sprintf(key, "UUIDS:%d", num);

    iniparser_set(ini, key, uuid.toString().toStdString().c_str());
    FILE *file = fopen(ini_name, "w");
    iniparser_dump_ini(ini, file);
    fclose(file);
    iniparser_freedict(ini);
}

int IniReader::checkAllowMoreThanOneInstance(const char *ini_name)
{
    dictionary  *ini2;

    /* Some temporary variables to hold query results */
    int b;

    ini2 = iniparser_load(ini_name);
    if (ini2==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }
    //iniparser_dump(ini2, stderr);

    //printf("Config:\n");
    b = iniparser_getboolean(ini2, "Config:AllowMoreThanOneInstance", -1);
    //printf("Config:AllowMoreThanOneInstance: [%d]\n", b);
    iniparser_freedict(ini2);
    return b;
}
QUuid IniReader::getUUID(const char *ini_name, int num)
{
    dictionary  *ini;

    /* Some temporary variables to hold query results */
    const char *s;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return NULL;
    }
    //iniparser_dump(ini, stderr);

    //printf("UUIDS:\n");

    char key[15];
    sprintf(key, "UUIDS:%d", num);
    s = iniparser_getstring(ini, key, NULL);
    //printf("first: [%s]\n", s ? s : "UNDEF");
    QString uuidString = QString::fromUtf8(s);
    uuidString.mid(1,38);
    QUuid uuid = uuidString;
    iniparser_set(ini, key, "INUSE");
    FILE *file = fopen(ini_name, "w");
    iniparser_dump_ini(ini, file);
    fclose(file);
    iniparser_freedict(ini);
    return uuid;
}
int IniReader::getUUIDNumber(const char *ini_name)
{
    dictionary *ini;
    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1;
    }
    iniparser_dump(ini, stderr);

    QUuid uuid = QUuid::createUuid();
    QString uuidString = uuid.toString();
    uuidString.append(";\n");
    char key[15];
    char keynumber[10];
    int i = 0;
    sprintf(key, "UUIDs:%d", i);
    sprintf(keynumber, "%d", i);

    while(iniparser_find_entry(ini, key))
    {
        if(strcmp(iniparser_getstring(ini, key, NULL),"INUSE") == 0)
        {
            i++;
            sprintf(keynumber, "%d", i);
            sprintf(key, "UUIDs:%d", i);
            continue;
        }
        else
        {
            //iniparser_set(ini, key, uuidString.toStdString().c_str());
            iniparser_freedict(ini);
            return i;
        }
    }
    iniparser_set(ini, key, uuidString.toStdString().c_str());
    FILE *file = fopen(ini_name, "w");
    iniparser_dump_ini(ini, file);
    fclose(file);
    iniparser_freedict(ini);

    return i;
}

const char* IniReader::createIni(const char *ini_name)
{
    FILE *ini;

    if((ini=fopen(ini_name, "w"))==NULL)
    {
        std::fprintf(stderr, "iniparser: cannot create ini file");
        return NULL;
    }

    std::fprintf(ini,
                 "#\n"
                 "#Configuration file for PXMessenger\n"
                 "#Written by Cory Bolar\n"
                 "#https://github.com/cbpeckles/PXMessenger\n"
                 "#\n"
                 "\n"
                 "[Config]\n"
                 "AllowMoreThanOneInstance=yes;\n"
                 "\n"
                 "[UUIDs]\n");
    std::fclose(ini);

    return ini_name;
}
