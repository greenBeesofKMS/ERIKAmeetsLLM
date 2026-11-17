#include <time.h>
#include <stdio.h>

//#define USEOTA
// enable OTA
#ifdef USEOTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif

#define CONFIGAPNAME "Erika-"
#define CONFIGAPPASS "MakeErikaGreatAgain"

char configAPname[32] = "";
char configAPpass[32] = "MakeErikaGreatAgain";

const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };

unsigned long mtime = 0;

//flag for saving data
bool shouldSaveParam = false;

char chat_server_url[120];
char chat_server_token[255];
char aimodel_id[64];
char aimodel_systemprompt[255];
char hello_msg[255];
char log_server_url[120];
char hw_version[2];

// TEST OPTION FLAGS
bool TEST_CP         = false; // always start the configportal, even if ap found
int  TEST_CP_TIMEOUT = 360; // test cp timeout

bool TEST_NET        = true; // do a network test after connect, (gets ntp time)
bool ALLOWONDEMAND   = true; // enable on demand
int  ONDDEMANDPIN    = WM_ONDEMAND_PIN; // gpio for button
bool WMISBLOCKING    = true; // use blocking or non blocking mode, non global params wont work in non blocking

uint8_t BUTTONFUNC   = 1; // 0 resetsettings, 1 configportal, 2 autoconnect

//callbacks
  // called after AP mode and config portal has started
  //  setAPCallback( std::function<void(WiFiManager*)> func );
  // called after webserver has started
  //  setWebServerCallback( std::function<void()> func );
  // called when settings reset have been triggered
  //  setConfigResetCallback( std::function<void()> func );
  // called when wifi settings have been changed and connection was successful ( or setBreakAfterConfig(true) )
  //  setSaveConfigCallback( std::function<void()> func );
  // called when saving either params-in-wifi or params page
  //  setSaveParamsCallback( std::function<void()> func );
  // called when saving params-in-wifi or params before anything else happens (eg wifi)
  //  setPreSaveConfigCallback( std::function<void()> func );
  // called just before doing OTA update
  //  setPreOtaUpdateCallback( std::function<void()> func );

void saveWifiCallback(){
  Serial.println("[CALLBACK] saveCallback fired");
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("[CALLBACK] configModeCallback fired");
  // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); 
  // Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  // Serial.println(myWiFiManager->getConfigPortalSSID());
  // 
  // esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  shouldSaveParam = true;
  wm.stopConfigPortal();
}


void handleRoute(){
  Serial.println("[HTTP] handle custom route");
  wm.server->send(200, "text/plain", "hello from user code");
}

void handleNotFound(){
  Serial.println("[HTTP] override handle route");
  wm.handleNotFound();
}

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);

  // you can override wm route endpoints, I have not found a way to remove handlers, but this would let you disable them or add auth etc.
  // wm.server->on("/info",handleNotFound);
  // wm.server->on("/update",handleNotFound);
  wm.server->on("/erase",handleNotFound); // disable erase
}

void handlePreOtaUpdateCallback(){
  Update.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("CUSTOM Progress: %u%%\r", (progress / (total / 100)));
  });
}


void wifiInfo(){
  // can contain gargbage on esp32 if wifi is not ready yet
  Serial.println("[WIFI] WIFI_INFO DEBUG");
  WiFi.printDiag(Serial);
  Serial.println("[WIFI] MODE: " + (String)(wm.getModeString(WiFi.getMode())));
  Serial.println("[WIFI] SAVED: " + (String)(wm.getWiFiIsSaved() ? "YES" : "NO"));
  Serial.println("[WIFI] SSID: " + (String)wm.getWiFiSSID());
  //Serial.println("[WIFI] PASS: " + (String)wm.getWiFiPass());
  // Serial.println("[WIFI] HOSTNAME: " + (String)WiFi.getHostname());
}


void getTime() {
  int tz           = -5;
  int dst          = 0;
  time_t now       = time(nullptr);
  unsigned timeout = 5000; // try for timeout
  unsigned start   = millis();
  configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2 ) { // what is this ?
    delay(100);
    Serial.print(".");
    now = time(nullptr);
    if((millis() - start) > timeout){
      Serial.println("\n[ERROR] Failed to get NTP time.");
      return;
    }
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void debugchipid(){
  // WiFi.mode(WIFI_STA);
  // WiFi.printDiag(Serial);
  // Serial.println(modes[WiFi.getMode()]);
  
  // ESP.eraseConfig();
  // wm.resetSettings();
  // wm.erase(true);
  WiFi.mode(WIFI_AP);
  // WiFi.softAP();
  WiFi.enableAP(true);
  delay(500);
  // esp_wifi_start();
  delay(1000);
  WiFi.printDiag(Serial);
  delay(60000);
  ESP.restart();
}

void setupWM() {
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.println("\n Starting WiFi manager");

  uint32_t chipId = 0;
	for(int i=0; i<17; i=i+8) {
	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}

  String apname = CONFIGAPNAME +  String(chipId);
  apname.toCharArray(configAPname, 32);

  //WiFi.setSleep(WIFI_NONE_SLEEP); // disable sleep, can improve ap stability

  //Serial.println("Error - TEST");
  //Serial.println("Information- - TEST");

  //Serial.println("[ERROR]  TEST");
  //Serial.println("[INFORMATION] TEST");  

  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN); // wifi_scan_method_t scanMethod
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL); // wifi_sort_method_t sortMethod - WIFI_CONNECT_AP_BY_SIGNAL,WIFI_CONNECT_AP_BY_SECURITY
  // WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);

  //wm.setDebugOutput(true, WM_DEBUG_DEV);
  //wm.debugPlatformInfo();

  //reset settings - for testing
  // wm.resetSettings();
  // wm.erase();


  // idea of overlayed input + dropdown: https://ekiwi.de/index.php/1999/html-und-javascript-eingabefeld-mit-dropdown-auswahl/
  const char *dropdown = R"(<div class='dropdown'>)";

  const char *selectUrl = R"(
        <select onchange='this.previousElementSibling.value=this.value; this.previousElementSibling.focus()'>
          <option>https://chat-ai.academiccloud.de/v1/chat/completions</option>
          <option>https://api.openai.com/v1/completions</option>
        </select>
    </div>
  )";

  WiFiManagerParameter custom_dropdown_url(dropdown);
  WiFiManagerParameter custom_chat_server_url("chat_server_url", "chat server URL (Open AI API /v1/chat/completions compatible)", chat_server_url, 120, "type='url'");
  WiFiManagerParameter custom_select_url(selectUrl);

  WiFiManagerParameter custom_chat_server_token("chat_server_token", "api token", chat_server_token, 255);

  //https://chat-ai.academiccloud.de/models  
  /*2025*/
  /*const char *selectModel = R"(
        <select onchange='this.previousElementSibling.value=this.value; this.previousElementSibling.focus()'>
            <option>gemma-3-27b-it</option>
            <option>llama-3.3-70b-instruct</option>
            <option>llama-3.1-sauerkrautlm-70b-instruct</option>
            <option>mistral-large-instruct</option>
        </select>
    </div>
  )";*/

  /*2026*/
  const char *selectModel = R"(
        <select onchange='this.previousElementSibling.value=this.value; this.previousElementSibling.focus()'>
            <option>gemma-4-31b-it</option>
            <option>openai-gpt-oss-120b</option>
            <option>meta-llama-3.1-8b-instruct</option>
            <option>mistral-medium-3.5-128b</option>
            <option>qwen3.6-27b</option>
        </select>
    </div>
  )";

  const char *selectLogServer = R"(
        <select onchange='this.previousElementSibling.value=this.value; this.previousElementSibling.focus()'>
            <option></option>
        </select>
    </div>
  )";

  /*const char *helloMsgTextarea = R"(
     <label for='hello_msg'>hello message</label><textarea id='hello_msg' name='hello_msg' rows='5' cols='51' maxlenght='255' wrap='soft'></textarea>
  )";*/

  WiFiManagerParameter custom_dropdown_model(dropdown);
  WiFiManagerParameter custom_aimodel_id("aimodel_id", "AI model id", aimodel_id, 64, "type='text'");
  WiFiManagerParameter custom_select_model(selectModel);

  WiFiManagerParameter custom_aimodel_systemprompt("aimodel_systemprompt", "system prompt", aimodel_systemprompt, 255, "type='text'");
  WiFiManagerParameter custom_hello_msg("hello_msg", "hello message", hello_msg, 255, "type='text'");
  //WiFiManagerParameter custom_hello_msg(helloMsgTextarea);

  WiFiManagerParameter custom_dropdown_logserver(dropdown);
  WiFiManagerParameter custom_log_server_url("log_server_url", "log server URL (empty for disabling)", log_server_url, 120, "type='url'");
  WiFiManagerParameter custom_select_logserver(selectLogServer);

  WiFiManagerParameter custom_hw_version("hw_version", "hardware version of the dongle (see sticker)", hw_version, 2, "type='number' min='1' max='3' step='1' inputmode=decimal");

  WiFiManagerParameter custom_configAPpass("configAPpass", "web portal access point WiFi password", configAPpass, 32);
  WiFiManagerParameter custom_section_AP( R"(<h2>Web Portal Configuration</h2>)");
  WiFiManagerParameter custom_section_HW( R"(<hr/><h2>Hardware Configuration</h2>)");
  WiFiManagerParameter custom_section_LLM( R"(<hr/><h2>LLM Configuration</h2>)");

  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setPreOtaUpdateCallback(handlePreOtaUpdateCallback);

  wm.addParameter(&custom_section_AP);
  wm.addParameter(&custom_configAPpass);
  
  wm.addParameter(&custom_section_HW);
  wm.addParameter(&custom_hw_version);

  wm.addParameter(&custom_section_LLM);
  wm.addParameter(&custom_dropdown_url);
  wm.addParameter(&custom_chat_server_url);
  wm.addParameter(&custom_select_url);
  wm.addParameter(&custom_chat_server_token);
  wm.addParameter(&custom_dropdown_model);
  wm.addParameter(&custom_aimodel_id);
  wm.addParameter(&custom_select_model);
  wm.addParameter(&custom_aimodel_systemprompt);
  wm.addParameter(&custom_hello_msg);
  wm.addParameter(&custom_dropdown_logserver);
  wm.addParameter(&custom_log_server_url);
  wm.addParameter(&custom_select_logserver);
  

  // set custom html head content , inside <head>
  // examples of favicon, or meta tags etc
  // const char* headhtml = "<link rel='icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mBx5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' />";
  // const char* headhtml = "<meta name='color-scheme' content='dark light'><style></style><script></script>";
  // wm.setCustomHeadElement(headhtml);

  //        height: 1.5em;

  wm.setCustomHeadElement(R"(<style>
    .dropdown {
        position: relative;
        width: 100%;
    }
    .dropdown select {
        width: 100%;
        padding: 3px;
    }
    .dropdown > * {
        box-sizing: border-box;
    }
    .dropdown input {
        position: absolute;
        width: calc(100% - 30px);
    }
    helloMsgTextarea {
        font-family: monospace;
    }
  </style>)");


  // set custom html menu content , inside menu item "custom", see setMenu()
  //const char* menuhtml = "<form action='/custom' method='get'><button>AI config</button></form><br/>\n";
  //wm.setCustomMenuHTML(menuhtml);

  // invert theme, dark
  wm.setDarkMode(false);

  // show scan RSSI as percentage, instead of signal stength graphic
  // wm.setScanDispPerc(true);

/*
  Set cutom menu via menu[] or vector
  const char* menu[] = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
  wm.setMenu(menu,9); // custom menu array must provide length
*/

  std::vector<const char *> menu = {"param","wifi","wifinoscan","info","custom","close","sep","erase","update","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector
  
  //wm.setParamsPage(false); // move params to seperate page, not wifi, do not combine with setmenu!

  // set STA static ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // wm.setShowStaticFields(false);
  // wm.setShowDnsFields(false);

  // set AP static ip
  // wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // wm.setAPStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); 

  // set country
  // setting wifi country seems to improve OSX soft ap connectivity, 
  // may help others as well, default is CN which has different channels
  wm.setCountry("DE"); // crashing on esp32 2.0

  // set Hostname
  // wm.setHostname(("WM_"+wm.getDefaultAPName()).c_str());
  // wm.setHostname("WM_RANDO_1234");

  // set custom channel
  // wm.setWiFiAPChannel(13);
  
  // set AP hidden
  // wm.setAPHidden(true);

  // show password publicly in form
  // wm.setShowPassword(true);

  // sets wether wm configportal is a blocking loop(legacy) or not, use wm.process() in loop if false
  // wm.setConfigPortalBlocking(false);
  
  if(!WMISBLOCKING){
    wm.setConfigPortalBlocking(false);
  }

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout(10*TEST_CP_TIMEOUT);
  
  // set min quality to show in web list, default 8%
  // wm.setMinimumSignalQuality(50);

  // set connection timeout
  wm.setConnectTimeout(10);

  // set wifi connect retries
  wm.setConnectRetries(3);

  // connect after portal save toggle
  wm.setSaveConnect(false); // do not connect, only save

  // show static ip fields
  // wm.setShowStaticFields(true);
  
  // wm.startConfigPortal("AutoConnectAP", "password");
  
  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  // wm.setCleanConnect(true); // disconnect before connect, clean connect
  
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback

  // set custom webserver port, automatic captive portal does not work with custom ports!
  // wm.setHttpPort(8080);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration

  // use autoconnect, but prevent configportal from auto starting
  // wm.setEnableConfigPortal(false);

  //wifiInfo();

  // to preload autoconnect with credentials
  // wm.preloadWiFi("ssid","password");

  //if(!wm.autoConnect(CONFIGAPNAME, CONFIGAPPASS)) {
  //  Serial.println("failed to connect and hit timeout");
  //}
  //else
  
  //if(TEST_CP || (ALLOWONDEMAND && (digitalRead(ONDDEMANDPIN) == LOW))) { // we shall start the config portal
  if(TEST_CP || (ALLOWONDEMAND && wmOnDemandStart)) { // we shall start the config portal
    // delay(1000);
    neopixelShowWiFiPortal();
    if(TEST_CP) {
      Serial.println("TEST_CP ENABLED");
      wm.setConfigPortalTimeout(TEST_CP_TIMEOUT);
    }
    else {
      Serial.println("ONDEMAND ENABLED");
    }
    wm.startConfigPortal(configAPname, configAPpass);
  } 
  if(!wm.autoConnect(configAPname, configAPpass)) {
    Serial.println("failed to connect and hit timeout");
  } else {      
    //if you get here you have connected to the WiFi
    Serial.println("connected");
  }

  //wifiInfo();

  //read updated parameters
  strcpy(chat_server_url, custom_chat_server_url.getValue());
  strcpy(chat_server_token, custom_chat_server_token.getValue());
  strcpy(aimodel_id, custom_aimodel_id.getValue());
  strcpy(aimodel_systemprompt, custom_aimodel_systemprompt.getValue());
  strcpy(hello_msg, custom_hello_msg.getValue());
  strcpy(log_server_url, custom_log_server_url.getValue());
  strcpy(hw_version, custom_hw_version.getValue());
  strcpy(configAPpass, custom_configAPpass.getValue());

  #ifdef USEOTA
    ArduinoOTA.begin();
  #endif
}