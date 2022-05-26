/***************************************************************************
 *                                                                         *
 ***************************************************************************/

#include <WiFi.h>
#include "secrets.h"
#include "server.h"
#include "updates.h"

#include <Wire.h>
//#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_PM25AQI.h"

#include <limits>

#define SEALEVELPRESSURE_HPA (1013.25)
#define CUBE(x) (x*x*x)

static String printValues();

String nitrousOxide();
String pendingUpdates();
void particulateMatter(float* pm25, float* pm100);
void hydroThermoBaric(float* temp, float* pressure, float* humidity);

Adafruit_BME280 bme; // I2C
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

unsigned long samplingInterval = 1000;
unsigned long updateInterval = 2*1000;

Updates updates(THNGSPK_WRITE_API_KEY);
WiFiClient client;

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RX, TX);

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start web server
    Api apis[] = {
                    {.name = "/serialLog",
                     .callback = printValues},
                    {.name = "/pendingUpdates",
                     .callback = pendingUpdates}
                 };
    serverSetup(apis, sizeof(apis)/sizeof(apis[0]));


    // PM2.5 sensor
    if(!aqi.begin_UART(&Serial1)) {
        Serial.println("Could not find PM 2.5 sensor!");
    }

    // enable and reset pins brought high for whatever reason?
    pinMode(A9, OUTPUT);
    pinMode(A10, OUTPUT);
    digitalWrite(A9, HIGH);
    digitalWrite(A10, HIGH);

    // BME280 sensor
    Serial.println(F("BME280 test"));

    if (! bme.begin(0x76, &Wire)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
    }

    /*
    Serial.println("-- Default Test --");
    Serial.println("normal mode, 16x oversampling for all, filter off,");
    Serial.println("0.5ms standby period");
    */  
    
    // For more details on the following scenarious, see chapter
    // 3.5 "Recommended modes of operation" in the datasheet
    

    // weather monitoring
    Serial.println("-- Weather Station Scenario --");
    Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
    Serial.println("filter off");
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
                      
    // suggested rate is 1/60Hz (1m)

/*    
    // humidity sensing
    Serial.println("-- Humidity Sensing Scenario --");
    Serial.println("forced mode, 1x temperature / 1x humidity / 0x pressure oversampling");
    Serial.println("= pressure off, filter off");
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_NONE, // pressure
                    Adafruit_BME280::SAMPLING_X1,   // humidity
                    Adafruit_BME280::FILTER_OFF );
                      
    // suggested rate is 1Hz (1s)
    delayTime = 1000;  // in milliseconds
*/

/*    
    // indoor navigation
    Serial.println("-- Indoor Navigation Scenario --");
    Serial.println("normal mode, 16x pressure / 2x temperature / 1x humidity oversampling,");
    Serial.println("0.5ms standby period, filter 16x");
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X2,  // temperature
                    Adafruit_BME280::SAMPLING_X16, // pressure
                    Adafruit_BME280::SAMPLING_X1,  // humidity
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5 );
    
    // suggested rate is 25Hz
    // 1 + (2 * T_ovs) + (2 * P_ovs + 0.5) + (2 * H_ovs + 0.5)
    // T_ovs = 2
    // P_ovs = 16
    // H_ovs = 1
    // = 40ms (25Hz)
    // with standby time that should really be 24.16913... Hz
    delayTime = 41;
    */
    
    /*
    // gaming
    Serial.println("-- Gaming Scenario --");
    Serial.println("normal mode, 4x pressure / 1x temperature / 0x humidity oversampling,");
    Serial.println("= humidity off, 0.5ms standby period, filter 16x");
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_X4,   // pressure
                    Adafruit_BME280::SAMPLING_NONE, // humidity
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5 );
                      
    // Suggested rate is 83Hz
    // 1 + (2 * T_ovs) + (2 * P_ovs + 0.5)
    // T_ovs = 1
    // P_ovs = 4
    // = 11.5ms + 0.5ms standby
    delayTime = 12;
*/

    // make sure nothing is waiting in the client
    if (client.available())
    {
        Serial.print("Junk in client: ");
        while (client.available())
            Serial.print((char)client.read());
        Serial.println("");
    }

    Serial.println();
}


void loop()
{
    unsigned long startTime = millis();

    // Take care of any incoming HTTP requests
    //handleClient();

    static unsigned long lastUpdateTime = startTime; // next update in updateInterval
    if (startTime - lastUpdateTime >= updateInterval)
    {
        lastUpdateTime = startTime;

        if(client.connect("api.thingspeak.com", 80))
        {
            char* json = updates.serialize();

            String JSON = String(json);
            updates.reset();

            Serial.println(JSON);

            client.println(String("POST /channels/"+THNGSPK_CHANNEL_ID+"/bulk_update.json HTTP/1.1"));
            client.println("Host: api.thingspeak.com");
            client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
            client.println("Connection: close");
            client.println("Content-Type: application/json");
            client.println("Content-Length: "+JSON.length()+1);
            client.println();
            size_t numCharsWritten = client.println(json);

            //Serial.print("JSON data available to wrte: "); Serial.println(strlen(json));
            //Serial.print("JSON data written to client: "); Serial.println(numCharsWritten);

            //for (int i = 0; i < strlen(json); i++)
            //    client.print(json[i]);

            while (client.connected())
            {
                if (client.available())
                    Serial.print((char)client.read());
            }

            //Serial.println(client.parseFloat()); // wtf?
            //String resp = String(client.parseInt());
            //Serial.println("Response code:"+resp); // Print the response code. 202 indicates that the server has accepted the response

            updates.reset();
        }
        else
        {
            Serial.println("Failed to connect to ThingSpeak");
        }     
    }

    static unsigned long lastSamplingTime = startTime - samplingInterval; // next sample, now!
    if (startTime - lastSamplingTime >= samplingInterval)
    {
        lastSamplingTime = startTime;

        // Only needed in forced mode! In normal mode, you can remove the next line.
        bme.takeForcedMeasurement(); // has no effect in normal mode

        fields entry = {0};
        entry.delta_t = lastSamplingTime / 1000;
        hydroThermoBaric(&entry.field1, &entry.field2, &entry.field3);
        particulateMatter(&entry.field4, &entry.field5);

        if (updates.getNumEntries() == updates.add(entry))
            Serial.println("ERROR: updates buffer full!");
    }

    // sleep until next action
    unsigned long executionTime, endTime = millis();
    
    if (endTime > startTime)
        executionTime = endTime - startTime;
    else
    {
        executionTime = ULONG_MAX - startTime + endTime; // account for wrap-around
    }
    unsigned long sleepTime = (samplingInterval - executionTime);

    Serial.print("Sleep time: "); Serial.println(sleepTime);
    delay(sleepTime);

    //esp_sleep_enable_timer_wakeup((samplingInterval - executionTime) * 1000);
    //esp_light_sleep_start();

}

void particulateMatter(float* pm25, float* pm100)
{
    PM25_AQI_Data data;

    if (! aqi.read(&data)) {
        Serial.println("Could not read from AQI");
        //return "";
    }

    /* PMS5003 sensor returns standardized PM2.5 and PM10 AQI measurements in µg/m^3
     * but without precision or explanation.
     *
     * Method here is developed and explained in DOI:10.13140/RG.2.2.17093.06884
     * "A transparent method of calculating PM2.5 concentrations from Plantower sensors:
     *  comparison of bias, precision, and limit of detection with the Plantower CF1 data series"
     * 
     * PM2.5 is the measure of particles less than 2.5µm but sensor reports n particles 'larger than'.
     * 1. Split particles in categories and their (geometric) mean sizes.
     * 2. Calculate total mass of all particle sizes, assume density of water (~1 kg/L)
     */
    int numParticles[] = {  data.particles_03um - data.particles_05um,     // particules between 0.3 µm and  0.5 µm
                            data.particles_05um - data.particles_10um,     // particules between 0.5 µm and  1.0 µm
                            data.particles_10um - data.particles_25um,     // particules between 1.0 µm and  2.5 µm
                            data.particles_25um - data.particles_50um,     // particules between 2.5 µm and  5.0 µm
                            data.particles_50um - data.particles_100um  }; // particules between 5.0 µm and 10.0 µm

    // particle count from sensor is per L, 1000 liters in a cubic meter, geometric mean Diameter of particles and volume of sphere
    static float const particleMassConcFactor[] = { 1000 * PI * CUBE(sqrt(0.3 *  0.5)) / 6,
                                                    1000 * PI * CUBE(sqrt(0.3 *  0.5)) / 6,
                                                    1000 * PI * CUBE(sqrt(0.3 *  0.5)) / 6,
                                                    1000 * PI * CUBE(sqrt(0.3 *  0.5)) / 6,
                                                    1000 * PI * CUBE(sqrt(0.3 *  0.5)) / 6 };

    float particleMassConcentrations[5];
    for (int i = 0; i < 5; i++)
    {
        particleMassConcentrations[i] = numParticles[i] * particleMassConcFactor[i];
    }

    *pm25 = *pm100 = 0.0;
    for (int i = 0; i < 3;  *pm25 += particleMassConcentrations[i++]);
    for (int i = 0; i < 5; *pm100 += particleMassConcentrations[i++]);

    Serial.print("PM2.5 calculated: "); Serial.print(*pm25);
    Serial.print(" PM10 calculated: "); Serial.println(*pm100);

    /* char update[128]; //78
    snprintf(update, sizeof(update), "{ \"delta_t\" : %lu, "     // 13+7+2 chars
                                       "\"field4\" : %u, "
                                       "\"field5\" : %u }, ",
                                    millis()/1000,
                                    data.pm25_standard,
                                    data.pm100_standard);

    return update; */
}

String nitrousOxide()
{
    // enable the mics6814 sensor, 
    //digitalWrite(A4, true); not an output capable pin on huzzah32 :(
    // possible maybe wire it to 3V ?

    // ADC2 exclusively used by WiFi, prohibiting A0 and A1 :(
    // possibly maybe disable wifi to read ?
    //int nh3 = analogRead(A0);
    //int red = analogRead(A1); 
    int oxi = analogRead(A2);

    char response[128];
    snprintf(response, sizeof(response), "{ \"nh3\" : %d, "
                                            "\"red\" : %d, "
                                            "\"oxi\" : %d }, ",
                                        0, 0, oxi);
    return response;
}

void hydroThermoBaric(float* temp, float* pressure, float* humidity)
{
    //printValues();
    /* char update[128]; //78
    snprintf(update, sizeof(update), "{ \"delta_t\" : %lu, "     // 13+7+2 chars
                                       "\"field1\" : %.2f, "     // 11+5+2 chars
                                       "\"field2\" : %.2f, "     // 11+7+2 chars
                                       "\"field3\" : %.2f }, ",  // 11+5+2+2 chars
                                    millis()/1000,
                                    bme.readTemperature(),
                                    bme.readPressure(),
                                    bme.readHumidity());
    return String(update); */

    *temp     = bme.readTemperature();
    *pressure = bme.readPressure();
    *humidity = bme.readHumidity();
}

String pendingUpdates()
{
    return updates.serialize();
}

static String printValues() {
    // bme280
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();

    // pms5003
    PM25_AQI_Data data;
    if (! aqi.read(&data)) {
        Serial.println("Could not read from AQI");
        return "";
    }
    Serial.println("AQI reading success");

    Serial.println();
    Serial.println(F("---------------------------------------"));
    Serial.println(F("Concentration Units (standard)"));
    Serial.println(F("---------------------------------------"));
    Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_standard);
    Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_standard);
    Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_standard);
    Serial.println(F("Concentration Units (environmental)"));
    Serial.println(F("---------------------------------------"));
    Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_env);
    Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_env);
    Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_env);
    Serial.println(F("---------------------------------------"));
    Serial.print(F("Particles > 0.3um / 0.1L air:")); Serial.println(data.particles_03um);
    Serial.print(F("Particles > 0.5um / 0.1L air:")); Serial.println(data.particles_05um);
    Serial.print(F("Particles > 1.0um / 0.1L air:")); Serial.println(data.particles_10um);
    Serial.print(F("Particles > 2.5um / 0.1L air:")); Serial.println(data.particles_25um);
    Serial.print(F("Particles > 5.0um / 0.1L air:")); Serial.println(data.particles_50um);
    Serial.print(F("Particles > 10 um / 0.1L air:")); Serial.println(data.particles_100um);
    Serial.println(F("---------------------------------------"));

    return "Immideate values written to serial console!";
}
