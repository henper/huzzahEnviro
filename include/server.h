#ifndef SERVER_H
#define SERVER_H

#include <Arduino.h>

typedef struct Api
{
    String name;
    String (*callback)(void);  //json?
} Api;

void serverSetup(Api* apis, unsigned int numApis);
void handleClient();
void handleRoot();
void handleNotFound();

#endif