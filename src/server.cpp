#include <server.h>
#include <ESP32WebServer.h>

// not very classy is it?
ESP32WebServer server;
Api* registeredApis = NULL;
unsigned int _numApis = 0;

static void handleApi();

void serverSetup(Api* apis, unsigned int numApis)
{
    _numApis = numApis;
    registeredApis = (Api*)malloc(sizeof(Api) * numApis);
    for (unsigned int apiIdx = 0; apiIdx < numApis; apiIdx++)
    {
        registeredApis[apiIdx] = apis[apiIdx];
        server.on(apis[apiIdx].name, handleApi); 
    }

    server.on("/", handleRoot);
    server.onNotFound ( handleNotFound );
    server.begin();
    Serial.println ( "HTTP server started" );
}

void handleClient()
{
    server.handleClient();
}

static void handleApi()
{
    Serial.print("Got a request for api ");
    Serial.println(server.uri());

    for (unsigned int apiIdx = 0; apiIdx < _numApis; apiIdx++)
    {
        if (registeredApis[apiIdx].name.startsWith(server.uri()))
            server.send(200, "text/html", registeredApis[apiIdx].callback());
    }
}

void handleRoot()
{
    char temp[400];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf(temp, 400,
             "<html>"
             "  <head>"
             "    <meta http-equiv='refresh' content='5'/>"
             "    <title>ESP32 Demo</title>"
             "    <style>"
             "      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }"
             "    </style>"
             "  </head>"
             "  <body>"
             "    <h1>Hello from ESP32!</h1>"
             "    <p>Uptime: %02d:%02d:%02d</p>"
             "  </body>"
             "</html>",
             hr, min % 60, sec % 60);
    server.send(200, "text/html", temp);
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}