#define FASTLED_INTERNAL
#include "./src/FastLED/src/FastLED.h"

#define LED_TYPE WS2811
#define COLOR_ORDER BRG
#define BRIGHTNESS 255

#define LED_PIN_FOREWARD 5
#define LED_PIN_BACKWARD 6
#define NUM_LEDS_FORWARD 2
#define NUM_LEDS_BACKWARD 2
#define BRIGHTNESS_FORWARD 64  // NOT USED
#define BRIGHTNESS_BACKWARD 64 // NOT USED
#define FADE_STEP 6

#define FRONT_COLOR CRGB::MintCream  //Turquoise
#define BACK_COLOR CRGB::Red


class BalanceLEDs {
  private:
    bool directionIsForward;
  
    CRGB forward[NUM_LEDS_FORWARD];
    CRGB backward[NUM_LEDS_BACKWARD];

    // Helper function that blends one uint8_t toward another by a given amount
    void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount){
      if( cur == target) return;
      
      if( cur < target ) {
        uint8_t delta = target - cur;
        delta = scale8_video( delta, amount);
        cur += delta;
      } else {
        uint8_t delta = cur - target;
        delta = scale8_video( delta, amount);
        cur -= delta;
      }
    }

    // Blend one CRGB color toward another CRGB color by a given amount.
    // Blending is linear, and done in the RGB color space.
    // This function modifies 'cur' in place.
    CRGB fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount){
      nblendU8TowardU8( cur.red,   target.red,   amount);
      nblendU8TowardU8( cur.green, target.green, amount);
      nblendU8TowardU8( cur.blue,  target.blue,  amount);
      return cur;
    }

    // Fade an entire array of CRGBs toward a given background color by a given amount
    // This function modifies the pixel array in place.
    void fadeTowardColor( CRGB* L, uint16_t N, const CRGB& bgColor, uint8_t fadeAmount){
      for( uint16_t i = 0; i < N; i++) {
        fadeTowardColor( L[i], bgColor, fadeAmount);
      }
    }
  public:
    
    void setup(){
      FastLED.addLeds<LED_TYPE, LED_PIN_FOREWARD, COLOR_ORDER>(forward, NUM_LEDS_FORWARD).setCorrection( TypicalLEDStrip );
      FastLED.addLeds<LED_TYPE, LED_PIN_BACKWARD, COLOR_ORDER>(backward, NUM_LEDS_BACKWARD).setCorrection( TypicalLEDStrip );
      FastLED.setBrightness(BRIGHTNESS);

      // Default to forward
      directionIsForward = true;
      fadeTowardColor(forward, NUM_LEDS_FORWARD, FRONT_COLOR, FADE_STEP);
      fadeTowardColor(backward, NUM_LEDS_BACKWARD, BACK_COLOR, FADE_STEP); 
      FastLED.show();
    }

    void loop(double erpm, int led_brightness){
      // Latching behavior, if you know, you know.
      if(erpm > 10){
        directionIsForward = true;
      }else if(erpm < -10){
        directionIsForward = false;
      }
      if (led_brightness < 20) {
        FastLED.setBrightness(0);
      } else if (led_brightness > 205) {
        FastLED.setBrightness(255);
      } else {
        FastLED.setBrightness(led_brightness);
      }


      if(directionIsForward){
        fadeTowardColor(forward, NUM_LEDS_FORWARD, FRONT_COLOR, FADE_STEP);
        fadeTowardColor(backward, NUM_LEDS_BACKWARD, BACK_COLOR, FADE_STEP);
      }else{
        fadeTowardColor(forward, NUM_LEDS_FORWARD, BACK_COLOR, FADE_STEP);
        fadeTowardColor(backward, NUM_LEDS_BACKWARD, FRONT_COLOR, FADE_STEP);
      }
        FastLED.show();
    }
};
