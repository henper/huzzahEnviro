#include <cstdlib>
#include <cstdio>
#include <cstring>

typedef struct fields
{
    unsigned long delta_t;
    float field1, field2, field3, field4, field5;
} fields;


class Updates
{
private:
    /* data */
    char* buf;
    unsigned int bufSize, numEntries;
    char key[17];
public:
    Updates(const char* api_key);
    ~Updates() { free(buf); }
    char* serialize();
    int add(fields entry);
    void reset();
};
