/*
Combination of Arduino Mini Pro ; and pushbutton (NightSwitch) and 2 x PIR sensor (PIR) to control LED ceiling light in WC and bathroom
When entering, PIR causes the LEDS to PWM to full for a defined period (DelayTime * Factor)
If NightSwitch pushbutton is pressed, the LEDs go down to a very low PWM level (useful at night time)
After a set time (DelayTime * Factor) LEDs PWM down to OFF state again
There is a test-jumper (digital pin 05) that determines the length of the DelayTime that LEDs are on (shorted >= 10 sec ; open >= 18 times longer)

The LED on digital pin 13 is used as indicator for the different modes, variable 'ínterval' contains the (ms value)
rapid flash (50 ms) - NightSwitch is being pressed
flashing extremely (10 ms) - panel LEDs are on after NightSwitch being pressed | indicatorLED digitalpin 13 is constant on at 50%, due to very high flasing rate
flashing at (200 ms) - Test mode active, no panel LEDs are on
flashing at (2000 ms) - panel LEDS are on due to PIR activity
no flashing (20000 ms), normal operation

LED-ceiling panels are powered by +58V - 13x white LED, 2x green LED, collector- basis - emmitter - white LED - 47 ohm resistor - GND
NPN - basis is connected with 1Kohm resistor to Arduino WCpin or BKpin, so transistor on the panel is current source for a set of 16 LEDs  
*/
 
int PIRwc = 0;                            // PIR connected to ground/5V and this analog pin - WC
int PIRbk = 2;                            // PIR connected to ground/5V and this analog pin - badkamer
int WCpin = 9;                            // LED-ceiling panels in WC connected to this digital PWM pin
int BKpin = 6;                            // LED-ceiling panels in Badkamer connected to this digital PWM pin
int NightSwitch = 12;                     // switch connected to this digital pin
int TestIn = 4;                           // Jumper on digital pin 5 to +5V (closed is active Test mode)
int TestOut = 13;                         // Output to digital pin 13 (on board LED) to indicate active Test mode
int ledState = LOW;                       // ledState used to set the LED on digital pin13

long DelayTime;                           // basic delay period = 1 minute (60000 milliseconds), but during testing it is 5 seconds (5000 miliseconds)
int WCDelayFactor = 2;                    // Factor to multiply basic delay period ( 1 minute, 60000) of panel LEDS = on
int BKDelayFactor = 4;                    // Period in milliseconds that panel LEDs will be switched on by BK-PIR

int WCleds = 0;                           // LED state: 0=OFF ; 2 = minimal PWM for night-time ; 255 = maximal on
int BKleds = 0;                           // LED state: 0=OFF ; 2 = minimal PWM ; 255 = maximal on
unsigned long TimerWC;  
unsigned long TimerBK;
unsigned long currentMillis;              // used as Timer for the Test-Led digital pin 13
unsigned long previousMillis = 0;         // will store last time the Test-Led digital pin 13 was updated
unsigned long interval = 20000;           // start with digitalpin 13 LED indicator OFF

void setup()  { 
pinMode(NightSwitch, INPUT);              // set pin to input (=default) - Night switch
pinMode(TestIn, INPUT);                   // Jumper on this digital pin to +5V (closed is active for Test mode)
pinMode(TestOut, OUTPUT);                 // Output to this digital pin to indicate that Test mode is active
pinMode(WCpin, OUTPUT);                   // set pin to output - WC LED panels
pinMode(BKpin, OUTPUT);                   // set pin to output - Badkamer LED panels
} 

void loop()  { 

  IndicatorLED();                                      // see above for the indications the LED of digital pin 13 will show, based on the value of the variable 'interval'
  
  if (digitalRead(TestIn) == HIGH) {                   // First: check whether we are in test-mode (jumper on digital pin 4 closed)
       DelayTime = 5000;                               // main function of test mode is to have short panel LED periods, easier for checking.
       if (interval > 3000) interval = 200;            // digitalpin 13 LED indicates Test-mode, unless another mode is active ( interval < 3000)
     }
     else {
       DelayTime = 60000;
       if (interval == 200) interval = 20000;          // this makes sure that digitalpin 13 LED indicator will be switched off
     }

  if (digitalRead(NightSwitch) == HIGH) {              // Second: check whether the switch for night-time mode has been pressed
    while (digitalRead(NightSwitch) == HIGH) {
      interval = 50;                                   // while the NightSwitch is pressed, the digitalpin 13 LED flashes rapidly
      IndicatorLED();      
    }
    interval = 10;                                     // extreme short interval indicates via digitalpin 13 LED that panel LEDs are on in NightSwitch mode (PWM = 2)
    WCleds = 2;                                        // WC-leds state-variable to keep track of on situation of the panel LEDs
    BKleds = 2;
    TimerWC = millis();                                // remember start time of on-state of panel LEDs
    TimerBK = millis();    
    analogWrite(WCpin, WCleds);                        // panel LEDs on
    analogWrite(BKpin, BKleds);    
  }
    
  if (analogRead(PIRwc) > 10){                         // Third: check activity of the WC PIR
    interval = 2000;                                   // used for indicator digital pin 13 LED
    TimerWC = millis();                                // if WC leds are already on, the period start time is renewed
    if (WCleds == 0) {                                 // only act if panel LEDs are OFF
      WCleds = 255;
      FadeUp(WCpin);
    }
  }
  
  if (analogRead(PIRbk) > 10){                         // Also do this for other PIR-driven LED panels (here the Bathroom)
    interval = 2000;
    TimerBK = millis();                                // if BK leds are already on, the period start time is renewed
    if (BKleds == 0) {
      BKleds = 255;
      FadeUp(BKpin);
    }
  }  

  if ((millis() - TimerWC) >= DelayTime*WCDelayFactor && WCleds > 0) {    // Fourth: check whether it is time to fade-down the panel LEDs
    FadeDown(WCpin, WCleds);
    WCleds = 0;
    if (BKleds == 0) interval = 20000;                                    // digitalpin 13 LED will indicate that there is no activity in the LED panels
  }

  if ((millis() - TimerBK) >= DelayTime*BKDelayFactor && BKleds > 0) {    // also check other panel-LEDs units whether it is time to fade down
    FadeDown(BKpin, BKleds);
    BKleds = 0;
    if (WCleds == 0) interval = 20000;                                    // digitalpin 13 LED will indicate that there is no activity in the LED panels
  }   
}

 
  void FadeUp(int Pin) {                                                  // subroutine to fade-up the panel LEDs
    // fade in from min to max in increments of 5 points:
    for(int fadeValue = 0 ; fadeValue <= 255; fadeValue++) { 
      // sets the value (range from 0 to 255):
      analogWrite(Pin, fadeValue); 
      IndicatorLED();                                                     // continue the indication service (digital pin 13 LED) during this period
      // wait some milliseconds to see the dimming effect    
      delay(20);                            
    }
  } 
  
  void FadeDown(int Pin, int fadeValue) {                                 // subroutine to fade-down the panel LEDs
    // fade out from max to min in increments of 5 points:
    for(fadeValue ; fadeValue >= 0; fadeValue--) { 
      // sets the value (range from 0 to 255):
      analogWrite(Pin, fadeValue); 
      IndicatorLED();                                                     // continue the indication service (digital pin 13 LED) during this period    
      // wait some milliseconds to see the dimming effect    
      delay(6);                            
    }
     digitalWrite(TestOut, LOW);                                         // switch off the digital pin 13 indicator LED 
  }
  

  void IndicatorLED() {                                                  // This subroutine controls the flashing of the TestOut LED (digital pin 13 LED)
     if (interval < 3000) {
     currentMillis = millis();                      
       if(currentMillis - previousMillis > interval) {
         previousMillis = currentMillis;                                 // save the last time you blinked the LED
           if (ledState == LOW)                                          // if the LED is off turn it on and vice-versa:
             ledState = HIGH;
            else
              ledState = LOW;
            digitalWrite(TestOut, ledState);                             // set the LED with the ledState of the variable:
       }
     }
     else digitalWrite(TestOut, LOW);                                   // if interval = 20000, then we are in normal operation and digitalpin 13 indicator LED is switched OFF
  }
