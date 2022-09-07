#include "Arduino.h"
#include "balance_display.cpp"
#include "balance_beeper.cpp"
#include "balance_leds.cpp"

ESC esc;
BalanceDisplay balanceDisplay;
BalanceBeeper balanceBeeper;
BalanceLEDs balanceLEDs;

#include "SPI.h"
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(7,10); // CE, CSN
const byte address[][6] = {"90001", "90002"};

#include "balance_NRF.h"
BalanceNRF balanceNRF;

void setup() {
  Serial.begin(115200);

  esc.setup();
  // balanceDisplay.setup();
  balanceBeeper.setup();
  balanceLEDs.setup();
  balanceNRF.setup();

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  // pinMode(A3, INPUT);
  pinMode(A2, OUTPUT);

}

void loop() {
  esc.loop();
  // balanceDisplay.loop(esc.tempMosfet, esc.dutyCycle, esc.voltage, esc.balanceState, esc.switchState, esc.adc1, esc.adc2);
  balanceBeeper.loop(esc.dutyCycle, esc.erpm, esc.switchState, esc.voltage, balanceNRF.deadman_switch_change, balanceNRF.arm_alarm);

  balanceLEDs.loop(esc.erpm, map(balanceNRF.receive_data.led_brightness,0,1023,10,255));
  balanceNRF.loop();
    // No delay? #YOLO
  if (esc.tempMosfet >= 40) {
    digitalWrite(A2, HIGH);
  } else if (esc.tempMosfet <= 39.5) {
    digitalWrite(A2, LOW);
  }
}