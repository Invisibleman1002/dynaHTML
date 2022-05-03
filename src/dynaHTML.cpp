#include <functional>
#include <ESPAsyncWebServer.h>
#include "dynaHTML.h"

dynaHTML::dynaHTML()
{
}
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
        alert("Configuration Updated....Rebooting.");
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
)rawlit",
                            R"rawlit(
    <label for="{id}">{lbl}</label>
    <input type="password" value="{val}" id="{id}">
)rawlit"};
const char *h_javascript[] = {"udVal(\"{id}\", document.getElementById(\"{id}\").value);", "udVal(\"{id}\", document.getElementById(\"{id}\").checked ? 1 : 0);", "udVal(\"{id}\", document.getElementById(\"{id}\").value);"};

void dynaHTML::setCallback(callback_function_t callback)
{
    _callback_function = callback;
}
uint16_t dynaHTML::setMenuItems(MenuItem aItem[], uint16_t menucount)
{
    allItem = aItem;
    NUM_MENU_ITEMS = menucount;
    /*     for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
        {
            Serial.println("[");
            Serial.println(allItem[i].displayName);
            Serial.println(allItem[i].id);
            Serial.println(allItem[i].pdata);
            Serial.println("]");
        } */
    return NUM_MENU_ITEMS;
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
    if (NUM_MENU_ITEMS == 0)
    {
        root_html_template = "You missed some important setup details.";
        return;
    }

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
    for (uint16_t g = 0; g <= imax; g++)
    {
        tmpData += "<fieldset>";
        for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
        {
            pitem = "";
            if (allItem[i].group == g)
            {
                pitem = h_Elements[allItem[i].HT_EM]; // Grab the type..  Currrently a TEXT or CHECKBOX

                pitem.replace("{lbl}", allItem[i].displayName);
                pitem.replace("{id}", allItem[i].id);
                if ((allItem[i].HT_EM == e_INPUT) or (allItem[i].HT_EM == e_PASS))
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

        for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
        {

            if (key == String(allItem[i].id))
            {
                size_t lwrite = 0;
                char *epromdata = allItem[i].pdata;
                if ((allItem[i].HT_EM == e_INPUT) or (allItem[i].HT_EM == e_PASS))
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
                        lwrite = strlcpy(epromdata, value.c_str(), strlen(value.c_str()) + 1);
                    }
                    else
                        strlcpy(epromdata, "0", 2);
                }
                break; // hey, we don't need to process the rest, since only one change happens at a time.
            }
        }

        request->send(200, content_Type, "OK");

        if (number_items_Updated == NUM_MENU_ITEMS)
        {
            delay(60);
            if (_callback_function != NULL)
                _callback_function();
        }
    } // if request
}