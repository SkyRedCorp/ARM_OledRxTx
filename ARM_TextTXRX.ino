/* Text transmission through RFM69 Radio Module
 *  and showed in OLED 128x32 px screen.
 *  
 * Written by Peter Tacon
 * February 2021
 */

// Libraries
#include <RH_RF69.h>
#include <SPI.h>
#include <RHReliableDatagram.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

//############RADIO CONFIG############

//Config Radio
#define RF69_FREQ 434.0
//Transmissor address
#define SKRTX_ADD 1
//Receptor Address
#define SKRX_ADD 2

//config board pin
#if defined(ADAFRUIT_FEATHER_M0) // Feather M0 w/Radio
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  #define LED           13
#endif

//config Button pin
#if defined(ADAFRUIT_FEATHER_M0) // Feather M0 w/Radio
  #define BUTTON_A     10
  #define BUTTON_B     11
  #define BUTTON_C     12
  #define LED           13
#endif

//Radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

//RX-TX package transmission
RHReliableDatagram rf69_manager(rf69, SKRTX_ADD);

//############OLED CONFIG############

//Screen dimension
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

//It's not used, but it's in the example, so ¯\_(ツ)_/¯
#define OLED_RESET 4 

//OLED driver
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Startup Logo (Finally I can make my own logo LOL)
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

//Binary Logo
 const unsigned char SkyRed [] PROGMEM = {
  0xff, 0xff, 0xe0, 0x07, 0xcb, 0xf3, 0x8a, 0x01, 
  0xaa, 0xfd, 0xaa, 0x81, 0xa8, 0x3d, 0xad, 0x85, 
  0xa1, 0xb5, 0xbc, 0x15, 0x81, 0x55, 0xbf, 0x55, 
  0x80, 0x51, 0xcf, 0xd3, 0xe0, 0x07, 0xff, 0xff
};

void setup() {
  //Serial is configured at 115200, because of the radio module
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate oled voltage from 3.3V internally
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  //Start the Boot logo
  skyRedLogo();

  //Button
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  //Built-in Led and radio reset setup
  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Serial Monitor for RX");
  
  //Radio manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if(!rf69.init()){
    Serial.println("Módulo RFM69 ha fallado al iniciar");
    while (1);
  }
  Serial.println("Módulo RFM69 iniciado con éxito");
  if(!rf69.setFrequency(RF69_FREQ)){
    Serial.println("Error en la frecuencia");
  }

  //Transmission power (range of 14 - 20 dBm)
  rf69.setTxPower(20, true);

  //Encryption Key (MUST BE THE SAME IN TX - RX)
  uint8_t key[] = { 0x03, 0x04, 0x01, 0x09, 0x01, 0x08, 0x01, 0x03,
                    0x02, 0x03, 0x07, 0x01, 0x01, 0x01, 0x02, 0x07};

  rf69.setEncryptionKey(key);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  // OLED text display tests
  delay(1000);
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  oled.setCursor(0,0);
  oled.println("RFM69 @ ");
  oled.print((int)RF69_FREQ);
  oled.println(" MHz");
  oled.display();
  delay(1000);
}

void skyRedLogo(void){
  oled.clearDisplay();

  oled.drawBitmap(
    (oled.width()  - LOGO_WIDTH ) / 2,
    (oled.height() - LOGO_HEIGHT) / 2,
    SkyRed, LOGO_WIDTH, LOGO_HEIGHT, 1);
  oled.display();
  delay(1000);
}

uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = " OK";

void loop() {
  /*In this loop, we write messages or data via Serial monitor
   * and transmits to the RFM69
   */
   if(rf69.waitAvailableTimeout(100)) {
    //awaiting a message
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (! rf69.recv(buf, &len)) {
      Serial.println("Receive failed");
      return;
    }
    digitalWrite(LED, HIGH);
    rf69.printBuffer("Received: ", buf, len);
    buf[len] = 0;

    Serial.print("Got: "); Serial.println((char*)buf);
    Serial.print("RSSI: "); Serial.println(rf69.lastRssi(), DEC);

    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.println((char*)buf);
    oled.print("RSSI: "); oled.print(rf69.lastRssi());
    oled.display(); 
    digitalWrite(LED, LOW);
   }

    if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B) || !digitalRead(BUTTON_C)){
      Serial.println("Button pressed!");
    
      char radiopacket[20] = "Button #";
      if (!digitalRead(BUTTON_A)) radiopacket[8] = 'A';
      if (!digitalRead(BUTTON_B)) radiopacket[8] = 'B';
      if (!digitalRead(BUTTON_C)) radiopacket[8] = 'C';
      radiopacket[9] = 0;

      Serial.print("Sending "); Serial.println(radiopacket);
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();
    }
}
