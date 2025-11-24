#include <WiFi.h>             
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "utf8coder.hpp"
#include "ddr2unicode.h"
#include "unicode2ddr.h"

#define PC_BAUD 9600

//#define DTD_PIN 14
#define RTS_PIN 23
#define ERIKA_RX 18
#define ERIKA_TX 19
#define ERIKA_BAUD 1200

#define MISTRAL_KOBOLD

class ErikaSchreibmaschine
{
  private:
  int _txPin, _rxPin, _rtsPin, _baud;
  SoftwareSerial serconn;
  size_t num_spalten;
  size_t spalte;

  public: 
  ErikaSchreibmaschine(int rxPin, int txPin, int rtsPin, int baud)
   :spalte(0), num_spalten(80), serconn(_rxPin, _txPin) // RX, TX
  {
    _rtsPin=rtsPin;
    _txPin=txPin;
    _rxPin=rxPin;
    _baud=baud;
  }

  bool Init()
  {
    pinMode(_rtsPin, INPUT);
    serconn.begin(_baud);
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
      char zeichen = ddr2uni[znummer];
      return zeichen;
     }
     return 0;
  }

  // Liest eine ganze Zeile bis \n und liefert das als utf-8 kodierten string
  String ReadLine()
  {
    String eingabestring;
    std::vector<uint16_t> uniZeichenfolge;
    while(true)
    {
      if(!CharAvailable())
      {
        int ddrZeichen = serconn.read();
        char16_t uniZeichen = ddr2uni[ddrZeichen];
        switch (ddrZeichen) 
        {
          // case 71:  { Eingabe = Eingabe + "ss"; break; } // Eszett zu ss
          // case 101: { Eingabe = Eingabe + "ae"; break; } // ä zu ae
          // case 102: { Eingabe = Eingabe + "oe"; break; } // ö zu oe
          // case 103: { Eingabe = Eingabe + "ue"; break; } // ü zu ue
          case 114:  // Backtab 
            { 
              if(uniZeichenfolge.size()>1)
              {
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
            if (ddrZeichen > 127) {} // nothing - kann nicht sein
            else // gutes Zeichen auf der Erikatastatur
            {
              uniZeichenfolge.push_back(uniZeichen);
            }
            break;
          }
        };
        // breaker
        if(uniZeichen == '\n') 
        {
          break;
        }
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

  void WriteC(char16_t c)
  {
    Serial.printf("WriteC: %c\n", c);
    if(c=='@')
    {
      serconn.write((char)uni2ddr['O']);
      serconn.write(114); // Backtab
      serconn.write((char)uni2ddr['a']);
      return;
    }
    char z = (char)uni2ddr[c];
    serconn.write(z);
    spalte++;
  }

  void Write(String wstring)
  {
    for (auto it = wstring.begin(); it != wstring.end(); it++)
    {
      WriteC(*it);
    }
  }
  
  void WriteLn(String wstring)
  {
    for (auto it = wstring.begin(); it != wstring.end(); it++)
    {
      WriteC(*it);
    }
    NewLine();
  }

  void NewLine()
  {
    serconn.write(uni2ddr['\n']);
    //serconn.write(ascii2ddr['\r']);
    spalte = 0;
  }

  // Maschin kann nächstes Z
  bool IsReady()
  {
    int rtsState = digitalRead(_rtsPin);
    return rtsState == LOW; 
  }
};


bool wait = false;


const char* ssid     = "..."; // hier eigenen WLan_Namen eintragen 
const char* password = "..."; // hier eigenes WLan_Passwort eintragen 

String WiFi_name;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
int WiFi_Count;

int max_tokens = 356;          // Für längere Antworten erhöhen
char Nachricht_array[6096];    // Bei größeren max_tokens das Nachricht_array auch vergrößern

#ifdef MISTRAL_KOBOLD

String authHeader = "Authorization: ...";
const char* hostname = "...";
String url = "/....";


#endif

String Ausgabe = "";
int Zeilenlaenge = 70;         // Auf der Erika ausgegebene Zeichen pro Zeile. Verringern falls Zeichen abgeschnitten werden
int Nachricht_len = 0;
int Stelle = 0;

// Instantiiere die Schreibmaschine
ErikaSchreibmaschine erika(ERIKA_RX, ERIKA_TX, RTS_PIN, ERIKA_BAUD);


void setup()
{
  delay(1000);
  Serial.begin(115200);
  delay(100);
  Serial.println("geht los ...");
  erika.Init();
  // Mit dem WLan verbinden 
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);

  
  erika.WriteLn("Hallo ich bin de Erika. Verbinde.. ");
  Serial.println("Verbinde mit WLAN ...");
  while (WiFi.status() != WL_CONNECTED) 
  { 
    Serial.print('.'); 
    delay(1000); 
  } 
  Serial.println(WiFi.localIP());
  erika.WriteLn(String("Mit ") + ssid);
}

void loop()
{
  // EINGABE ###############################################################################
  String eingabe= erika.ReadLine();

  Serial.println(eingabe);

  // Request    
      String getResponse = "";
      int merker = 0;
    
      Serial.printf("Sende API Request an %s.", hostname);
    
      WiFiClientSecure client_tcp;
      client_tcp.setInsecure();   //run version 1.0.5 or above

      // Request formatieren. Mit Streaming.
      String request;
      request = "{\"model\":\"gpt-4.0-turbo\",\"stream\":true,\"prompt\":\"[INST]" + eingabe + "[/INST]\",\"temperature\":0.7,\"max_tokens\":" + String(max_tokens) + ",\"frequency_penalty\":0,\"presence_penalty\":0.6,\"top_p\":1.0}";
      Serial.print("request: "); Serial.println(request);
    
      if (client_tcp.connect(hostname, 443)) 
      {
        client_tcp.println(String("POST ") + url + " HTTP/1.1");
        client_tcp.println("Connection: close");
        client_tcp.println(String("Host: ") + hostname);
        client_tcp.println(authHeader);
        client_tcp.println("Content-Type: application/json; charset=utf-8");
        client_tcp.println("Content-Length: " + String(request.length()));
        client_tcp.println();
        client_tcp.println(request);
    
        boolean state = false;
        int waitTime = 150000;   // timeout 15 seconds
        long startTime = millis();
        while ((startTime + waitTime) > millis()) 
        {
          Serial.print(".");
          delay(1000);
          // UPDATE 21.06.2023
          String puffer;
          while (client_tcp.available()) 
          {
              char c = client_tcp.read();
              puffer += c;
              if(c=='\n')
              {
                // Ein Response-JSON-Block angekommen - es kommen mehrere mit nur wenigen zeichen
                JsonDocument jsondoc;
                deserializeJson(jsondoc, puffer);
                puffer.clear();
                Serial.println("geparst");
                if(jsondoc["index"] == -1)
                {
                  client_tcp.stop(); 
                }

                String teilantwort = jsondoc["choices"][0]["text"];
                Serial.printf("Extrahierte Teilantwort: %s", teilantwort);

                erika.Write(teilantwort);

              
              }
              startTime = millis();
          }
        }
        client_tcp.stop();
      }
      else 
      {
        Serial.println("Connection failed");
      }
 
  // EINGABE ENDE ##########################################################################
  // AUSGABE ###############################################################################

  Serial.println("Request+response fertig");
  erika.WriteLn("======");
  // AUSGABE ENDE ##########################################################################  
  
}
