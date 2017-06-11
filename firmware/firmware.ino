//#define WIZ550io_WITH_MACADDRESS
#include <SPI.h>
#include <Ethernet.h>
//#include <EthernetUdp3.h>
#include <FastLED.h>

#define NUM_LEDS    105
#define COLOR_ORDER BGR
#define SPI_RATE    DATA_RATE_MHZ(5)

#define UDP_RX_BUFFER_SIZE  4096
#define UDP_PORT_NUMBER 1337

CRGB leds[NUM_LEDS];

typedef struct _opc_header_t {
  uint8_t channel;
  uint8_t command;
  uint8_t datalen_h;
  uint8_t datalen_l;
  uint8_t data[];
} opc_header_t;

IPAddress ip(192, 168, 1, 79);
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
EthernetUDP Udp;
char packet_buffer[UDP_RX_BUFFER_SIZE]; 

FastPin<PA0> fps;

void setup() {
  FastLED.addLeds<APA102,PB13,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  FastLED.addLeds<APA102,PB14,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  FastLED.addLeds<APA102,PB15,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
// Note - these 2 pins are the arm SWD pins. Replaced with PB8 and PB9
//  FastLED.addLeds<APA102,PB3,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
//  FastLED.addLeds<APA102,PB4,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);  

  FastLED.addLeds<APA102,PB5,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  FastLED.addLeds<APA102,PB6,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  FastLED.addLeds<APA102,PB7,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  
  FastLED.addLeds<APA102,PB8,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);
  FastLED.addLeds<APA102,PB9,PB12, COLOR_ORDER, SPI_RATE>(leds,NUM_LEDS);

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.setBrightness(255);
  SPI.begin();

  fps.setOutput();
}

void render() {
  static uint8_t hue=0;
  FastLED.show();
}

bool network_connected = false;

void network_connect() {
  Ethernet.init(PA4);
  Ethernet.softReset();
  Ethernet.begin(mac, ip);
  Udp.begin(UDP_PORT_NUMBER);
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  network_connected = true;
}

void network_poll() {
  int packet_size = Udp.parsePacket();

  if(packet_size)
  {
    Udp.read(packet_buffer,UDP_RX_BUFFER_SIZE);
    opc_header_t * header = (opc_header_t *)packet_buffer;
    uint16_t data_size = header->datalen_h << 8 | header->datalen_l;
    memcpy8(leds, header->data, min(data_size, sizeof(leds)));
  }
}

void loop() {
  render();

  if (!network_connected) {
    network_connect();
  } else {
    network_poll();  
  }
  fps.toggle();
}
