Adafruit_NeoPixel neopixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void neopixelInit() {
  neopixel.begin();
  neopixel.clear();
  neopixel.show();
  /*neopixel.setPixelColor(0,neopixel.Color(255, 0, 0));
  neopixel.show();
  delay(200);
  neopixel.setPixelColor(0,neopixel.Color(0, 255, 0));
  neopixel.show();
  delay(200);
  neopixel.setPixelColor(0,neopixel.Color(0, 0, 255));
  neopixel.show();
  delay(200);
  neopixel.setPixelColor(0,neopixel.Color(255, 255, 255));
  neopixel.show();
  delay(1000);*/
}

void neopixelShowInit() {
        neopixel.setPixelColor(0, neopixel.Color(255, 255, 255));
        neopixel.show();
}

void neopixelShowWiFiPortal() {
        neopixel.setPixelColor(0, neopixel.Color(255, 0, 255));
        neopixel.show();
}

void neopixelShowWiFiConnect() {
        neopixel.setPixelColor(0, neopixel.Color(0, 0, 255));
        neopixel.show();
}

void neopixelShowConnect() {
        neopixel.setPixelColor(0, neopixel.Color(0, 255, 255));
        neopixel.show();
}

void neopixelShowFail() {
        neopixel.setPixelColor(0, neopixel.Color(255, 0, 0));
        neopixel.show();
}

void neopixelShowReady() {
        neopixel.setPixelColor(0, neopixel.Color(0, 255, 0));
        neopixel.show();
}

void neopixelShowBusy() {
        neopixel.setPixelColor(0, neopixel.Color(255, 255, 0));
        neopixel.show();
}

void neopixelFade(int brightness) {
        neopixel.setBrightness(brightness);
        neopixel.show();
}