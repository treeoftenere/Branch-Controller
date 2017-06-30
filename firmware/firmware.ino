//#define WIZ550io_WITH_MACADDRESS
#include <SPI.h>
#include <Ethernet.h>
//#include <EthernetUdp3.h>
#include <FastLED.h>

#define LEDS_PER_CHANNEL  105
#define NUM_CHANNELS 8
#define NUM_LEDS    (NUM_CHANNELS * LEDS_PER_CHANNEL)
#define COLOR_ORDER BGR
#define SPI_RATE    DATA_RATE_MHZ(5)

#define UDP_RX_BUFFER_SIZE  4096
#define UDP_PORT_NUMBER 1337

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_CHANNELS][LEDS_PER_CHANNEL];

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

uint8_t dip_switch_pins[] = {PA0, PA1, PA2, PA3, PB0};
#define SWITCH_PIN  PB1

uint8_t get_address() {
    uint8_t result = 0;
    for (int i = 0; i < ARRAY_SIZE(dip_switch_pins); i++) {
        result = result | !digitalRead(dip_switch_pins[i]) << i;
    }
    return result;
}

void setup() {

  // diable jtag so we get 2 extra pins
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);

  
  FastLED.addLeds<APA102,PB13,PB12, COLOR_ORDER, SPI_RATE>(leds[0],LEDS_PER_CHANNEL);
  FastLED.addLeds<APA102,PB14,PB12, COLOR_ORDER, SPI_RATE>(leds[1],LEDS_PER_CHANNEL);
  FastLED.addLeds<APA102,PB15,PB12, COLOR_ORDER, SPI_RATE>(leds[2],LEDS_PER_CHANNEL);


  FastLED.addLeds<APA102,PB5,PB12, COLOR_ORDER, SPI_RATE>(leds[3],LEDS_PER_CHANNEL);
  FastLED.addLeds<APA102,PB6,PB12, COLOR_ORDER, SPI_RATE>(leds[4],LEDS_PER_CHANNEL);
  FastLED.addLeds<APA102,PB7,PB12, COLOR_ORDER, SPI_RATE>(leds[5],LEDS_PER_CHANNEL);
  
//  FastLED.addLeds<APA102,PB3,PB12, COLOR_ORDER, SPI_RATE>(leds[6],LEDS_PER_CHANNEL);
//  FastLED.addLeds<APA102,PB4,PB12, COLOR_ORDER, SPI_RATE>(leds[7],LEDS_PER_CHANNEL);  
  FastLED.addLeds<APA102,PB8,PB12, COLOR_ORDER, SPI_RATE>(leds[6],LEDS_PER_CHANNEL);
  FastLED.addLeds<APA102,PB9,PB12, COLOR_ORDER, SPI_RATE>(leds[7],LEDS_PER_CHANNEL);

  fill_solid((CRGB*)leds, NUM_LEDS, CRGB::Red);
  FastLED.setBrightness(255);
  SPI.begin();

  fps.setOutput();
  // address dip switch
   for (int i = 0; i < ARRAY_SIZE(dip_switch_pins); i++) {
       pinMode(dip_switch_pins[i], INPUT_PULLUP);
   }
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(PC13, OUTPUT);
}

void render() {
  static uint8_t hue=0;
  fill_rainbow((CRGB*)leds, NUM_LEDS, ++hue, 10);
  FastLED.delay(1000/120);
}

bool network_connected = false;

void network_connect() {
  Ethernet.init(PA4);
  Ethernet.softReset();
  Ethernet.begin(mac, ip);
  Udp.begin(UDP_PORT_NUMBER);
  fill_solid((CRGB*)leds, NUM_LEDS, CRGB::Green);
  network_connected = true;
}

void network_poll() {
  int packet_size = Udp.parsePacket();

  if(packet_size)
  {
    Udp.read(packet_buffer,UDP_RX_BUFFER_SIZE);
    opc_header_t * header = (opc_header_t *)packet_buffer;
    uint16_t data_size = header->datalen_h << 8 | header->datalen_l;
    uint16_t offset = header->channel * LEDS_PER_CHANNEL * 3;
    memcpy8(leds + offset, header->data, min(data_size, sizeof(leds) - offset));
  }
}

void loop() {
  uint8_t address = get_address();
  render();

  if (!network_connected) {
    network_connect();
    //digitalWrite(PC13, HIGH);
  } else {
    network_poll();
    //digitalWrite(PC13, LOW);
  }
  digitalWrite(PC13, digitalRead(SWITCH_PIN));
  fps.toggle();
}
