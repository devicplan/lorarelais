/*******************************************************************************
 * Jens Dietrich - www.icplan.de - Hoyerswerda - 17.04.2023 LoRaWan Relais
 * my customization for lower power consumption
 *   ADC while standby off -> reduces current by 72µA
 *   BOD off               -> reduces current by 20µA
 * 
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman 
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 * 
 * https://www.arduinoforum.de/arduino-Thread-Messen-der-eigenen-Betriebsspannung-mit-dem-Arduino
  *******************************************************************************/

#include <lmic.h>                                                                         // MCCI LoRaWAN LMIC library V 4.1.1 (11/2022)
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#define debug                                                                             // comment out for debug output

char LED = 4;                                                                             // led red
char PWM = 6;                                                                             // voltage doubler
char R_ON = 7;                                                                            // relais pin 11
char R_OFF = 8;                                                                           // relais pin 12

uint8_t rec_b = 0;                                                                        // received bytes
uint8_t rec_t = 1;                                                                        // send intervall in minutes
//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// This EUI must be in little-endian format, so least-significant-byte
static const u1_t PROGMEM APPEUI[8]={0x01, 0x20, 0x0A0, 0x04, 0x11, 0x01, 0x89, 0xA0};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={0xB2, 0x73, 0x25, 0xD0, 0x8E, 0xD5, 0xC3, 0x70};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
static const u1_t PROGMEM APPKEY[16] = {0xE8, 0x5A, 0xA5, 0x8F, 0xA7, 0x18, 0x98, 0xC6, 0xF6, 0xB2, 0xBA, 0x3B, 0x10, 0xC1, 0xBE, 0xCF};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//static uint8_t mydata[] = "Hello, world!";
//static uint8_t mydata[] = "000000";
static uint8_t mydata[] = {0,0,0,0,0,0};
static osjob_t sendjob;
uint16_t TX_INTERVAL = 60;                                                                // send interval in sec, in between everything sleeps (5 minutes)

const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 5, LMIC_UNUSED_PIN},
};

void relais_on(void)  {
  uint16_t a;
  for (a=0;a<100;a++)  {
    digitalWrite(PWM, HIGH);
    delay(5);        
    digitalWrite(PWM, LOW);
    delay(5);
  }
  digitalWrite(R_ON, HIGH); 
  delay(500); 
  digitalWrite(R_ON, LOW);
  Serial.println("Relais on");
  mydata[0] = 0x01;
}

void relais_off(void)  {
  uint16_t a;
  for (a=0;a<100;a++)  {
    digitalWrite(PWM, HIGH);
    delay(5);        
    digitalWrite(PWM, LOW);
    delay(5);
  }
  digitalWrite(R_OFF, HIGH); 
  delay(500); 
  digitalWrite(R_OFF, LOW);  
  Serial.println("Relais off"); 
  mydata[0] = 0x00;
}

float messen() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);                                 // preparation to measure internal 1.1 volts
  delay(10);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  uint8_t low  = ADCL;  
  uint8_t high = ADCH;
  long result = (high<<8) | low;
  float vcc = (1068 * 1023L / result) + 0;                                                // 0 or 534 is blocking voltage diode batt->IC
  analogReference(DEFAULT);                                                               // reset to Vcc as reference
  delay(10);
  return vcc;
}

#ifdef debug
void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}
#endif

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("F1"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("F2"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("F3"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("F4"));
            break;
        case EV_JOINING:
            Serial.println(F("J-ING"));
            break;
        case EV_JOINED:
            Serial.println(F("J-NED"));
#ifdef debug
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print(F("netid: "));
              Serial.println(netid, DEC);
              Serial.print(F("devaddr: "));
              Serial.println(devaddr, HEX);
              Serial.print(F("AppSKey: "));
              digitalWrite(LED, HIGH);                                                    // join ok - led off              
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print(F("NwkSKey: "));
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
#endif
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("F5"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("F6"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("F7"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("R ack"));
            if (LMIC.dataLen) {                                                           // rx data evaluation
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              rec_b = LMIC.dataLen;
              Serial.print(F(" bytes of payload: 0x"));                                   // display rx data
              for (int i = 0; i < LMIC.dataLen; i++) {
                if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                  Serial.print(F("0"));
                }
                Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
              }
              Serial.println();

              if (rec_b == 1)  {                                                          // read one byte
                if (LMIC.frame[LMIC.dataBeg + 0] != 0)  {                                 // read new relais state
                  relais_on();
                }
                else  {
                  relais_off();
                }
              }

              if (rec_b == 2)  {                                                          // read two byte
                rec_t = LMIC.frame[LMIC.dataBeg + 0];                                     // read new send timeer
                if (rec_t < 1)  rec_t = 1;                                                // min one minutes
                if (rec_t > 180) rec_t = 180;                                             // max three hours
                if (LMIC.frame[LMIC.dataBeg + 1] != 0)  {                                 // read new relais state
                  relais_on();
                }
                else  {
                  relais_off();
                }
              }
            }

            digitalWrite(LED, LOW);                                                       // short led pulse 
            delay(40);
            digitalWrite(LED, HIGH);             
            uint8_t adcbackup = ADCSRA;                                                   // push adc parameter
            ADCSRA = 0;                                                                   // adc switch off before standby

            delay(500);
            for(uint16_t t=0;t<((TX_INTERVAL * rec_t)/8);t++)  {                          // interval see above
              startSleeping();
            }
            ADCSRA = adcbackup;                                                           // pop adc parameter
            delay(500);
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(1), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("F8"));
            break;
        case EV_RESET:
            Serial.println(F("F9"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("F10"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("F11"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("F12"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("F13"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("F14"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("F15"));
            break;

        default:
            Serial.print(F("F16: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
    float sp;
    uint8_t a = 0;
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("F17"));
    } else {
        mydata[1] = rec_t;
        sp = messen();
        Serial.println(sp); 
        a = 0;
        while (sp > 999)  {
          sp = sp - 1000;
          a = a + 1;
        }
        mydata[2] = a;
        a = 0;
        while (sp > 99)  {
          sp = sp - 100;
          a = a + 1;
        }
        mydata[3] = a;
        a = 0;
        while (sp > 9)  {
          sp = sp - 10;
          a = a + 1;
        }
        mydata[4] = a;
        mydata[5] = sp;
        LMIC_setTxData2(1, mydata, 6, 0);        

        Serial.println(F("P queued"));
    }
}

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(PWM, OUTPUT);
    pinMode(R_ON, OUTPUT);
    pinMode(R_OFF, OUTPUT);
    digitalWrite(LED, LOW); 
    
    Serial.begin(9600);
    delay(2000);
    Serial.println(F("Start"));
    os_init();
    LMIC_reset();
    do_send(&sendjob);
    relais_off();
    mydata[0] = 0x00;
}

void loop() {
    os_runloop_once();
}

ISR (WDT_vect) {
   wdt_disable(); 
}

void startSleeping() {
    // clear various "reset" flags
    MCUSR = 0;                                                  // allow changes, disable reset, enable Watchdog interrupt
    WDTCSR = bit (WDCE) | bit (WDE);                            // set interval (see datasheet p55)
    WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);              // set WDIE, and 8 seconds delay
    //WDTCSR = bit (WDIE) | bit (WDP2) | bit (WDP1);            // same with 1 second
    wdt_reset();                                                // start watchdog timer
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);                       // prepare for powerdown  
    sleep_enable();  
    sleep_cpu ();                                               // power down !
}
