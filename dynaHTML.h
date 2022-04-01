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
#include <Arduino.h>

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
MenuItem allItem[] = {{"wiid", "SSID", MyconfigData.wifi_ssid, e_INPUT, 0},
                      {"wipw", "Password", MyconfigData.wifi_pw, e_INPUT, 0},
                      {"msrv", "MQTT Server", MyconfigData.mqtt_server, e_INPUT, 1},
                      {"ss", "Sensor Publish", MyconfigData.sensorstatus, e_INPUT, 2},
                      {"sn", "Sensor Name", MyconfigData.sensorname, e_INPUT, 2},
                      {"mqi", "MQTT ID", MyconfigData.mqtt_id, e_INPUT, 1},
                      {"mqp", "MQTT PW", MyconfigData.mqtt_key, e_INPUT, 1},
                      {"powr", "USB Power", MyconfigData.usb_power, e_CHECK, 2}};

// -- HTML page fragment
uint16_t NUM_MENU_ITEMS = sizeof(allItem) / sizeof(MenuItem);
// EXTERN because the functions are Created in the other file.  THis says, USE THEM otherwise you get out of scope error!  COOL.
extern void saveconfigtoEE(configData MyconfigData);
extern void saveAPEE(apData MyAPdata);
extern configData MyconfigData;
extern apData MyAPdata;

void resetFunc()
{
    delay(1000);
    ESP.restart();
}
int my_min(int a, int b)
{
    return a <= b ? a : b;
}
int my_max(int a, int b)
{
    return b <= a ? a : b;
}
void createHTML(String &root_html_template)
{
    Serial.print("NUM_MENU_ITEMS");
    Serial.println(NUM_MENU_ITEMS);
    String pitem;
    String tmpData;
    String ht_form;
    String ht_js;
    ht_form = String(html_form);
    ht_js = String(html_btnJS);
    root_html_template = "";
    root_html_template = html_top;

    root_html_template += MiligramStyle;
    root_html_template += html_mid;

    uint8_t group = 0;
    tmpData = "";

    int imin = 0;
    int imax = 0;
    for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
    {
        imin = my_min(allItem[i].group, imin);

        imax = my_max(allItem[i].group, imax);
    }
    // printf("min:%d\nmax:%d\n", imin, imax);
    //  minmax - tried to use min/max/minmax but wasnt getting it to work on this array of structures.
    /* ! NEED TO DEAL WITH group*/
    for (uint16_t g = 0; g <= imax; g++)
    {
        tmpData += "<fieldset>";
        for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
        {
            if (allItem[i].group == g)
            {
                pitem = h_Elements[allItem[i].HT_EM]; // Grab the type..  Currrently a TEXT or CHECKBOX

                pitem.replace("{lbl}", allItem[i].displayName);
                pitem.replace("{id}", allItem[i].id);
                if (allItem[i].HT_EM == e_INPUT)
                {
                    pitem.replace("{val}", allItem[i].pdata);
                }
                if (allItem[i].HT_EM == e_CHECK)
                {
                    char *_chkval = allItem[i].pdata;
                    if (String(_chkval) == "1")
                        pitem.replace("{chk}", "checked");
                    else
                        pitem.replace("{chk}", "");
                }
                tmpData += pitem;
            }
        }
        tmpData += "</fieldset>";
    }

    ht_form.replace("{fields}", tmpData);
    root_html_template += ht_form;

    tmpData = "";
    // Write the Javascript
    for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
    {
        pitem = h_javascript[allItem[i].HT_EM];
        pitem.replace("{id}", allItem[i].id);

        tmpData += pitem;
    }
    ht_js.replace("{javascript}", tmpData);
    root_html_template += ht_js;

    return;
}

void handleRequest(AsyncWebServerRequest *request)
{
    if (request)
    {
        String key = request->arg("key");
        String value = request->arg("value");

        static int number_items_Updated = 0;

        if (key == "" && value == "")
        {
            String result;
            createHTML(result);

            request->send(200, content_Type, result);

            // Fix ESP32-S2 issue with WebServer (https://github.com/espressif/arduino-esp32/issues/4348)
            delay(1);

            return;
        }
        Serial.println("Updating[ ");
        Serial.println(key);
        Serial.println(value);
        Serial.println(" ]");

        for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
        {
            Serial.println("displayname[ ");
            Serial.println(allItem[i].id);
            Serial.println(String(allItem[i].displayName));

            if (key == String(allItem[i].id))
            {
                size_t lwrite = 0;
                Serial.println("Updating:");
                Serial.println(key);
                Serial.println(value);
                char *epromdata = allItem[i].pdata;
                if (allItem[i].HT_EM == e_INPUT)
                {
                    // strlcpy is designed to solve the null-termination problems – it always null-terminates.
                    lwrite = strlcpy(epromdata, value.c_str(), strlen(value.c_str()) + 1);
                    number_items_Updated++;
                }
                if (allItem[i].HT_EM == e_CHECK)
                {
                    number_items_Updated++;
                    if (value == "1" or value == "0")
                    {
                        Serial.println("CHECKBOX:");
                        Serial.println(key);
                        Serial.println(value);
                        lwrite = strlcpy(epromdata, value.c_str(), strlen(value.c_str()) + 1);
                    }
                    else
                        strlcpy(epromdata, "0", 2);
                }
                Serial.println(lwrite);
                Serial.println(" ]");
                break; // hey, we don't need to process the rest, since only one change happens at a time.
            }
        }

        request->send(200, content_Type, "OK");

        if (number_items_Updated == NUM_MENU_ITEMS)
        {
            Serial.println("");
            Serial.println("");
            Serial.println("MyconfigData:");
            Serial.println(MyconfigData.wifi_ssid);
            Serial.println(MyconfigData.wifi_pw);
            Serial.println(MyconfigData.mqtt_server);
            Serial.println(MyconfigData.sensorname);
            Serial.println(MyconfigData.sensorstatus);
            Serial.println(MyconfigData.mqtt_id);
            Serial.println(MyconfigData.mqtt_key);
            Serial.println(MyconfigData.usb_power);
            Serial.println(MyconfigData.usb_power == "1");
            Serial.println("MyconfigData:");
            saveconfigtoEE(MyconfigData);
            // if we are updating data, force a refresh of the SSID
            MyAPdata.crc32 = 8675309;
            saveAPEE(MyAPdata);

            delay(1000);
            resetFunc(); // call reset
        }
    } // if request
}