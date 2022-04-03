/*
    ESP8266 WIFI Manager specific to my use case.
    Saves Data to the EEPPROM.
    HTML element can be a input type of TEXT of CHECKBOX
    uses a dynamic array to generate the HTML elements to display.
    The elements should map to an eeprom variable

    Created by Trey Aughenbaugh
    https://github.com/Invisibleman1002/dynaHTML

    version 1.1.0
    * Thanks to the work done by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager_Lite

  Version Modified By   Date        Comments
  ------- -----------  ----------   -----------
  1.0.0   Trey A       03/31/2022   Initial coding for ESP8266
  1.1.0   Trey A       04/01/2022   Added Grouping

  ! TODO
    Convert this to a class.
*/
#ifndef DYNAHTML_H
#define DYNAHTML_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

typedef enum
{
  e_INPUT = 0,

  e_CHECK = 1, // for a Checkbox, allow only 1 / 0 for expected value when checked or not
} HTML_ELEMENT;

#define MAX_ID_LEN 6
#define MAX_DISPLAY_NAME_LEN 16

typedef struct
{
  char id[MAX_ID_LEN + 1];
  char displayName[MAX_DISPLAY_NAME_LEN + 1];
  char *pdata;
  HTML_ELEMENT HT_EM;
  uint8_t group;

} MenuItem;
// MenuItem allItem[] = {{"wiid", "SSID", MyconfigData.wifi_ssid, e_INPUT, 0},
//                       {"wipw", "Password", MyconfigData.wifi_pw, e_INPUT, 0},
//                       {"msrv", "MQTT Server", MyconfigData.mqtt_server, e_INPUT, 1},
//                       {"ss", "Sensor Publish", MyconfigData.sensorstatus, e_INPUT, 2},
//                       {"sn", "Sensor Name", MyconfigData.sensorname, e_INPUT, 2},
//                       {"mqi", "MQTT ID", MyconfigData.mqtt_id, e_INPUT, 1},
//                       {"mqp", "MQTT PW", MyconfigData.mqtt_key, e_INPUT, 1},
//                       {"powr", "USB Power", MyconfigData.usb_power, e_CHECK, 2}};

// -- HTML page fragment
// uint16_t NUM_MENU_ITEMS = sizeof(allItem) / sizeof(MenuItem);
// EXTERN because the functions are Created in the other file.  THis says, USE THEM otherwise you get out of scope error!  COOL.
// extern void saveconfigtoEE(configData MyconfigData);
// extern void saveAPEE(apData MyAPdata);
// extern configData MyconfigData;
// extern apData MyAPdata;
//#define CALLBACK_SIGNATURE std::function<void()> callback
typedef std::function<void(void)> callback_function_t;
// MenuItem allItem[] = {};
class dynaHTML
{
public:
  dynaHTML();
  void createHTML(String &root_html_template);
  void handleRequest(AsyncWebServerRequest *request);
  void setCallback(callback_function_t callback);
  uint16_t setMenuItems(MenuItem aItem[], uint16_t menucount);
  // callback(topic,payload,len-llen-3-tl-2);
private:
  // CALLBACK_SIGNATURE;
  int my_min(int a, int b);
  int my_max(int a, int b);
  MenuItem *allItem;
  uint16_t NUM_MENU_ITEMS;
  void resetFunc();

protected:
  callback_function_t _callback_function = nullptr;
};

#endif