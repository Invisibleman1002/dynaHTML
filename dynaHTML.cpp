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

#include <functional>
#include <ESPAsyncWebServer.h>
#include "dynaHTML.h"
dynaHTML::dynaHTML()
{
}

void dynaHTML::setCallback(callback_function_t callback)
{
    _callback_function = callback;
}
void dynaHTML::setMenuItems(MenuItem aItem[])
{
    allItem = aItem;
    NUM_MENU_ITEMS = sizeof(allItem) / sizeof(MenuItem);
    for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
    {
        Serial.println("[");
        Serial.println(allItem[i].displayName);
        Serial.println(allItem[i].id);
        Serial.println(allItem[i].pdata);
        Serial.println("]");
    }
}
void dynaHTML::resetFunc()
{
    delay(1000);
    ESP.restart();
}
int dynaHTML::my_min(int a, int b)
{
    return a <= b ? a : b;
}
int dynaHTML::my_max(int a, int b)
{
    return b <= a ? a : b;
}
void dynaHTML::createHTML(String &root_html_template)
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

void dynaHTML::handleRequest(AsyncWebServerRequest *request)
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
            // Serial.println("");
            // Serial.println("");
            // Serial.println("MyconfigData:");
            // Serial.println(MyconfigData.wifi_ssid);
            // Serial.println(MyconfigData.wifi_pw);
            // Serial.println(MyconfigData.mqtt_server);
            // Serial.println(MyconfigData.sensorname);
            // Serial.println(MyconfigData.sensorstatus);
            // Serial.println(MyconfigData.mqtt_id);
            // Serial.println(MyconfigData.mqtt_key);
            // Serial.println(MyconfigData.usb_power);
            // Serial.println(MyconfigData.usb_power == "1");
            // Serial.println("MyconfigData:");
            // saveconfigtoEE(MyconfigData);
            // // if we are updating data, force a refresh of the SSID
            // MyAPdata.crc32 = 8675309;
            // saveAPEE(MyAPdata);

            delay(1000);
            // resetFunc(); // call reset
            if (_callback_function != NULL)
                _callback_function();
        }
    } // if request
}