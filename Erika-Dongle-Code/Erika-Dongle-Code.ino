#include <Preferences.h>
#include <WiFi.h>             
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "esp_mac.h"  // required - exposes esp_mac_type_t values

// hardware specific includes
#ifdef ARDUINO_M5STACK_STAMP_S3
  #define HWVERSION 2  // see readme
  #include "M5StampS3_board.h"
#else
  #error "The code is currently only adapted for use with M5-Stamp S3 board. Please ensure to have selected the right board." 
#endif

#include "neopixel.hpp"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wm;
bool wmOnDemandStart = false;
#include "wifimanger.hpp"

#include "utf8coder.hpp"
#include "ddr2unicode.h"
#include "unicode2ddr.h"
#define PC_BAUD 115200

Preferences preferences;

String authToken;
String logURL;
String url;
String authMode;
String model;
String systemprompt;
String temperature;
String helloMsg;
String hwVersionStr;
int hwVersion = 3;

uint32_t chipId = 0;

int rtsPin = 7;
//uint8_t onDemandPin = 0;

class ErikaSchreibmaschine
{
  private:
  int _txPin, _rxPin, _rtsPin, _baud;
  bool _wait = false;

  #ifdef SWSERIAL
  EspSoftwareSerial::UART serconn;
  #else
  #define serconn HWSERIAL  
  #endif
  size_t num_spalten;
  size_t spalte;

  public: 
  ErikaSchreibmaschine(int rxPin, int txPin, int rtsPin, int baud)
  //ErikaSchreibmaschine()
   #ifdef SWSERIAL
   :spalte(0), num_spalten(75), serconn(_rxPin, _txPin) // RX, TX
   #else
   :spalte(0), num_spalten(75) // end of line "beep" here
   #endif
  {
    _rtsPin=rtsPin;
    _txPin=txPin;
    _rxPin=rxPin;
    _baud=baud;
  }

  bool Init(int rtsPin)
  {
    _rtsPin=rtsPin;
    
    pinMode(_rtsPin, INPUT_PULLUP);
    #ifdef SWSERIAL
    serconn.begin(_baud, SWSERIAL_8N1, _rxPin, _txPin, false);
    #else
    serconn.begin(_baud);
    #endif
    
    //serconn.setTimeout(0);
    //serconn.begin(_baud);
    //return ClearBuf();
    return true;
  }

  bool ClearBuf()
  {
    // alles weglesen - puffer leeren
    bool r =false;
    while (serconn.read() > 0) 
      r=true;
    if(r)
    {  
      Serial.println("Tastaturpuffer geleert.");
    }
    return r;
  }

  bool CharAvailable()
  {
    return serconn.available();
  }

  char16_t ReadC()
  {
     if(CharAvailable())
     {
      int znummer = serconn.read();
      Serial.print(znummer,16);
      char zeichen = ddr2uni[znummer];
      Serial.print(zeichen,16);
      return zeichen;
     }
     return 0;
  }

// Liest eine ganze Zeile bis \n und liefert das als utf-8 kodierten string
  String ReadLine()
  {
    String eingabestring;
    std::vector<char16_t> uniZeichenfolge;
    while(true)
    {
      if(CharAvailable())
      {
        int ddrZeichen = serconn.read();
        // breaker for while loop
        if(ddrZeichen == 119) // newline
        {
          break;
        }

        switch (ddrZeichen)
        {
          case 0xAE:  // previously: 0x72=114 Backspace, Dokumentation sagt aber, dass die Korrekturtaste 0xAE ist (Code 174) 
            { 
              if(!uniZeichenfolge.empty()) //works now as expected
              {
                //Serial.println("delete last character");
                uniZeichenfolge.pop_back();
              }
            break; 
            }                          
          case 117: { break; }                           // Einzug ignorieren
          case 118: { break; }                           // Papier zurück ignorieren
          case 119: { break; }                           // Newline hier ignorieren
          case 121: { break; }                           // Tab ignorieren
          default: 
          {                                   // Druckbare Zeichen anhängen 
            if (ddrZeichen > 127) {} // nothing - kann nicht sein  // it can be ;-) it can be a command code!
            else // gutes Zeichen auf der Erikatastatur
            {
              char16_t uniZeichen = ddr2uni[ddrZeichen]; // a bit dangerous before where it was (8 bit in for 7 bits lookup)
              uniZeichenfolge.push_back(uniZeichen);
            }
            break;
          }
        };
      }
      else
      {
        // däumchen drehen
        delay(100);
      }
    }

    // Tastatur in unicode eingaben sind im vector
    // codepoints in utf-8 bytefolge verwandeln.
    for (size_t i = 0; i < uniZeichenfolge.size(); i++)
    {
        std::vector<uint8_t> utfbytes = numberToUtf8(uniZeichenfolge[i]);
        String s(utfbytes.data(), utfbytes.size());
        eingabestring.concat(s);     
    }
    return eingabestring;  
  }

  void Write(String wstring)
  {    
    int i = 0;
    int length = wstring.length();
    SetDuplexMode();

    while (i <= length)
    {
      if (IsReady()) {        
        int zeichen = wstring[i];

        int charsToSpace = wstring.indexOf(' ', i) - i; //how far is the next space away?

        if (charsToSpace > 0) {
          if ((spalte + charsToSpace) >= num_spalten) { //next word will not fit on the remaining space of the line
            NewLine();
          }
        }

        if (spalte >= num_spalten) {
          NewLine();
        }

      //TODO: avoid "\n " what would create a space as first character on new line

        if(zeichen=='@')
        {
          serconn.write(uni2ddr['O']);
          serconn.write(114); // Backspace
          serconn.write(uni2ddr['a']);
        }
        else {
          if (zeichen < 128) { // 1 byte character
            if (zeichen == '\n') {
              NewLine();
            }
            else {
              serconn.write(uni2ddr[zeichen]);
            }
          }
          else if (zeichen >= 194 && zeichen <= 223) { // 2 byte character start byte
            i++;
            uint8_t zeichen2 = wstring[i];//utf8chars[i];
            zeichen = (zeichen<<8) | zeichen2;

            if      (zeichen == 0xc39f ) {serconn.write(0x47); }//ß
            else if (zeichen == 0xc3a4 ) {serconn.write(0x65); }//ä
            else if (zeichen == 0xc3b6 ) {serconn.write(0x66); }//ö
            else if (zeichen == 0xc3bc ) {serconn.write(0x67); }//ü
            else if (zeichen == 0xc384 ) {serconn.write(0x3F); }//Ä
            else if (zeichen == 0xc396 ) {serconn.write(0x3C); }//Ö
            else if (zeichen == 0xc39c ) {serconn.write(0x3A); }//Ü
            else if (zeichen == 0xc2b0 ) {serconn.write(0x39); }//°
            else if (zeichen == 0xc2a7 ) {serconn.write(0x3D); }//§
            else if (zeichen == 0xC2A0 ) {serconn.write(0x71); }// non-breaking space
            else if (zeichen == 0x201c ) {serconn.write(0x17); }// “ (U+201C)  
            else if (zeichen == 0x201d ) {serconn.write(0x17); }// ” (U+201D)
            else if (zeichen == 0x201e ) {serconn.write(0x17); }// „ (U+201E)
            else {serconn.write(27);} // write an asteriks * for any unsupported UTF char
          }
          else if (zeichen >= 224 && zeichen <=239) { // 3 byte character start byte
            i++;
            uint8_t zeichen2 = wstring[i];
            zeichen = (zeichen<<8) | zeichen2;

            i++;
            uint8_t zeichen3 = wstring[i];
            if (zeichen == 0xe280 ) { //0xE280 for starting special characters in UTF block general interpunctation
              if      (zeichen3 == 0x9c ) {serconn.write(0x17); }// “ "
              else if (zeichen3 == 0x9d ) {serconn.write(0x17); }// “ "
              else if (zeichen3 == 0x9e ) {serconn.write(0x17); }// „ "
              else if (zeichen3 == 0x9e ) {serconn.write(0x17); }// „ "
              else if (zeichen3 == 0x91 ) {serconn.write(uni2ddr['-']); }// DASH
              else if (zeichen3 == 0x92 ) {serconn.write(uni2ddr['-']); }// DASH
              else if (zeichen3 == 0x93 ) {serconn.write(uni2ddr['-']); }// DASH
              else if (zeichen3 == 0x94 ) {serconn.write(uni2ddr['-']); }// DASH
              else if (zeichen3 == 0x95 ) {serconn.write(uni2ddr['-']); }// DASH
              else if (zeichen3 == 0xaf ) {serconn.write(0x71); }// protected space                
              else {serconn.write(27);} // write an asteriks * for any unsupported UTF char
            }
            else if (zeichen == 0xe289 ) { //0xE289 for starting special characters in UTF block
              if      (zeichen3 == 0x88 ) {serconn.write(uni2ddr['~']); }// approximately         
              else {serconn.write(27);} // write an asteriks * for any unsupported UTF char
            }
            else { serconn.write(27); } // write an asteriks * for any unsupported UTF char
          }
          else if (zeichen >= 240 && zeichen <=244) { // 4 byte character
            i++;
            uint8_t zeichen4 = wstring[i];
            // currently no 4 byte characters supported
            serconn.write(27); // write an asteriks * for any unsupported UTF char
          }
          else { serconn.write(27); } // should not happen - is either a broken coding or we missend a start byte          
        }
        spalte++;
        i++;
        _wait = true;
        delay(50);  // RTS will not change stange instantly 
      }
      else {
        delay(10);
      }
    }
    SetSimplexMode();
  }

  void WriteLn(String wstring)
  {
    Write(wstring);
    NewLine();
  }

  void NewLine()
  {
    serconn.write(uni2ddr['\n']);
    //serconn.write(ascii2ddr['\r']);
    spalte = 0;
  }

  // Maschine bereit für nächstes Zeichen
  bool IsReady()
  {
    int rtsState = digitalRead(_rtsPin);
    if (rtsState == LOW) {
        neopixelShowReady();  // I know, bad style...
    }
    else {
        neopixelShowBusy();  // I know, bad style...
    }
    return rtsState == LOW; 
  }

  void writeCommand(int commandCode) {
    if(IsReady()) {
      serconn.write(commandCode);
      delay(100);
    }
  }

  /*
  Trennmode aktivieren (Trennung von Tastatur und Druckwerk)
  Duplexbetrieb: Alle Tasteninformationen werden nur nach TxD ausgegeben und
  nur die von RxD kommenden gelangen zum Druck (Korr, CREL und CRL sind nicht wirksam!)
  */ 
  void SetDuplexMode(){
    delay(500);
    writeCommand(0x91);
    delay(100);
  }

  /*
  Trennmode aufheben
  Simplexbetrieb: Alle Tasteninformationen werden gedruckt
  */
  void SetSimplexMode(){
    delay(1000);
    writeCommand(0x92);
    delay(100);
  }
};

//int max_tokens = 356;          // Für längere Antworten erhöhen
//char Nachricht_array[6096];    // Bei größeren max_tokens das Nachricht_array auch vergrößern

// Instantiiere die Schreibmaschine
ErikaSchreibmaschine erika(ERIKA_RX, ERIKA_TX, RTS_PIN, ERIKA_BAUD);
//ErikaSchreibmaschine erika();

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            Serial.println("Connected to access point");
            neopixel.setPixelColor(0, neopixel.Color(0, 0, 255));
            neopixel.show();
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to access point");
            neopixel.setPixelColor(0, neopixel.Color(0, 255, 0));
            neopixel.show();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            neopixel.setPixelColor(0, neopixel.Color(255, 0, 0));
            neopixel.show();
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Serial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Serial.println("STA IPv6 is preferred");
            break;
        default: break;
    }
}

String getDefaultMacAddress() {

  String mac = "";

  unsigned char mac_base[8] = {0};

  if (esp_efuse_mac_get_default(mac_base) == ESP_OK) {
    char buffer[24];  // 8*2 characters for hex + 7 characters for colons + 1 character for null terminator
#ifdef CONFIG_SOC_IEEE802154_SUPPORTED
    snprintf(
      buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5],
      mac_base[6], mac_base[7]
    );
#else
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
#endif
    mac = buffer;
  }

  return mac;
}

void buttonPressed() {
  wmOnDemandStart = true;
}

void setup() {  
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp can default to STA+AP
  Serial.begin(115200);
  delay(1000);
  // Serial.setDebugOutput(true);

  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  preferences.begin("Erika", true);
  url = preferences.getString("url", "");       // wm.addParameter(&custom_chat_server_url);
  authToken = preferences.getString("authToken", ""); // wm.addParameter(&custom_chat_server_token);
  authMode = preferences.getString("authMode", "Bearer");
  model = preferences.getString("model", ""); // wm.addParameter(&custom_aimodel_id);
  systemprompt = preferences.getString("systemprompt", "Du bist ein hilfreicher Assistent, der kurze Antworten gibt.");  // wm.addParameter(&custom_aimodel_systemprompt);
  helloMsg = preferences.getString("helloMsg", "Hallo, ich bin Erika.");
  logURL = preferences.getString("logURL", ""); // wm.addParameter(&custom_log_server_url);
  hwVersionStr = preferences.getString("hwVersion", "3");

  preferences.getString("configAPpass", "MakeErikaGreatAgain").toCharArray(configAPpass, 32);

  preferences.end();

  hwVersion = hwVersionStr.toInt();

  Serial.print(F("Erika Dongle "));
  Serial.print("Chip ID: ");
  Serial.print(chipId);
  Serial.print(" MAC: ");
  Serial.println(getDefaultMacAddress());

  Serial.print(F("HW version: "));
  Serial.print(hwVersion);
  Serial.print(F(" SW version: "));
  Serial.println(__DATE__);

  if (hwVersion < 3) {
    rtsPin = RTS_PIN_V2;
  }
  else {
    rtsPin = RTS_PIN_V3;
  }

  if (hwVersion < 2) {
    ONDDEMANDPIN = WM_ONDEMAND_PIN_V1;
  }
  else {
    ONDDEMANDPIN = WM_ONDEMAND_PIN_V2;
  }
  attachInterrupt(ONDDEMANDPIN, buttonPressed, FALLING);

  pinMode(WM_ONDEMAND_PIN, INPUT_PULLUP);
  pinMode(DTD_PIN, OUTPUT);
  digitalWrite(DTD_PIN, LOW);

  #ifdef PIN_NEOPIXEL_POWER
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  digitalWrite(PIN_NEOPIXEL_POWER, HIGH);  
  #endif

  neopixelInit();

  url.toCharArray(chat_server_url, min(url.length()+1, (uint) 120));
  authToken.toCharArray(chat_server_token, min(authToken.length()+1, (uint) 255));
  model.toCharArray(aimodel_id, min(model.length()+1, (uint) 64));
  systemprompt.toCharArray(aimodel_systemprompt, min(systemprompt.length()+1, (uint) 255));
  helloMsg.toCharArray(hello_msg, min(helloMsg.length()+1, (uint) 255));
  logURL.toCharArray(log_server_url, min(logURL.length()+1, (uint) 120));
  hwVersionStr.toCharArray(hw_version, min(hwVersionStr.length()+1, (uint) 2));
  hwVersion = hwVersionStr.toInt();

  neopixelShowInit();
  delay(2000);
  neopixelShowWiFiConnect();
  wm.setEnableConfigPortal(false); // FOR autoconnect - **do not** start config portal if autoconnect failed

  setupWM();
  
  if (shouldSaveParam) {
    url = chat_server_url;
    authToken = chat_server_token;
    model = aimodel_id;
    systemprompt = aimodel_systemprompt;
    helloMsg = hello_msg;
    logURL = log_server_url;
    hwVersionStr = hw_version;

    url.trim();
    logURL.trim();
    
    preferences.begin("Erika", false);
    preferences.putString("url", url);
    preferences.putString("authToken", authToken);
    preferences.putString("model", model);
    preferences.putString("systemprompt", systemprompt);
    preferences.putString("temperature", temperature);
    preferences.putString("helloMsg",  helloMsg);
    preferences.putString("logURL", logURL);
    preferences.putString("hwVersion", hwVersionStr);
    preferences.putString("configAPpass", configAPpass);
    preferences.end();

    delay(500);
    ESP.restart();
  }
  else {
    if (WiFi.status() == WL_CONNECTED) {
      neopixelShowReady();
      delay(1000);
    }
    else {
      Serial.println("WiFi connect failed, giving up");
      for (int i=0;i<5;i++) {
        neopixelShowFail();
        delay(500);
        neopixelShowWiFiConnect();
        delay(500);
      }
      ESP.restart();
    }
  }

  Serial.println("Initializing Erika and waiting for her...");
  erika.Init(rtsPin); //debug

  delay(2000);

  erika.NewLine(); //debug
  erika.WriteLn(helloMsg); //debug
  erika.NewLine();  //debug
}

void loop() {
  // EINGABE ###############################################################################
  String eingabe= erika.ReadLine(); //waits until \n
  
  Serial.println(eingabe);
  
  // EINGABE ENDE ##########################################################################

  // AUSGABE ###############################################################################

  if (eingabe.length() == 0) {
    Serial.println("leere Zeile");
  }
  else {
    // Request
    Serial.println();
    Serial.print("Sende API Request an: "); Serial.println(url);    

    JsonDocument jsonaiapi;
    //jsonaiapi["input"] = eingabe; // for /v1/response endpoint
    jsonaiapi["model"] = model;    
    jsonaiapi["messages"][0]["role"] = "system";
    jsonaiapi["messages"][0]["content"] = systemprompt;
    jsonaiapi["messages"][1]["role"] = "user";
    jsonaiapi["messages"][1]["content"] = eingabe;
    jsonaiapi["temperature"] = 0.7;
    jsonaiapi["stream"] = false;

    String request;
    serializeJson(jsonaiapi, request);

    Serial.print("request: "); Serial.println(request);

    WiFiClient client;
    WiFiClientSecure clients;
    clients.setInsecure();   // we do not check certificates or checksums
    
    HTTPClient http;
    http.setTimeout(60000); //server response might take a while

    http.addHeader("Content-Type", "application/json; charset=utf-8");
    http.addHeader("Authorization", authMode + " " + authToken);
    http.addHeader("Accept", "application/json");

    neopixelShowConnect();

    bool connectionResult;

    if (url.startsWith("https")) {
     connectionResult = http.begin(clients, url);
    }
    else {
      connectionResult = http.begin(client, url);
    }

    if (connectionResult) {

      Serial.print("[HTTP] POST...");

      unsigned long millisStart = millis();

      int httpCode = http.POST(request);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled        
        Serial.printf(" successful: %s\n", http.errorToString(httpCode).c_str());      

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          unsigned long millisDuration = millis()-millisStart;
          neopixelShowReady();
          String payload = http.getString();
          Serial.println(payload);

          JsonDocument jsondoc;
          deserializeJson(jsondoc, payload);
          payload.clear();            
          Serial.println("geparst: ");
          
          http.end();

          String antwort = jsondoc["choices"][0]["message"]["content"];
          Serial.println(antwort);

          if (logURL != "") {
            HTTPClient httplog;
            if (logURL.startsWith("https")) {
              connectionResult = httplog.begin(clients, logURL);
            }
            else {
              connectionResult = httplog.begin(client, logURL);
            }
            
            httplog.setTimeout(10000);
            httplog.addHeader("Content-Type", "application/json; charset=utf-8");  

            if (connectionResult) {                          
              JsonDocument jsonlog;
              jsonlog["request"] = eingabe;
              jsonlog["response"] = antwort;
              jsonlog["systemprompt"] = systemprompt;
              jsonlog["model"] = model;
              jsonlog["duration"] = millisDuration;
              jsonlog["device"] = chipId;

              String logString;
              serializeJson(jsonlog, logString);
              
              Serial.print("[HTTP] POST Logging ... ");
              
              int httpCodeLog = httplog.POST(logString);

              // httpCode will be negative on error
              if (httpCodeLog > 0) {
                Serial.printf(" successful: %s\n", httplog.errorToString(httpCodeLog).c_str());
              } else {
                Serial.printf(" failed, error: %s\n", httplog.errorToString(httpCodeLog).c_str());                
              }
              httplog.end();
            } else {
              Serial.printf("[HTTP] Unable to connect to log server\n");            
            }
          }

          erika.NewLine();
          erika.WriteLn(antwort);
        }
      } else {
        Serial.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        neopixelShowFail();
        delay(1000);
      }
    } else {
      Serial.printf("[HTTP] Unable to connect\n");
      neopixelShowFail();
      delay(1000);
    }

    Serial.println("Request+response fertig");
    erika.WriteLn("======");
    erika.NewLine();
    // AUSGABE ENDE ##########################################################################  
  }
}
