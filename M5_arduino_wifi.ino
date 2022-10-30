#include <ESPmDNS.h>
#include <M5Stack.h>
#include "M5_ENV.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "WebServer.h"


// env stuff
SHT3X sht30;
QMP6988 qmp6988;
float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

// webserver stuff
boolean restoreConfig();
boolean checkConnection();
void startWebServer();
void setupMode();
String makePage(String title, String contents);
String urlDecode(String input);

const IPAddress apIP(
    192, 168, 4, 1);  // Define the address of the wireless AP
const char* apSSID = "M5STACK_SETUP";  
boolean settingMode;
String ssidList;
String wifi_ssid;  // Store the name of the wireless network
String wifi_password;  // Store the password of the wireless network

WebServer webServer(80);

// wifi config store.  wifi
Preferences preferences;

void setup() {
    M5.begin();       
    M5.Power.begin();
    M5.Lcd.setTextColor(YELLOW);  // Set the font color to yellow
    Wire.begin();  
    qmp6988.init();
    M5.lcd.println(F("ENVIII Unit test"));  
    preferences.begin("wifi-config");
    delay(10);
    if (restoreConfig()) {  // Check if wifi configuration information has been
                            // stored
        if (checkConnection()) {  // Check wifi connection. 
            settingMode = false;  // Turn off setting mode. 
            startWebServer();  // Turn on network service. 
            return;            // Exit setup().
        }
    }
    settingMode =
        true;  // If there is no stored wifi configuration information, turn on
               // the setting mode.  
    setupMode();
}



void loop() {
  pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {  
        tmp = sht30.cTemp;                      
        hum = sht30.humidity;  
                               
    } else {
        tmp = 0, hum = 0;
    }

    M5.lcd.setCursor(0, 100);
    M5.lcd.setTextSize(2);
    M5.lcd.setTextColor(WHITE);
    M5.lcd.drawRect(0, 100, 300, 300, BLACK);       //creates rectangle around sensor readings
    M5.lcd.fillRect(0, 100, 300, 300, BLACK);       //fills rectangle around sensor readings------need both this and ^^ to happen, otherwise sensor readings eventually become garbled text
    M5.Lcd.printf("Temp: %2.1fC \r\nTemp: %2.1fF \r\nHumi: %2.0f%% \r\nPressure: %2.0fPa \r\nPressure: %2.0fmmHg",
                  tmp, tmp*(9/5)+32, hum, pressure, pressure/1.333);

    delay(5000);
  
  if (settingMode) {
    }
    webServer
        .handleClient();  // Check that there is no facility to send requests to
                          // the M5StickC Web server over the network.
}

boolean
restoreConfig() { /* Check whether there is wifi configuration information
                  storage, if there is 1 return, if no return 0. */
    wifi_ssid     = preferences.getString("WIFI_SSID");
    wifi_password = preferences.getString("WIFI_PASSWD");
    M5.Lcd.printf(
        "WIFI-SSID: %s\n",
        wifi_ssid);  // Screen print format string.
    M5.Lcd.printf("WIFI-PASSWD: %s\n", wifi_password);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

    if (wifi_ssid.length() > 0) {
        return true;
    } else {
        return false;
    }
}

boolean checkConnection() {  // Check wifi connection. 
    int count = 0;         
    M5.Lcd.print("Waiting for Wi-Fi connection");
    while (count <
           30) { /* If you fail to connect to wifi within 30*350ms (10.5s),
                 return false; otherwise return true. */
        if (WiFi.status() == WL_CONNECTED) {
            M5.Lcd.printf("\nConnected!\n");
            return (true);
        }
        delay(350);
        M5.Lcd.print(".");
        count++;
    }
    M5.Lcd.println("Timed out.");
    return false;
}

void startWebServer() {  // Open the web service.  
    if (settingMode) {  // If the setting mode is on.  
        M5.Lcd.print("Starting Web Server at: ");
        M5.Lcd.print(
            WiFi.softAPIP());  // Output AP address (you can change the address
                               // you want through apIP at the beginning).
        webServer.on(
            "/settings", []() {  // AP web interface settings. 
                String s =
                    "<h1>Wi-Fi Settings</h1><p>Please enter your password by "
                    "selecting the SSID.</p>";
                s += "<form method=\"get\" action=\"setap\"><label>SSID: "
                     "</label><select name=\"ssid\">";
                s += ssidList;
                s += "</select><br>Password: <input name=\"pass\" length=64 "
                     "type=\"password\"><input type=\"submit\"></form>";
                webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
            });
        webServer.on("/setap", []() {
            String ssid = urlDecode(webServer.arg("ssid"));
            M5.Lcd.printf("SSID: %s\n", ssid);
            String pass = urlDecode(webServer.arg("pass"));
            M5.Lcd.printf("Password: %s\n\nWriting SSID to EEPROM...\n", pass);

            // Store wifi config.  
            M5.Lcd.println("Writing Password to nvr...");
            preferences.putString("WIFI_SSID", ssid);
            preferences.putString("WIFI_PASSWD", pass);

            M5.Lcd.println("Write nvr done!");
            String s =
                "<h1>Setup complete.</h1><p>device will be connected to \"";
            s += ssid;
            s += "\" after the restart.";
            webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
            delay(2000);
            ESP.restart();  // Restart MPU.  
        });
        webServer.onNotFound([]() {
            String s =
                "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi "
                "Settings</a></p>";
            webServer.send(200, "text/html", makePage("AP mode", s));
        });
    } else {  // If the setting mode is off.  
        M5.Lcd.print("Starting Web Server at ");
        M5.Lcd.println(WiFi.localIP());
        webServer.on("/", []() {  // AP web interface settings.  
            String s =
                "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi "
                "Settings</a></p>";
            webServer.send(200, "text/html", makePage("STA mode", s));
        });
        webServer.on("/reset", []() {
            // reset the wifi config
            preferences.remove("WIFI_SSID");
            preferences.remove("WIFI_PASSWD");
            String s =
                "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
            webServer.send(200, "text/html",
                           makePage("Reset Wi-Fi Settings", s));
            delay(2000);
            ESP.restart();
        });
    }
    webServer.begin();  // Start web service.  
}

void setupMode() {
    WiFi.mode(WIFI_MODE_STA);  // Set Wi-Fi mode to WIFI_MODE_STA.
    WiFi.disconnect();         // Disconnect wifi connection.  
    delay(100);
    int n = WiFi.scanNetworks();  // Store the number of wifi scanned into n.
    delay(100);
    M5.Lcd.println("");
    for (int i = 0; i < n; ++i) {  // Save each wifi name scanned to ssidList.
        ssidList += "<option value=\"";
        ssidList += WiFi.SSID(i);
        ssidList += "\">";
        ssidList += WiFi.SSID(i);
        ssidList += "</option>";
    }
    delay(100);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID);  // Turn on Ap mode. 
    WiFi.mode(WIFI_MODE_AP);  // Set WiFi to soft-AP mode. 
    startWebServer();         // Open the web service. 
    M5.Lcd.printf("\nStarting Access Point at \"%s\"\n\n", apSSID);
}

String makePage(String title, String contents) {
    String s = "<!DOCTYPE html><html><head>";
    s += "<meta name=\"viewport\" "
         "content=\"width=device-width,user-scalable=0\">";
    s += "<title>";
    s += title;
    s += "</title></head><body>";
    s += contents;
    s += "</body></html>";
    return s;
}

String urlDecode(String input) {
    String s = input;
    s.replace("%20", " ");
    s.replace("+", " ");
    s.replace("%21", "!");
    s.replace("%22", "\"");
    s.replace("%23", "#");
    s.replace("%24", "$");
    s.replace("%25", "%");
    s.replace("%26", "&");
    s.replace("%27", "\'");
    s.replace("%28", "(");
    s.replace("%29", ")");
    s.replace("%30", "*");
    s.replace("%31", "+");
    s.replace("%2C", ",");
    s.replace("%2E", ".");
    s.replace("%2F", "/");
    s.replace("%2C", ",");
    s.replace("%3A", ":");
    s.replace("%3A", ";");
    s.replace("%3C", "<");
    s.replace("%3D", "=");
    s.replace("%3E", ">");
    s.replace("%3F", "?");
    s.replace("%40", "@");
    s.replace("%5B", "[");
    s.replace("%5C", "\\");
    s.replace("%5D", "]");
    s.replace("%5E", "^");
    s.replace("%5F", "-");
    s.replace("%60", "`");
    return s;
}
