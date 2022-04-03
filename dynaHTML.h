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

const char content_Type[] = "text/html";

const char html_top[] = R"rawlit(<!DOCTYPE html>
<html>
  <head>
    <title>Sensor Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>)rawlit";
const char MiligramStyle[] = "*,:after,:before{box-sizing:inherit}html{box-sizing:border-box;font-size:62.5%}body{color:#606c76;font-family:Roboto,'Helvetica Neue',Helvetica,Arial,sans-serif;font-size:1.6em;font-weight:300;letter-spacing:.01em;line-height:1.6}.button,button,input[type=button],input[type=reset],input[type=submit]{background-color:#9b4dca;border:.1rem solid #9b4dca;border-radius:.4rem;color:#fff;cursor:pointer;display:inline-block;font-size:1.1rem;font-weight:700;height:3.8rem;letter-spacing:.1rem;line-height:3.8rem;padding:0 3rem;text-align:center;text-decoration:none;text-transform:uppercase;white-space:nowrap}input:not([type]),input[type=color],input[type=date],input[type=datetime-local],input[type=datetime],input[type=email],input[type=month],input[type=number],input[type=password],input[type=search],input[type=tel],input[type=text],input[type=url],input[type=week],select,textarea{-webkit-appearance:none;background-color:transparent;border:.1rem solid #d1d1d1;border-radius:.4rem;box-shadow:none;box-sizing:inherit;height:3.8rem;padding:.6rem 1rem .7rem;width:100%}label,legend{display:block;font-size:1.6rem;font-weight:700;margin-bottom:.5rem}fieldset{border-width:0;padding:0}input[type=checkbox],input[type=radio]{display:inline}.label-inline{display:inline-block;font-weight:400;margin-left:.5rem}.button,button,dd,dt,li{margin-bottom:1rem}fieldset,input,select,textarea{margin-bottom:1.5rem}.float-right{float:right}";
const char html_mid[] = R"rawlit(    </style>
  </head>
  <body>
    <div style="text-align: left; display: inline-block; min-width: 260px">)rawlit";
const char html_form[] = R"rawlit(
<form>
{fields}
</form>
)rawlit";
//<fieldset></fieldset>
const char html_btnJS[] = R"rawlit(
    </div>
    <div><button onclick="sv()">Save</button></div>
    <script id="jsbin-javascript">
      function udVal(key, val) {
        var request = new XMLHttpRequest();
        var url = "/?key=" + key + "&value=" + encodeURIComponent(val);
        request.open("GET", url, false);
        request.send(null);
      }
      function sv() {
        {javascript}
        alert("Configuration Updated. MessageMe Rebooting.");
      }
    </script>
  </body>
</html>
)rawlit";

const char *h_Elements[] = {R"rawlit(
    <label for="{id}">{lbl}</label>
    <input type="text" placeholder="{lbl}" value="{val}" id="{id}">
)rawlit",
                            R"rawlit(
    <div class="float-right">
      <input type="checkbox" id="{id}" {chk}>
      <label class="label-inline" for="{id}">{lbl}</label>
    </div>
)rawlit"};
const char *h_javascript[] = {"udVal(\"{id}\", document.getElementById(\"{id}\").value);", "udVal(\"{id}\", document.getElementById(\"{id}\").checked ? 1 : 0);"};

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
  ~dynaHTML();
  void createHTML(String &root_html_template);
  void handleRequest(AsyncWebServerRequest *request);
  void setCallback(callback_function_t callback);
  void setMenuItems(MenuItem aItem[]);
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