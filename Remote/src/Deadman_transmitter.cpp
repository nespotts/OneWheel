#include "Arduino.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>

RF24 radio(8,10); // CE, CSN  new CB = (8,10),  old C = (9,10)
const byte address[][6] = {"90001", "90002"};  // 90001 - send to Onewheel, 90002 - receive from Onewheel

// Display variables

long display_timer = 0;
long display_interval = 250;

// Deadman switch variables
int switch_pin = 6; // 6 new CB, 2 old
int switch_val = 0;
int last_switch_val = 0;
int switch_val_index = 0;
int num_vals = 2;  // 3
int switch_vals[10];
int switch_sum=0;
int deadman_switch = 0;
int last_deadman = 0;
bool deadman_switch_change = false;

int led_brightness_pin = A1;
int armed_led_pin = 3;
int light_sw_pin = 4;
int extra_sw_pin = 7;

long currenttime = 0;
long lasttime = 0;
long request_interval = 10;  // 800 ms
long request_timer = 0;

#include "transmitter_functions.h"

#include "balance_display.h"
#include "balance_beeper.h"
BalanceDisplay balanceDisplay;
BalanceBeeper balanceBeeper;

void setup() {
  // put your setup code here, to run once:
  pinMode(switch_pin, INPUT);
  pinMode(light_sw_pin, INPUT);
  pinMode(extra_sw_pin, INPUT);
  pinMode(A1, INPUT);
  pinMode(A0, INPUT);
  pinMode(armed_led_pin, OUTPUT);

  Serial.begin(115200);

  Radio_Setup(4);

  balanceDisplay.setup();
  balanceBeeper.setup();
}


void loop() {
  currenttime = millis();
  Deadman_Filter();

  send_data.led_brightness = analogRead(led_brightness_pin);

  if((currenttime - request_timer) >= request_interval) {
    send_data.request_data = true;
  } else {
    send_data.request_data = false;
  }

  SendData();
  ReceiveData();

  if((currenttime - display_timer) >= display_interval) {
    balanceDisplay.loop(rdf.tempMosfet, rdf.dutyCycle, rdf.motorCurrent, rdf.voltage, rdf.balanceState, rdf.switchState, rdf.adc1, rdf.adc2);
    display_timer = currenttime;
  }

  // balanceDisplay.loop(rdf.tempMosfet, rdf.dutyCycle, rdf.motorCurrent, rdf.voltage, rdf.balanceState, rdf.switchState, rdf.adc1, rdf.adc2);
  balanceBeeper.loop(rdf.dutyCycle, rdf.erpm, rdf.switchState, rdf.voltage, deadman_switch_change, false, balanceDisplay.mode_change);

  // if(deadman_switch_change) {
  //   balanceBeeper.loop(0.9, 500, 1, 50);
  // } else {
  //   // balanceBeeper.loop(0.2, 500, 1, 50);   
  //   balanceBeeper.loop(rdf.dutyCycle, rdf.erpm, rdf.switchState, rdf.voltage);
  // }

  if (rd.switchState == 2) {
    digitalWrite(armed_led_pin, HIGH);
  } else {
    digitalWrite(armed_led_pin, LOW);
  }

  // Calculate and display loop frequency
  // Serial.println((double)1000.0/((double)currenttime - (double)lasttime));
  lasttime = currenttime;
}