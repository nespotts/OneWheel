#include "beeper.cpp"

#define BEEPER_PIN 5

#define PLAY_STARTUP true
#define DUTY_CYCLE_ALERT 0.70 // 0 to disable
#define SWITCH_ERPM 1000 // 0 to disable
#define LOW_VOLTAGE 40 // 0 to disable
#define LOW_VOLTAGE_INTERVAL 30 * 1000 // every 30 seconds

class BalanceBeeper {
  private:
    Beeper beeper;

    bool switchStateLatch = false;
    long lastLowVoltageMillis = 0;
  public:
    BalanceBeeper() :
      beeper(BEEPER_PIN){
    }
    void setup(){ 
      beeper.setup();
      if(PLAY_STARTUP){
        beeper.queueThreeShort();
      }
    }

    void loop(double dutyCycle, double erpm, uint16_t switchState, double voltage, bool deadmanSwitchChange, bool armlockalarm, int mode_change){
      beeper.loop();

      // Non latching beeps for Duty Cycle
      if(dutyCycle > DUTY_CYCLE_ALERT && DUTY_CYCLE_ALERT > 0){
        beeper.queueShortSingle();
      }

      // Latching beep for HALF footpad state
      if(switchState == 1 && erpm > SWITCH_ERPM && SWITCH_ERPM > 0 && switchStateLatch == false){
        switchStateLatch = true;
        beeper.queueLongSingle();
      }else if(switchState != 1){
        switchStateLatch = false;
      }

      // Low voltage, time based repeat
      if(voltage < LOW_VOLTAGE && LOW_VOLTAGE > 0 && lastLowVoltageMillis + LOW_VOLTAGE_INTERVAL < (long)millis()){
        beeper.queueSad();
        lastLowVoltageMillis = millis();
      }

      // beep on deadman switch position change
      if (deadmanSwitchChange) {
        beeper.queueShortSingle();
      }

      // beep when the board is arm locked
      if (armlockalarm) {
        beeper.queueShortSingle();
      }

      if (mode_change == 1 || mode_change == 2) {
        beeper.queueLongSingle();
      }
    }

};
