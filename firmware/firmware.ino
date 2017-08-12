#include <SPI.h>
#include <Ethernet.h>
#include <FastLED.h>
#include <EEPROM.h>
#include "button.h"

#define LEDS_PER_CHANNEL    105
#define NUM_CHANNELS        8
#define NUM_LEDS            (NUM_CHANNELS * LEDS_PER_CHANNEL)
#define COLOR_ORDER         BGR
#define SPI_RATE            DATA_RATE_MHZ(4)
#define UDP_RX_BUFFER_SIZE  8192
#define UDP_PORT_NUMBER     1337

#define PARALLEL_OUTPUT
#define PLAYA_MODE

//-------------------------------------------------------------------------------------------
// Pin mappings
//-------------------------------------------------------------------------------------------

//#define REVB
#define REVC

#ifdef REVB
uint8_t dip_switch_pins[] = {PA0, PA1, PA2, PA3, PB0};
#define SWITCH_PIN  PB1
#endif 

#ifdef REVC
uint8_t dip_switch_pins[] = {PB9, PB8, PA1, PA0, PA2, PA3, PB0, PB1};
#define SWITCH_PIN  PA15
#endif

#define FPS_PIN         PA0
#define CLOCK_PIN       PB12
#define ETHERNET_CS_PIN PA4
#define INDICATOR_PIN   PC13

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_CHANNELS][LEDS_PER_CHANNEL];

typedef struct _opc_header_t {
  uint8_t channel;
  uint8_t command;
  uint8_t datalen_h;
  uint8_t datalen_l;
  uint8_t data[];
} opc_header_t;

EthernetUDP Udp;
char packet_buffer[UDP_RX_BUFFER_SIZE]; 

FastPin<FPS_PIN> fps;

Button sw(SWITCH_PIN);

uint32_t last_packet_time = 0;
uint8_t brightness = 0;

//-------------------------------------------------------------------------------------------
// IP & Mac address
//-------------------------------------------------------------------------------------------

// this default mac seems to work well as a template, lets just replace a single
// byte based on controller address
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

uint8_t get_address() {
    uint8_t result = 0;
    for (int i = 0; i < ARRAY_SIZE(dip_switch_pins); i++) {
        result = result | !digitalRead(dip_switch_pins[i]) << i;
    }
    return result;
}


//-------------------------------------------------------------------------------------------
// Test patterns
//-------------------------------------------------------------------------------------------

void network() {
  // clear the buffer if we didn't receive a packet for a while
  int32_t delta = millis() - last_packet_time - 10000;
  if (delta > 10000) {
    brightness = 0;
  } else if (delta > 0) {
    brightness = map(delta, 0, 10000, 255, 0);
  } else {
    brightness = qadd8(brightness, 5);
  }

  FastLED.setBrightness(brightness);
}

void rainbow() {
    static uint8_t hue=0;
    fill_rainbow((CRGB*)leds, NUM_LEDS, ++hue, 10);  
}

void fullwhite() {
    fill_solid((CRGB*)leds, NUM_LEDS, CRGB::White);  
}

typedef void (*SimplePatternList[])();
uint8_t g_current_pattern = 0;

SimplePatternList patterns = {
  network,
  rainbow,
  //fullwhite,
};

void render() {
  (patterns)[g_current_pattern]();
  FastLED.delay(1000/120);
}

void next_pattern()
{
  g_current_pattern = (g_current_pattern + 1) % ARRAY_SIZE( patterns);
  fill_solid((CRGB*)leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(255);
}


//-------------------------------------------------------------------------------------------
// Network code
//-------------------------------------------------------------------------------------------

bool got_a_packet = false;

void network_connect() {
  Ethernet.init(ETHERNET_CS_PIN);
  Ethernet.softReset();

  // attempt to read stored address first
  uint8_t addr;
  addr = EEPROM.read(0);
  if (255 == addr) {
      addr = get_address();
  }
  
  mac[2] = addr;
  IPAddress ip_addr(192, 168, 1, addr);
  Ethernet.begin(mac, ip_addr);
  Udp.begin(UDP_PORT_NUMBER);
}

void network_poll() {
  int packet_size = Udp.parsePacket();
  static int remaining = 0;
  if (packet_size > 0) {
    got_a_packet = true;
    last_packet_time = millis();
    FastLED.setBrightness(255);

    // force us into network mode if we receive a packet
    g_current_pattern = 0;
    
    
    remaining += Udp.read(packet_buffer+remaining,UDP_RX_BUFFER_SIZE);
    opc_header_t * header = (opc_header_t *)packet_buffer;
    while (remaining > 4) {
      fps.toggle();
      uint16_t data_size = header->datalen_h << 8 | header->datalen_l;
      uint16_t offset = (int)header->channel * LEDS_PER_CHANNEL * 3;
      memcpy8((uint8_t *) leds + offset, header->data, min(data_size, remaining-4));
      remaining -= (data_size + 4);
      header = (opc_header_t *) (packet_buffer+data_size+4);

      // intermediate render if we have multiple frames in this packet
      if (remaining > 4) {
        render();
      }
    }
  }
}

//-------------------------------------------------------------------------------------------
// Button handlers
//-------------------------------------------------------------------------------------------

void button_press() {
  // Only switch modes on playa if we don't have network activity
  #ifdef PLAYA_MODE
  if (!got_a_packet)
  #endif
    next_pattern();
}

void button_hold() {
  EEPROM.write(0, get_address());  
  network_connect();

  digitalWrite(INDICATOR_PIN, LOW);
  delay(250);
  digitalWrite(INDICATOR_PIN, HIGH);
  delay(250);
  digitalWrite(INDICATOR_PIN, LOW);
  delay(250);
  digitalWrite(INDICATOR_PIN, HIGH);
}

void setup() {
  #ifdef REVB
    #ifdef PARALLEL_OUTPUT
      FastLED.addLeds<TENERE_REVB, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds, LEDS_PER_CHANNEL);
    #else
      FastLED.addLeds<APA102, PB13, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[0],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB14, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[1],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB15, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[2],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB5, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[3],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB6, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[4],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB7, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[5],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB8, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[6],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB9, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[7],LEDS_PER_CHANNEL);
    #endif
  #endif

  #ifdef REVC
    #ifdef PARALLEL_OUTPUT
      FastLED.addLeds<TENERE_REVC, CLOCK_PIN, COLOR_ORDER, SPI_RATE>((CRGB *)leds, LEDS_PER_CHANNEL);
    #else
      FastLED.addLeds<APA102, PB7, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[0],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB6, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[1],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB5, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[2],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB4, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[3],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB3, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[4],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB15, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[5],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB14, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[6],LEDS_PER_CHANNEL);
      FastLED.addLeds<APA102, PB13, CLOCK_PIN, COLOR_ORDER, SPI_RATE>(leds[7],LEDS_PER_CHANNEL);
    #endif
  #endif
  
  FastLED.setBrightness(255);
  SPI.begin();

  // debug pin
  fps.setOutput();

  // ip address dip switch
   for (int i = 0; i < ARRAY_SIZE(dip_switch_pins); i++) {
       pinMode(dip_switch_pins[i], INPUT_PULLUP);
   }

   // indicator LED
   pinMode(INDICATOR_PIN, OUTPUT);
   digitalWrite(INDICATOR_PIN, HIGH);

   // wait a bit before connecting to ensure the switches are powered on
   delay(3000);
   network_connect();
}


void loop() {
  sw.poll(button_press, button_hold);

  network_poll();
  
  render();  
}
