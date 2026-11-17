class SerialTerminal
{
  private:
  int _txPin, _rxPin, _rtsPin, _baud;
  #ifdef SWSERIAL
  SoftwareSerial serconn;
  #else
  #define serconn HWSERIAL
  #endif
  size_t num_spalten;
  size_t spalte;

  public: 
  SerialTerminal(int rxPin, int txPin, int rtsPin, int baud)
   #ifdef SWSERIAL
   :spalte(0), num_spalten(80), serconn(_rxPin, _txPin) // RX, TX
   #else
   :spalte(0), num_spalten(80)
   #endif
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
      char zeichen = (char)znummer;
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
      if(CharAvailable())
      {
        int ddrZeichen = serconn.read();
        char16_t uniZeichen = ddrZeichen;                             // Druckbare Zeichen anhängen 

        uniZeichenfolge.push_back(uniZeichen);

        // breaker
        if(uniZeichen == '\n') 
        {
          uniZeichenfolge.pop_back();
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
   // Serial.printf("WriteC: %c\n", c);
    /*if(c=='@')
    {
      serconn.write((char)uni2ddr['O']);
      serconn.write(114); // Backtab
      serconn.write((char)uni2ddr['a']);
      return;
    }*/
    char z = (char)c;
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
    serconn.write('\n');
    spalte = 0;
  }

  // Maschine bereit für nächstes Zeichen
  bool IsReady()
  {
    int rtsState = digitalRead(_rtsPin);
    return rtsState == LOW; 
  }
};

