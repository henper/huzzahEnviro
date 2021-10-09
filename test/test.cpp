
#include <unity.h>
#include "updates.h"

void jsonifier_empty()
{
    Updates updates("123456789ABCDEFG");

    char expected[] = "{ \"write_api_key\" : \"123456789ABCDEFG\",\n"
                      "    \"updates\" : [] }";

    char* actual = updates.serialize();
    TEST_ASSERT_EQUAL_STRING(expected, actual);
}

void jsonifier_1entry()
{
    Updates updates("123456789ABCDEFG");

    fields entry = {.delta_t = 0,
                    .field1 = 1};

    updates.add(entry);

    char expected2[] = "{ \"write_api_key\" : \"123456789ABCDEFG\",\n"
                       "    \"updates\" : ["
                       "{ \"delta_t\" : 0, "
                       "\"field1\" : 1.00, "
                       "\"field2\" : 0.00, "
                       "\"field3\" : 0.00, "
                       "\"field4\" : 0.00, "
                       "\"field5\" : 0.00 }] }";

    char* actual2 = updates.serialize();
    TEST_ASSERT_EQUAL_STRING(expected2, actual2);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(jsonifier_empty);
    RUN_TEST(jsonifier_1entry);

    return 0;
}
