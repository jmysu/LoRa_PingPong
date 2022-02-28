#include <Arduino.h>
/*
 * OLED Display & UI test
 *  	for ttgo_lora32 & heltec_lora32
 *
*/
#include "Wire.h"
#include "SSD1306Wire.h"
#include "OLEDDisplay.h"
#include "OLEDDisplayUi.h"
#include "iconMesh.xbm"
#define TTGO_VOL_PIN 35
#define HELTEC_V2_0_VOL  13       // Set this to switch between GPIO13(V2.0) and GPIO37(V2.1) for VBatt ADC.
#define HELTEC_V2_1_VOL  37       // Set this to switch between GPIO13(V2.0) and GPIO37(V2.1) for VBatt ADC.
#if defined(ARDUINO_TTGO_LoRa32_V1) 
   SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);
   #define RST_OLED OLED_RST 
   #define VOL_PIN TTGO_VOL_PIN
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32_V2)
   SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED); 
   #define VOL_PIN HELTEC_V2_0_VOL
#endif

OLEDDisplayUi ui( &display );
const uint8_t activeSymbol[] PROGMEM = {
    B00000000,
    B00000000,
    B00011000,
    B00100100,
    B01000010,
    B01000010,
    B00100100,
    B00011000
};
const uint8_t inactiveSymbol[] PROGMEM = {
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00011000,
    B00011000,
    B00000000,
    B00000000
};

uint64_t chipid;

void logo(){
    display.clear();
	display.drawXbm((display.getWidth()-iconMesh_width)/2, (display.getHeight()-iconMesh_height)/2, iconMesh_width, iconMesh_height, (const unsigned char *)iconMesh_bits);
	display.display();
}

//Extern power control (only Heltec LoRa32 V2)
void PowerExt(bool on)
{
	#if defined(heltec_wifi_lora_32_V2)
		pinMode(Vext, OUTPUT);
		if (on) 
			digitalWrite(Vext, LOW);
		else
			digitalWrite(Vext, HIGH);
	#endif
}

float getBatteryVoltage(int nbMeasurements) {
    float voltageDivider = 338;
    // int readMin = 1080; // -> 338 * 3.2 // If you want to draw a progress bar, this is 0%
    // int readMax = 1420; // -> 338 * 4.2 // If you want to draw a progress bar, this is 100%

    // Take x measurements and average
    int readValue = 0;
    for (int i = 0; i < nbMeasurements; i++) {
        readValue += analogRead(VOL_PIN);
        delay(100);
    }
    
    float voltage = (readValue/nbMeasurements)/voltageDivider;
    return voltage;
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
 	// draw an xbm image.
 	display->drawXbm((display->getWidth()-iconMesh_width)/2, (display->getHeight()-iconMesh_height)/2, iconMesh_width, iconMesh_height, (const unsigned char *)iconMesh_bits);
}
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  //display->drawString(0 + x, 10 + y, "Arial 10");

  //display->setFont(ArialMT_Plain_16);
  //display->drawString(0 + x, 20 + y, "Arial 16");

  //display->setFont(ArialMT_Plain_24);
  //display->drawString(0 + x, 34 + y, "Arial 24");
  	uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
	//Serial.printf("ESP32ChipID=%04X ",(uint16_t)(chipid>>32));//print High 2 bytes
	//Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
	char buf[32];
	sprintf(buf, "ChipID:%04X-%08X", (uint16_t)(chipid>>32), (uint32_t)chipid );
	display->drawString(0, 32, buf);
  // Battery Voltage
  	//float voltage = getBatteryVoltage(analogRead(35));

	float voltage = (float)(analogRead(VOL_PIN)) / 4095*2*3.3*1.1;
    sprintf(buf, "%4.2fV", voltage);
  	display->setTextAlignment(TEXT_ALIGN_RIGHT);
  	display->drawString(128, 50, buf);
}

extern String strLoRA;
extern float rssiLoRA, snrLoRA;
void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	//LoRa
  char buf[32];
  sprintf(buf, "RSSI:%3.0f SNR:%3.0f", rssiLoRA, snrLoRA);
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 10, strLoRA);
  display->drawString(64, 32, buf);
  display->display();
}
// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3};
// how many frames are there?
int frameCount = 3;
// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;


extern void setupLoRA();
extern void loopLoRA();
void setup()
{
	PowerExt(1);

	// UART
	Serial.begin(115200);
	Serial.flush();
	while (!Serial) {}
	delay(500);
	Serial.print("Serial initial done\r\n");

	// OLED HW RESET before init
	pinMode(RST_OLED, OUTPUT);
	digitalWrite(RST_OLED, LOW);
	delay(50);
	digitalWrite(RST_OLED, HIGH);

	display.init();
	//display->flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	display.drawString(0, 0, "OLED initial done!");
	display.display();
	
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);
  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  // Add frames
  ui.setFrames(frames, frameCount);
  // Add overlays
  ui.setOverlays(overlays, overlaysCount);
  // Initialising the UI will init the display too.
  ui.init();

	logo();
	delay(300);

	chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
	Serial.printf("ESP32ChipID=%04X ",(uint16_t)(chipid>>32));//print High 2 bytes
	Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.

	pinMode(BUILTIN_LED, OUTPUT);
 
  setupLoRA();
}

extern bool gotLoRAdata;
void loop()
{
  loopLoRA();
  if (gotLoRAdata) {
    ui.switchToFrame(2); //switch to LoRA
    gotLoRAdata = false;
    }
  ui.update();

	if (ui.getUiState()->frameState == IN_TRANSITION)
		digitalWrite(BUILTIN_LED, HIGH);
	else
		digitalWrite(BUILTIN_LED, LOW);
	/*
	if (digitalRead(BUILTIN_LED)) {
		display.displayOff();
		PowerExt(0);
		}
	else {
 		display.displayOn();
		PowerExt(1);
		}	
	delay(1000);	
	*/
}