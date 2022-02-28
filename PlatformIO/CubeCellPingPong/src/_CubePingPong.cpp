/* Heltec Automation Ping Pong communication test example
 *
 * Function:
 * 1. Ping Pong communication in with CubeCell devices.
 * 2. Dynamic switch between client and server by pressing USER_KEY
 * 3. client sends input from serial port via LoRa
 * 4. server echos received data in uppercase
 * 
 * Description:
 * 1. Only hardware layer communicate, no LoRaWAN protocol support;
 * 2. Download the same code into two CubeCell devices, then they will begin Ping Pong test each other;
 * 3. This example is for CubeCell hardware basic test.
 *
 * HelTec AutoMation, Chengdu, China, www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/HelTecAutomation/ASR650x-Arduino
 * */
#include <Arduino.h>
#include "LoRaWan_APP.h"

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef NOBLINK
#define LoraWan_RGB 1
#include "CubeCell_NeoPixel.h"
CubeCell_NeoPixel LED(1, RGB, NEO_GRB + NEO_KHZ800);
#endif
#ifndef NO_OLED
    #ifdef CubeCell_BoardPlus
        #include "HT_SH1107Wire.h"
        //SH1107Wire  display(0x3c, 500000, SDA, SCL ,GEOMETRY_128_64,GPIO10); // addr, freq, sda, scl, resolution, rst
        extern SH1107Wire  display;
    #endif
    #ifdef CubeCell_GPS
        #include "HT_SSD1306Wire.h"
        //SSD1306Wire  display(0x3c, 500000, I2C_NUM_0,GEOMETRY_128_64,GPIO10 ); 
        extern SSD1306Wire  display;
    #endif
//char str[32];
//extern SH1107Wire display; //Use ASR6502 arduino package display
#include "HT_DisplayUi.h"
#include "iconMeshTastic.xbm"
#include "CDP48.xbm"

#endif



#define RF_FREQUENCY                915000000 // Hz

#define TX_OUTPUT_POWER             18        // dBm

#define LORA_BANDWIDTH              0         // [0: 125 kHz,
                                              //  1: 250 kHz,
                                              //  2: 500 kHz,
                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR       7         // [SF7..SF12]
#define LORA_CODINGRATE             1         // [1: 4/5,
                                              //  2: 4/6,
                                              //  3: 4/7,
                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define RX_TIMEOUT_VALUE            1000
#define BUFFER_SIZE                 60 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

void displayInfo(bool displaySend, bool displayReceive);
void stringUpper(char string[]);

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

typedef enum
{
  WAIT,
  WAIT_INPUT,
  RX,
  TX,
  SWITCH_MODE
}States_t;

States_t state;
int16_t Rssi, rxSize, txSize;
int8_t Snr;

// decides if the device acts as a client or a server
// switch with user key: short press -> client, long press -> server
bool isServer = false;



void OnTxDone( void )
{
  Serial.print("TX done......");
  turnOnRGB(0,0);
  state = RX;
}

void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.println("TX Timeout.");
  state = SWITCH_MODE;
}
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
  Rssi = rssi;
  Snr = snr;
  rxSize = size;
  memcpy(rxpacket, payload, size );
  rxpacket[size] = '\0';
  turnOnRGB(COLOR_RECEIVED,0);
  Radio.Sleep( );

  Serial.printf("RX \"%s\" with SNR %d and Rssi %d , length %d\r\n",rxpacket,Snr, Rssi,rxSize);

  if (isServer) {
    displayInfo(false, true);
    state = TX;
  } else {
    displayInfo(true, true);
    txSize = 0;
    state = WAIT_INPUT;
  }
  turnOnRGB(0,0);
}

void displayInfo(bool displaySend, bool displayReceive)
{
  display.clear();
  display.drawString(0, 0, isServer ? "Server" : "Client");
  display.drawString(0, 15,  "TX =>");
  if (displaySend)
    display.drawString(30, 15, txpacket);
  display.drawString(0, 30,  "RX <=");
  if (displayReceive) {
    display.drawString(30, 30, rxpacket);
    display.drawString(0, 45, "SNR " + String(Snr,DEC) + ", RSSI  " + String(Rssi,DEC));
  }
  display.drawXbm(128-icon_width, 0, icon_width, icon_height, icon_bits);
  //display.drawXbm(128-duck_width, 0, duck_width, duck_height, duck_bits);
  
  display.display();
}

void userKey(void)
{
  delay(10);
  if(digitalRead(P3_3) == 0)
  {
    uint16_t keyDownTime = 0;
    while(digitalRead(P3_3 )== 0)
    {
      delay(1);
      keyDownTime++;
      if(keyDownTime>=800)
        break;
    }
    if(keyDownTime<800)
    {
      isServer = false;
    }
    else
    {
      isServer = true;
    }
    state = SWITCH_MODE;
  }
}

void stringUpper(char string[]) 
{
  int i = 0;
  while (string[i] != '\0') 
  {
    if (string[i] >= 'a' && string[i] <= 'z') {
      string[i] = string[i] - 32;
    }
    i++;
  }
}



void setup() {
  Serial.begin(115200);

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW); 
  delay(100);

  display.init();
  //display.flipScreenVertically();

  pinMode(P3_3,INPUT);
  attachInterrupt(P3_3,userKey,FALLING);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                 LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                 LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

  state = SWITCH_MODE;
}

void loop()
{
  switch(state)
  {
    case TX:
      turnOnRGB(COLOR_SEND,0);

      if (isServer) {
        memcpy(txpacket, rxpacket, rxSize); //copy rx to tx
        txSize = rxSize;
        txpacket[txSize] = '\0';
        stringUpper(txpacket); //convert to UpperCase
        displayInfo(true, true);
      } else {
        displayInfo(true, false);
      }
      Serial.printf("\r\nTX: \"%s\"\r\n", txpacket);
      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) );
      state = WAIT;
      break;
    case RX:
      Serial.println("into RX mode");
      Radio.Rx( 0 );
      state = WAIT;
      break;
    case WAIT_INPUT:
      while(Serial.available() > 0) {
        int input = Serial.read();
        if (input == '\n') {
          // terminate string and send
          txpacket[txSize] = '\0';
          state = TX;
          break;
        } else {
          txpacket[txSize] = input;
          txSize++;
          // overwrite buffer from begin if too small
          if(txSize >= BUFFER_SIZE)
            txSize = 0;
        }
      }
      break;
    case SWITCH_MODE:
      if (isServer) {
        state = RX;
      } else {
        txSize = 0;
        state = WAIT_INPUT;
      }
      displayInfo(false, false);

      break;
    case WAIT:
    default:
      break;
  }
  Radio.IrqProcess( );
}
