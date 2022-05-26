
#include "updates.h"
#include <cstring>

Updates::Updates(const char* api_key)
{
    bufSize = 2048;
    buf = (char*)malloc(bufSize);

    strncpy(key, api_key, sizeof(key));

    reset();

}

void Updates::reset()
{
    numEntries = 0;
    serialized = false;
    memset(buf, 0, bufSize);
    snprintf(buf, bufSize, "{ \"write_api_key\" : \"%s\",\n"
                           "   \"updates\" : [\n", key);
}

char* Updates::serialize()
{
    if (serialized)
        return buf;

    if(numEntries != 0)
    {
        unsigned int i = strnlen(buf, bufSize);
        i -= 2;
        buf[i] = '\0'; // eat ",\n"
        strncat(buf, "\n   ]\n}", bufSize);
    }
    else
    {
        unsigned int i = strnlen(buf, bufSize);
        i -= 1;
        buf[i] = '\0'; // eat "\n"
        strncat(buf, "]\n}", bufSize);
    }
    serialized = true;
    return buf;
}

unsigned int Updates::add(fields entry)
{
    if (serialized)
    {
        unsigned int i = strnlen(buf, bufSize);
        i -= 3;
        buf[i] = '\0'; // eat "] }"
        strncat(buf, ", ", bufSize);
        serialized = false;
    }

    char tmp[256] = {0};
    int charsToWrite = 
        snprintf(tmp, sizeof(tmp),
                "      {\n"
                "         \"delta_t\" : %lu,\n"     // 13+7+2 chars
                "         \"field1\" : %.2f,\n"
                "         \"field2\" : %.2f,\n"
                "         \"field3\" : %.2f,\n"
                "         \"field4\" : %.2f,\n"
                "         \"field5\" : %.2f\n"
                "      },\n",
                entry.delta_t,
                entry.field1,
                entry.field2,
                entry.field3,
                entry.field4,
                entry.field5);

    // TODO: verify charsToWrite

    // 
    size_t bufLen = strnlen(buf, bufSize);
    
    if ((bufLen + charsToWrite + 1) < bufSize)
    {
        strncat(&buf[bufLen], tmp, charsToWrite);
        return ++numEntries;
    }

    // no new entry added!
    return numEntries;
}