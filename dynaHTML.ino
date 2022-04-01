/*
 THis is using a Generic ESP8266 Board..  ESP 12-F
flash size: FS 2mb ota 1019k
upload 921600, which works, but interface changes to 460800.

  Version Modified By   Date        Comments
  ------- -----------  ----------   -----------
  1.0.0   Trey A       03/31/2022  Initial coding for ESP8266
  1.1.0   Trey A       04/01/2022  Added Grouping

*/
#define DEBUG_ON 1

// define the output channels
#define DEBUG_USE_SERIAL 1
#define DEBUG_USE_TELNET 1

#include <ESP_EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include "ESPTelnet.h"
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const int sleepSeconds = 60;
// Update these with values suitable for your network.

#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 64
#define ADA_URL_LEN 20
#define ADA_FEED_LEN 40
#define ADA_ID_LEN 30
#define ADA_KEY_LEN 33

const char *ssidAP = "ESP_ALERT";
const char *passwordAP = "12345678";
struct configData
{
    char wifi_ssid[SSID_MAX_LEN];
    char wifi_pw[PASS_MAX_LEN];
    char mqtt_server[ADA_URL_LEN];
    char sensorstatus[ADA_FEED_LEN];
    char sensorname[ADA_KEY_LEN];
    char mqtt_id[ADA_ID_LEN];
    char mqtt_key[ADA_KEY_LEN];
    char usb_power[2]; // bool is a byte, is a char[1] +null termination
};
configData MyconfigData = {};
struct apData
{
    uint32_t crc32;   // 4 bytes
    uint8_t channel;  // 1 byte,   5 in total
    uint8_t bssid[6]; // 6 bytes, 11 in total
};
apData MyAPdata = {};
bool apValid = false;
// configData MyconfigData = {"A11_IOT", "SKIOT", "192.168.0.44", "home/door/side/open", "SIDE-DOOR", "mqtt", "mqtt"};

#define eepromstart 0
#define EEPROM_SIZE (sizeof(configData) + sizeof(apData))
#define eepromapstart sizeof(configData)

#include "dynaHTML.h"
int holdPin = 0; // defines GPIO 0 as the hold pin (will hold CH_PD high untill we power down).
int pirPin = 12; // defines GPIO 12 as the PIR read pin (reads the state of the PIR output).
int pir = 1;     // sets the PIR record (pir) to 1 (it must have been we woke up).
int GPIO4 = 4;
ADC_MODE(ADC_VCC); // vcc read-mode
typedef enum
{
    CONFIG = 0,
    RUN = 1,
} ConfigMode;
uint8_t configmode = RUN;

AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
ESPTelnet telnet;

int brightness = 0; // how bright the LED is
int fadeAmount = 5; // how many points to fade the LED by
unsigned long lastMsg = 0;
unsigned long mslightdim = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
bool OpenPublished = false;
bool Debugging = false;
#define OTA_PASSDW "admin"

Ticker OTAstart;
Ticker Telnetstart;
/*
IPAddress staticIP(192, 168, 0, 232); //ESP static ip
IPAddress gateway(192, 168, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(192, 168, 0, 1);//(8, 8, 8, 8);  //DNS
*/

void create_NEW_feed(char *strng, char *NEWNAME)
{
    printf(" strng '%s'\n", strng);
    // char str[ADA_FEED_LEN]; // = "home/door/front/open";
    // char strng[ADA_FEED_LEN]; //"home/door/front/open";
    //  const char s[2] = "/";

    // strncpy(str, sendfeed, sizeof(sendfeed));
    // printf( " sendfeed '%s'\n",  sendfeed  );
    // printf( " str '%s'\n",  str  );
    char *pch;
    pch = strrchr(strng, '/');
    if (pch != NULL)
        strncpy(pch + 1, NEWNAME, sizeof(NEWNAME));
    // strncpy(pch, "vcc", 6);
    //  Serial.printf(" %s\n", strng);
    // Serial.print("STRGINSGNKKNT");
    //  Serial.println(strng);
}

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffff;
    while (length--)
    {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1)
        {
            bool bit = crc & 0x80000000;
            if (c & i)
            {
                bit = !bit;
            }

            crc <<= 1;
            if (bit)
            {
                crc ^= 0x04c11db7;
            }
        }
    }

    return crc;
}

configData getConfigData()
{
    configData customData;
    EEPROM.get(eepromstart, customData);
    DEBUG_MSG(F("Read getConfigData from EEPROM: "));
    return customData;
}

void saveconfigtoEE(configData MyconfigData)
{
    EEPROM.put(eepromstart, MyconfigData);
    boolean ok2 = EEPROM.commit();
    DEBUG_MSG((ok2) ? "Second commit OK" : "Commit failed");
    yield;
}

apData getAPData()
{
    apData customAPData;
    EEPROM.get(eepromapstart, customAPData);
    // Serial.println(F("Read customAPData from EEPROM: "));
    uint32_t crc = calculateCRC32(((uint8_t *)&customAPData) + 4, sizeof(customAPData) - 4);
    if (crc == customAPData.crc32)
    {
        apValid = true;
    }
    return customAPData;
}

void saveAPEE(apData MyAPData)
{
    EEPROM.put(eepromapstart, MyAPData);
    boolean ok2 = EEPROM.commit();
    // Serial.println((ok2) ? "MyAPData commit OK" : "Commit failed");
}

bool chechee()
{
    int val = 255;
    EEPROM.get(0, val);
    // Serial.println(val);
    if (val == 0)
        return false;
    return true;
}

void OTAinit()
{
    // Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("DOOR-FRONT-ESP");
    ArduinoOTA.setHostname(MyconfigData.sensorname);
    // if (DEBUG == false) {
    //  Comment to: No authentication by default
    ArduinoOTA.setPassword(OTA_PASSDW);
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
    // }

    ArduinoOTA.onStart([]()
                       {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DEBUG_MSG("Start updating " + type); });
    ArduinoOTA.onEnd([]()
                     { DEBUG_MSG("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r\n", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
    ArduinoOTA.begin();
    DEBUG_MSG("Ready");
    DEBUG_MSG("IP address: ");
    DEBUG_SER_VAR(WiFi.localIP());
}

/* ------------------------------------------------- */

// (optional) callback functions for telnet events
void onTelnetConnect(String ip)
{
    Serial.print("- Telnet: ");
    Serial.print(ip);
    Serial.println(" connected");
    telnet.println("\nWelcome " + telnet.getIP());
    telnet.println("(Use ^] + q  to disconnect.)");
}

void onTelnetDisconnect(String ip)
{
    Serial.print("- Telnet: ");
    Serial.print(ip);
    Serial.println(" disconnected");
}

void onTelnetReconnect(String ip)
{
    Serial.print("- Telnet: ");
    Serial.print(ip);
    Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip)
{
    Serial.print("- Telnet: ");
    Serial.print(ip);
    Serial.println(" tried to connected");
}

void setupTelnet()
{
    // passing on functions for various telnet events
    telnet.onConnect(onTelnetConnect);
    telnet.onConnectionAttempt(onTelnetConnectionAttempt);
    telnet.onReconnect(onTelnetReconnect);
    telnet.onDisconnect(onTelnetDisconnect);

    // passing a lambda function
    telnet.onInputReceived([](String str)
                           {
    // checks for a certain command
    if (str == "ping") {
      telnet.println("> pong");
      Serial.println("- Telnet: pong");
    }
    if (str =="debug"){
      Debugging = !Debugging;
      DEBUG_MSG("Debugging:");
      DEBUG_MSG(Debugging);
    }
    if (str =="help"){
      Debugging = !Debugging;
      DEBUG_MSG("HELP:");
      DEBUG_MSG("Set mqtt");
      DEBUG_MSG("mqtt home/door/back/open");
    } });
    // this wont work!  You cannot wait for input.
    // MIght need to check ou that Telnet Steam Library instead
    // if (str =="mqtt"){} });

    Serial.print("- Telnet: ");
    if (telnet.begin())
    {
        Serial.println("running");
    }
    else
    {
        Serial.println("error.");
        // errorMsg("Will reboot...");
    }
}

/* ------------------------------------------------- */

void setup_wifi()
{
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(MyconfigData.wifi_ssid);
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.forceSleepWake();
    // delay(200);
    // WiFi.config(staticIP, subnet, gateway, dns);
    // WiFi.begin(MyconfigData.wifi_ssid, MyconfigData.wifi_pw);
    if (apValid)
    {
        DEBUG_MSG("Using BSSID data");
        // The RTC data was good, make a quick connection
        WiFi.begin(MyconfigData.wifi_ssid, MyconfigData.wifi_pw, MyAPdata.channel, MyAPdata.bssid, true);
    }
    else
    {
        DEBUG_MSG("not using AP data");
        // The RTC data was not valid, so make a regular connection
        WiFi.begin(MyconfigData.wifi_ssid, MyconfigData.wifi_pw);
    }
    int retries = 0;
    int wifiStatus = WiFi.status();

    while (wifiStatus != WL_CONNECTED)
    {
        retries++;
        if (retries == 100)
        {
            // Quick connect is not working, reset WiFi and try regular connection
            // boolean result = EEPROM.wipe();
            // EEPROM.commit();
            WiFi.disconnect();
            delay(10);
            WiFi.forceSleepBegin();
            delay(10);
            WiFi.forceSleepWake();
            delay(10);
            WiFi.begin(MyconfigData.wifi_ssid, MyconfigData.wifi_pw);
        }
        // Give it 20 Seconds of retrying, otherwise reset the BSSID and try again.
        if (retries == 400)
        {
            WiFi.disconnect(true);
            delay(1);
            WiFi.mode(WIFI_OFF);
            // invalidate the CRC, force a refresh of the WIFI AP without a BSSID/CHANNEL
            MyAPdata.crc32 = 8675309;
            saveAPEE(MyAPdata);
            delay(10);
            ESP.restart();
            return;
        }
        delay(50);
        DEBUG_MSG(".-");
        wifiStatus = WiFi.status();
    }

    MyAPdata.channel = WiFi.channel();
    memcpy(MyAPdata.bssid, WiFi.BSSID(), 6); // Copy 6 bytes of BSSID (AP's MAC address)
    /*
    DEBUG_MSG("BSSID");
    DEBUG_MSG(WiFi.BSSID()[0]);
    DEBUG_MSG(".");
    DEBUG_MSG(WiFi.BSSID()[1]);
    DEBUG_MSG(".");
    DEBUG_MSG(WiFi.BSSID()[2]);
    DEBUG_MSG(".");
    DEBUG_MSG(WiFi.BSSID()[3]);
    DEBUG_MSG(".");
    DEBUG_MSG(WiFi.BSSID()[4]);
    DEBUG_MSG(".");
    DEBUG_MSG(WiFi.BSSID()[5]);
    DEBUG_MSG("BSSSTR");
    DEBUG_MSG(WiFi.BSSIDstr());
    Serial.println(WiFi.BSSIDstr());
    */
    MyAPdata.crc32 = calculateCRC32(((uint8_t *)&MyAPdata) + 4, sizeof(MyAPdata) - 4);
    saveAPEE(MyAPdata);

    randomSeed(micros());

    DEBUG_MSG("");
    DEBUG_MSG("WiFi connected");
    DEBUG_MSG("IP address: ");
    DEBUG_VAR(WiFi.localIP().toString());
}

void reconnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }
    // Loop until we're reconnected
    while (!client.connected())
    {
        DEBUG_MSG("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "DB-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), MyconfigData.mqtt_id, MyconfigData.mqtt_key))
        {
            DEBUG_MSG("connected");
            DEBUG_MSG(MyconfigData.sensorstatus);
        }
        else
        {
            DEBUG_MSG("failed, rc=");
            DEBUG_MSG(client.state());
            DEBUG_MSG(" try again in 1 seconds");
            // Wait 1 seconds before retrying
            delay(1000); // oh man, we should not be blocking.
        }
    }
}

void setup()
{

    pinMode(holdPin, OUTPUT);     // sets GPIO 0 to output
    digitalWrite(holdPin, HIGH);  // sets GPIO 0 to high (this holds CH_PD high even if the PIR output goes low)
    pinMode(pirPin, INPUT);       // sets GPIO 12 to an input so we can read the PIR output state
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    pinMode(GPIO4, INPUT_PULLUP); // If this pin is LOW, then REPROGRAM by setting up the Access Point.
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    delay(10);
    MyconfigData = getConfigData();
    delay(10);
    MyAPdata = getAPData();
    delay(10);

    DEBUG_MSG("Config/ap data");
    DEBUG_MSG(MyconfigData.wifi_ssid);
    DEBUG_MSG(MyAPdata.crc32);

    String result;
    createHTML(result);
    Serial.print(result);

    // return;

    if (chechee() == false or digitalRead(GPIO4) == LOW)
    {
        configmode = CONFIG;
        Debugging = true;
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(ssidAP, passwordAP);
        IPAddress IP = WiFi.softAPIP();
        // DEBUG_MSG("AP IP address: ");
        // DEBUG_MSG(IP);

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { handleRequest(request); });

        server.begin();
    }
    if (chechee() == true and digitalRead(GPIO4) == HIGH)
    {
        configmode = RUN;
        setup_wifi();
        client.setServer(MyconfigData.mqtt_server, 1883);
        // client.setCallback(callback);

        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);

        if (!MDNS.begin(MyconfigData.sensorname))
        { // Start the mDNS responder for esp8266.local
            DEBUG_MSG("Error setting up MDNS responder!");
        }

        // OTAinit();
        OTAstart.once(1, OTAinit);
        Telnetstart.once(1, setupTelnet);
        // setupTelnet();

        MDNS.addService("telnet", "tcp", 23);
        // MDNS.addService("http", "tcp", 80);

        DEBUG_MSG(WiFi.localIP().toString());
        DEBUG_MSG("Testing the debug class...");
        DEBUG_WHERE;
    }
}
/*  WOUld like to do this withouth using STRING for the BSSID
String mac2String(byte ar[]) {
  String s;
  for (byte i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", ar[i]); // J-M-L: slight modification, added the 0 in the format for padding
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}
*/

void loop()
{
    byte inChar;
    inChar = Serial.read();
    if (inChar == 'c')
    {
        DEBUG_MSG("memory message for Config");
        configData lmsgConfig = {};

        lmsgConfig = getConfigData();
        DEBUG_MSG(lmsgConfig.wifi_ssid);
        DEBUG_MSG(lmsgConfig.wifi_pw);
        DEBUG_MSG(lmsgConfig.mqtt_server);
        DEBUG_MSG(lmsgConfig.sensorstatus);
        DEBUG_MSG(lmsgConfig.sensorname);
        DEBUG_MSG(lmsgConfig.mqtt_id);
        DEBUG_MSG(lmsgConfig.mqtt_key);
        DEBUG_MSG(lmsgConfig.usb_power);
        apData lmsgAP = {};

        lmsgAP = getAPData();
        // DEBUG_MSG(WiFi.BSSID());
        DEBUG_MSG(lmsgAP.channel);
        DEBUG_MSG(lmsgAP.crc32);
    }
    if (inChar == 'w')
    {
        boolean result = EEPROM.wipe();
        if (result)
        {
            DEBUG_MSG("All EEPROM data wiped");
        }
        else
        {
            DEBUG_MSG("EEPROM data could not be wiped from flash store");
        }
    }

    if (inChar == 'm')
    {
        DEBUG_MSG("Sending mDNS query");
        int n = MDNS.queryService("telnet", "tcp"); // Send out query for esp tcp services
        DEBUG_MSG("mDNS query done");
        if (n == 0)
        {
            DEBUG_MSG("no services found");
        }
        else
        {
            Serial.print(n);
            DEBUG_MSG(" service(s) found");
            for (int i = 0; i < n; ++i)
            {
                // Print details for each service found
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(MDNS.hostname(i));
                Serial.print(" (");
                Serial.print(MDNS.IP(i));
                Serial.print(":");
                Serial.print(MDNS.port(i));
                DEBUG_MSG(")");
            }
        }
        DEBUG_MSG();
    }
    if (configmode == RUN)
    {
        if (!client.connected())
        {
            reconnect();
        }
        client.loop();

        telnet.loop();

        MDNS.update();
        // send serial input to telnet as output
        // if (Serial.available())
        // {
        //   telnet.print(Serial.read());
        // }
    }

    if (configmode == CONFIG)
    {

        unsigned long now = millis();
        if (now - mslightdim > 30)
        {
            mslightdim = now;
            analogWrite(LED_BUILTIN, brightness);

            // change the brightness for next time through the loop:
            brightness = brightness + fadeAmount;

            // reverse the direction of the fading at the ends of the fade:
            if (brightness <= 0 || brightness >= 255)
            {
                fadeAmount = -fadeAmount;
            }
        }
    }
    unsigned long now = millis();
    if (now - lastMsg > 10000)
    {
        lastMsg = now;
        ++value;
        snprintf(msg, MSG_BUFFER_SIZE, "test #%ld", value);
        DEBUG_MSG("Publish message ota: ");
        DEBUG_MSG(msg);
        DEBUG_MSG("Sensor GPIO:");
        DEBUG_MSG(digitalRead(pirPin));
        DEBUG_MSG("GPIO4:");
        DEBUG_MSG(digitalRead(GPIO4));
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}
