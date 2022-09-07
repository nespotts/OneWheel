
#include "src/SSD1306Ascii/src/SSD1306Ascii.h"
#include "src/SSD1306Ascii/src/SSD1306AsciiWire.h"

class BalanceDisplay {
  private:
    SSD1306AsciiWire oled;
  public:
    float min_voltage = 60;
    float max_temp = 0;
    float max_duty = 0;
    float max_velocity = 0;
    float max_amps = 0;

    float wheel_diameter = 270;  //mm
    float velocity = 0; //mph
    float num_poles = 30;
    
    int extra_switch_val;
    int extra_switch_state;
    bool mode_change_flag;
    long mode_change_timer = 0;
    long mode_change_interval = 3000;
    int mode_change = 0; // 0 - no change, 1 - change to remote, 2 - change to footpad

    void setup(){
      Wire.begin();
      Wire.setClock(400000L);
      
      oled.begin(&Adafruit128x64, 0x3c);
      Reset_Max();
    }

    void Calc_Speed() {
      velocity = rdf.erpm/(num_poles/2.0) * 3.141592654 * wheel_diameter/(25.4*12.0*5280.0) * 60.0;
    }

    void Reset_Max() {
      min_voltage = 60;
      max_temp = 0;
      max_duty = 0;
      max_velocity = 0;
      max_amps = 0;      
    }

    void Calc_Max() {
      if (rdf.voltage < min_voltage && rdf.voltage != 0) {
        min_voltage = rdf.voltage;
      }
      if (rdf.tempMosfet > max_temp) {
        max_temp = rdf.tempMosfet;
      }
      if (fabs(rdf.dutyCycle) > fabs(max_duty)) {
        max_duty = rdf.dutyCycle;
      }
      if (fabs(velocity) > fabs(max_velocity)) {
        max_velocity = velocity;
      }
      if (fabs(rdf.motorCurrent) > fabs(max_amps)) {
        max_amps = rdf.motorCurrent;
      }
    }

    void loop(double tempMosfet, double dutyCycle, double motorCurrent, double voltage, uint16_t balanceState, uint16_t switchState, double adc1, double adc2){
      Calc_Speed();
      Calc_Max();

      extra_switch_val = digitalRead(extra_sw_pin);

      if (extra_switch_val == HIGH && extra_switch_state == LOW) {
        Reset_Max();
        extra_switch_state = HIGH;
        mode_change_flag = true;
        mode_change_timer = currenttime;
      }
      else if (extra_switch_val == HIGH && extra_switch_state == HIGH) {
        Reset_Max();
      }
      else if (extra_switch_val == LOW && extra_switch_state == HIGH) {
        extra_switch_state = LOW;
        mode_change_flag = false;
      }
      else if (extra_switch_val == LOW && extra_switch_state == LOW) {
        // do nothing
        mode_change_flag = false;
      }

      if (currenttime - mode_change_timer >= mode_change_interval && mode_change_flag) {
        if (send_data.mode == 1) {
          send_data.mode = 0;
          mode_change = 1;
        } else {
          send_data.mode = 1;
          mode_change = 2;
        }
        mode_change_flag = false;
      } else {
        mode_change = 0;
      }

      if (digitalRead(light_sw_pin) == HIGH) {
        tempMosfet = max_temp;
        dutyCycle = max_duty;
        motorCurrent = max_amps;
        voltage = min_voltage;
        velocity = max_velocity;
      }


        oled.home();

        // Line 1: Voltage and Temp
        oled.setFont(ZevvPeep8x16);
        oled.set1X();
        oled.print(voltage,1);
        oled.print("V");
        oled.print(" ");
        oled.print(tempMosfet,1);
        oled.print((char)247);
        oled.print("C ");
        oled.print(dutyCycle*100.0,0);
        oled.print("%");
        oled.println("                                    ");
        
        // Line 2: ADC Switches
        oled.setFont(Adafruit5x7);
        oled.set1X();
        oled.print("ADC1: ");
        oled.print(adc1);
        oled.set2X();
        if(switchState == 0){
          oled.print(" OFF ");
        }else if(switchState == 1){
          oled.print(" HALF");
        }else{
          oled.print(" ON  ");
        }
        oled.set1X();
        oled.println();
        oled.print("ADC2: ");
        oled.println(adc2);

        // Line 3: Balance state
        oled.setFont(TimesNewRoman13);
        if(balanceState == 0){
          oled.print("Calibrating               ");
        }else if(balanceState == 1){
          oled.print("Running                   ");
        }else if(balanceState == 2){
          oled.print("Run: Tiltback Duty        ");
        }else if(balanceState == 3){
          oled.print("Run: Tiltback HV          ");
        }else if(balanceState == 4){
          oled.print("Run: Tiltback LV          ");
        }else if(balanceState == 5){
          oled.print("Run: Tilt Constant        ");
        }else if(balanceState == 6){
          oled.print("Fault: Pitch Angle        ");
        }else if(balanceState == 7){
          oled.print("Fault: Roll Angle         ");
        }else if(balanceState == 8){
          oled.print("Fault: Switch Half        ");
        }else if(balanceState == 9){
          oled.print("Fault: Switch Full       ");
        }else if(balanceState == 10){
          oled.print("Fault: Duty               ");
        }else if(balanceState == 11){
          oled.print("Initial                  ");
        }else{
          oled.print("Unknown              ");
        } 

        oled.setCol(120);
        if (send_data.mode == 0) {
          oled.println("R    ");
        } else {
          oled.println("F    ");
        }

        // Line 4: Duty Cycle (up to 64 verical bars)
        oled.setFont(TimesNewRoman16_bold);
        // int column = 0;
        // for(float i = 0; i < fabsf(dutyCycle); i += 0.024){
        //   oled.print("|");
        //   column++;
        // }
        oled.print(velocity);
        oled.print(" mph            ");

        // oled.print("                              ");
        // Negative Currents
        if (motorCurrent < 0 && motorCurrent > -10.0) {
          oled.setCol(83);        
        } else if (motorCurrent < 0 && motorCurrent <= -10.0) {
          oled.setCol(78);
        // Positive Currents
        } else if (motorCurrent >= 0 && motorCurrent < 10.0) {
          oled.setCol(90);
        } else if (motorCurrent >= 0 && motorCurrent >= 10.0) {
          oled.setCol(83);
        } 
        oled.print(motorCurrent,2);
        oled.print("A     ");
    }
};
