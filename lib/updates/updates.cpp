
#include "updates.h"
#include <cstring>

Updates::Updates(const char* api_key)
{
    bufSize = 1024;
    buf = (char*)malloc(bufSize);

    strncpy(key, api_key, sizeof(key));

    reset();

}

void Updates::reset()
{
    numEntries = 0;
    memset(buf, 0, bufSize);
    snprintf(buf, bufSize, "{ \"write_api_key\" : \"%s\",\n"
                               "    \"updates\" : [", key);
}

char* Updates::serialize()
{
    if(numEntries != 0)
    {
        unsigned int i = strnlen(buf, bufSize);
        i -= 2;
        buf[i] = '\0'; // eat ", "
    }

    strncat(buf, "] }", bufSize);

    return buf;
}

int Updates::add(fields entry)
{
    char tmp[128] = {0};
    snprintf(tmp, sizeof(tmp), "{ \"delta_t\" : %lu, "     // 13+7+2 chars
                                 "\"field1\" : %.2f, "
                                 "\"field2\" : %.2f, "
                                 "\"field3\" : %.2f, "
                                 "\"field4\" : %.2f, "
                                 "\"field5\" : %.2f }, ",
                               entry.delta_t,
                               entry.field1,
                               entry.field2,
                               entry.field3,
                               entry.field4,
                               entry.field5);

    strncat(buf, tmp, bufSize);
    return ++numEntries;
}