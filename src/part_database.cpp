#include <EEPROM.h> // read and write from flash memory
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <ArduinoOTA.h>

#include "secrets.h"

#define HOSTNAME "ESP-PART-CAM"
#define TEXT_LENGTH 80

#define RECONNECT_DELAY_SECONDS 15
#define RECONNECT_ATTEMPTS 5

char drawerContents[39][TEXT_LENGTH];

AsyncWebServer server(80);

bool connecting = false;

bool OTAInProgress = false;

String OTAStatus = "N/A";

String getPartText(int drawer)
{
  return String(drawerContents[drawer - 1]);
};

String getWiFiStatus()
{
  if (OTAInProgress)
  {
    return OTAStatus;
  }

  switch (WiFi.status())
  {
  case WL_CONNECTED:
    return WiFi.localIP().toString();
  case WL_CONNECTION_LOST:
    return "Connection Lost";
  case WL_DISCONNECTED:
    return "Disconnected";
  default:
    if (connecting)
    {
      return "Connecting...";
    }
    return "Unknown";
    break;
  }
};

void connectToWifi()
{
  connecting = true;
  WiFi.begin(SSID, PASSWORD);
  WiFi.setHostname(HOSTNAME);
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setupServer()
{
  server.on("/set_drawer", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message;
    int drawer;
    if (request->hasParam("drawer", true) && request->hasParam("msg", true))
    {
      message = request->getParam("msg", true)->value();
      drawer = (request->getParam("drawer", true)->value()).toInt();
      message.toCharArray(drawerContents[drawer - 1], TEXT_LENGTH);
      if (drawer > 0 && drawer <= 39)
      {
        EEPROM.writeBytes((drawer - 1) * sizeof(drawerContents[0]), drawerContents[drawer - 1], TEXT_LENGTH);
        EEPROM.commit();
      }

      printf("\n HTTP Set Drawer %d to \"%s\" \n", drawer, drawerContents[drawer - 1]);
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, POST: " + message);
  });

  server.onNotFound(notFound);
  server.begin();
}

void setupOTA()
{
  ArduinoOTA.setHostname("esp32-part-cam");
  ArduinoOTA.setPassword(OTApassword);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
}

void runOTALoop()
{
  ArduinoOTA.handle();
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  connecting = false;
  WiFi.setHostname(HOSTNAME);
  setupServer();
  setupOTA();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  server.end();
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED || attempts > RECONNECT_ATTEMPTS)
  {
    attempts++;
    connectToWifi();
    delay(RECONNECT_DELAY_SECONDS * 1000);
  }
}

void setupEEPROMAndWiFi()
{
  EEPROM.begin(sizeof(drawerContents));
  delay(100);

  for (int i = 0; i <= 39; i++)
  {
    EEPROM.readBytes(i * sizeof(drawerContents[0]), drawerContents[i], TEXT_LENGTH);
  }

  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  connectToWifi();
}
