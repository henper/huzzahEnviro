
#include <unity.h>
#include <ArduinoJson.h>
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

void jsonifier_validation()
{
    Updates updates("123456789ABCDEFG");

    fields entry1 = {.delta_t = 0,
                     .field1 = 1};
    updates.add(entry1);

    fields entry2 = {.delta_t = 1,
                     .field1 = 2};
    updates.add(entry2);

    char* json = updates.serialize();

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json);
    TEST_ASSERT(!error);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(jsonifier_empty);
    RUN_TEST(jsonifier_1entry);
    RUN_TEST(jsonifier_validation);

    return 0;
}
