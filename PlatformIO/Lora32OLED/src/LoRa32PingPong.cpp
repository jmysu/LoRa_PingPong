/*
   RadioLib SX127x Ping-Pong Example

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/


  // LoRa Settings for both LoRA32 & CubeCell  
  #define BAND 915E6
  #define SpreadingFactor 7
  #define SignalBandwidth 125E3
  #define PreambleLength 8
  #define CodingRateDenominator 5
  #define SyncWord 0x12
  //-----------------------------------------

*/
#include <Arduino.h>
// include the library
#include <RadioLib.h>

// uncomment the following only on one
// of the nodes to initiate the pings
#define INITIATING_NODE

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// NRST pin:  9
// DIO1 pin:  3
//
//#if LORA_V1_0_OLED
//#define OLED_CLASS_OBJ  SSD1306Wire
//#define OLED_ADDRESS    0x3C
//#define OLED_SDA    4
//#define OLED_SCL    15
//#define OLED_RST    16
// SPI LoRa Radio
#define LORA_SCK   5   // GPIO5 - SX1276 SCK
#define LORA_MISO 19   // GPIO19 - SX1276 MISO
#define LORA_MOSI 27   // GPIO27 - SX1276 MOSI
#define LORA_CS   18   // GPIO18 - SX1276 CS
#define LORA_RST  14   // GPIO14 - SX1276 RST
#define LORA_IRQ  26   // GPIO26 - SX1276 IRQ (interrupt request)

SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1278 radio = RadioShield.ModuleA;

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// flag to indicate that a packet was sent or received
volatile bool operationDoneLoRA = false;
bool gotLoRAdata = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we sent or received  packet, set the flag
  operationDoneLoRA = true;
  gotLoRAdata = true;
}

void setupLoRA() {
  //Serial.begin(9600);

  // initialize SX127x with default settings
  Serial.print(F("[SX127x] Initializing ... "));
  // initialize SX127x
  // carrier frequency:           915.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            7
  // coding rate:                 5
  // sync word:                   X:0x1424 (private network)   V:0x12 compatible
  // output power:                22 dBm
  // current limit:               60 mA
  // preamble length:             8 symbols
  // CRC:                         enabled
  //int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = SX126X_SYNC_WORD_PRIVATE, int8_t power = 14, float currentLimit = 60.0, uint16_t preambleLength = 8, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
  int state = radio.begin(915.0, 125.0, 7);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    Serial.print(F("Freq Err:"));
    Serial.println(radio.getFrequencyError());
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
/*
 // set carrier frequency to 915 MHz
  if (radio.setFrequency(915.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
    }
  else
    Serial.print(F("\n915MHz ready!\n"));  

  //radio.setSyncWord(0x12);
  radio.setSpreadingFactor(7); //
*/
  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag);

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    Serial.print(F("[SX127x] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello LoRA!");
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
    Serial.print(F("[SX127x] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }
  #endif
}

String strLoRA;
float rssiLoRA, snrLoRA;
static unsigned long lastMillis=millis();
void loopLoRA() {
  // check if the previous operation finished
  if(operationDoneLoRA) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    operationDoneLoRA = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("[SX127x] Received packet!"));

        // print data of the packet
        Serial.print(F("[SX127x] Rx Data:\t\t"));
        Serial.println(strLoRA=str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX127x] Rx RSSI:\t\t"));
        Serial.print(rssiLoRA=radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX127x] Rx SNR:\t\t"));
        Serial.print(snrLoRA=radio.getSNR());
        Serial.println(F(" dB"));
      }
        // wait a second before transmitting again
        delay(1000);
        //if ((millis()-lastMillis)>1000) {
        //lastMillis = millis();
        // send another one
        Serial.print(F("[SX127x] Sending another packet ... "));
        transmissionState = radio.startTransmit("hello lora!");
        transmitFlag = true;

        //}
    }

    // we're ready to process more packets,
    // enable interrupt service routine
    enableInterrupt = true;

  }
}
