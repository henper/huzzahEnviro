
#include <vector>
#include <unity.h>
#include <ArduinoJson.h>
#include "updates.h"

std::vector<fields> extractEntries(JsonDocument &doc)
{
    std::vector<fields> entriesActual;
    JsonArray array = doc["updates"];
    for( JsonObject update: array )
    {
        fields e = {.delta_t = update["delta_t"].as<unsigned long>(),
                    .field1 = update["field1"].as<float>(),
                    .field2 = update["field2"].as<float>(),
                    .field3 = update["field3"].as<float>(),
                    .field4 = update["field4"].as<float>(),
                    .field5 = update["field5"].as<float>()};
        entriesActual.push_back(e);
    }

    return entriesActual;
}

void jsonifier_empty()
{
    Updates updates("123456789ABCDEFG");

    char expected[] = "{ \"write_api_key\" : \"123456789ABCDEFG\",\n"
                      "   \"updates\" : []\n}";

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
                       "   \"updates\" : [\n"
                       "      {\n"
                       "         \"delta_t\" : 0,\n"
                       "         \"field1\" : 1.00,\n"
                       "         \"field2\" : 0.00,\n"
                       "         \"field3\" : 0.00,\n"
                       "         \"field4\" : 0.00,\n"
                       "         \"field5\" : 0.00\n"
                       "      }\n"
                       "   ]\n"
                       "}";

    char* actual2 = updates.serialize();
    TEST_ASSERT_EQUAL_STRING(expected2, actual2);
}

void jsonifier_validation()
{
    Updates updates("123456789ABCDEFG");

    const int numEntries = 10;

    std::vector <fields>entries;
    for (int i = 0; i < numEntries; i++)
    {
        fields e = {.delta_t = (unsigned long)i,
                    .field1 = i + 1.0f};
        entries.push_back(e);
        
        updates.add(e);
    }
    
    const char* json = updates.serialize();

    DynamicJsonDocument doc(10*1024);
    DeserializationError error = deserializeJson(doc, json);
    TEST_ASSERT(!error);

    char pretty[10*1024] = {0};
    serializeJsonPretty(doc, pretty);
    printf(pretty);
    printf("\n");

    TEST_ASSERT(doc.containsKey("write_api_key"));
    TEST_ASSERT(doc.containsKey("updates"));
    TEST_ASSERT(numEntries == doc["updates"].size());
    
    std::vector<fields> entriesActual = extractEntries(doc);

    for (int i = 0; i < numEntries; i++)
        TEST_ASSERT(0 == memcmp(&entries[i], &entriesActual[i], sizeof(fields)));

    // add an entry after serialization, before reset
    fields e = {.delta_t = numEntries + 1UL,
                .field1 = numEntries + 2.0f};
    updates.add(e);

    json = updates.serialize();
    doc.clear();
    error = deserializeJson(doc, json);
    TEST_ASSERT(!error);

    TEST_ASSERT(doc["updates"].size() == numEntries + 1);

    entriesActual = extractEntries(doc);
        for (int i = 0; i < numEntries; i++)
        TEST_ASSERT(0 == memcmp(&entries[i], &entriesActual[i], sizeof(fields)));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(jsonifier_empty);
    RUN_TEST(jsonifier_1entry);
    RUN_TEST(jsonifier_validation);

    return 0;
}
