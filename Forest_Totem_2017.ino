#include <Adafruit_NeoPixel.h>

#define SERIAL_DEBUG_DISABLE

#define NUM_PIXELS 81
#define NUM_COLORS 4
unsigned long interval=5, lastInterval;  // the time we need to wait
unsigned long previousMillis=0;

uint32_t currentColor;// current Color in case we need it
uint16_t currentPixel = 0;// what pixel are we operating on

uint8_t colorCounter = 0;

uint32_t colorArray[NUM_COLORS];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, 6, NEO_GRB + NEO_KHZ800);

// Testing interrupt-based analog reading
// ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

// High when a value is ready to be read
volatile int readFlag;

// Value to store analog result
volatile long analogVal;
long avgAnalogVal;

int locateComplete;

#if defined ( LED_DEBUG_ENABLE)
const int ledPin =  13;
unsigned long ledDelay;
#endif

typedef enum {
   COLORWIPE,
   RAINBOW,
   LOCATE,
   THEATERCHASE,
   NONE
} tStrandPattern;

tStrandPattern strandPattern = COLORWIPE, lastPattern;

// Initialization
void setup(){

   // clear ADLAR in ADMUX (0x7C) to right-adjust the result
   // ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits)
   //ADMUX &= B11011111;

   // Set REFS1..0 in ADMUX (0x7C) to change reference voltage to the
   // proper source (01)
   //ADMUX |= B00100000;

   // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
   // input
   ADMUX &= B11110000;

   // Set MUX3..0 in ADMUX (0x7C) to read from AD8 (Internal temp)
   // Do not set above 15! You will overrun other parts of ADMUX. A full
   // list of possible inputs is available in Table 24-4 of the ATMega328
   // datasheet
   /*ADMUX |= B00000000; //Z Axis*/
   ADMUX |= B00000001; //Y Axis
   /*ADMUX |= B00000011; //X Axis*/
   // ADMUX |= B00001000; // Binary equivalent

   // Set ADEN in ADCSRA (0x7A) to enable the ADC.
   // Note, this instruction takes 12 ADC clocks to execute
   ADCSRA |= B10000000;

   // Set ADATE in ADCSRA (0x7A) to enable auto-triggering.
   ADCSRA |= B00100000;

   // Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
   // This means that as soon as an ADC has finished, the next will be
   // immediately started.
   ADCSRB &= B11111000;

   // Set the Prescaler to 128 (16000KHz/128 = 125KHz)
   // Above 200KHz 10-bit results are not reliable.
   ADCSRA |= B00000111;

   // Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
   // Without this, the internal interrupt will not trigger.
   ADCSRA |= B00001000;

   // Enable global interrupts
   // AVR macro included in <avr/interrupts.h>, which the Arduino IDE
   // supplies by default.
   sei();

   // Kick off the first ADC
   readFlag = 0;
   // Set ADSC in ADCSRA (0x7A) to start the ADC conversion
   ADCSRA |=B01000000;

#if defined ( LED_DEBUG_ENABLE)
   pinMode(ledPin, OUTPUT);
   digitalWrite(ledPin, LOW);
   ledDelay=0;
#endif
   analogVal = 0;
   /*
   colorArray[0] = strip.Color(64,0,0);
   colorArray[1] = strip.Color(0,64,0);
   colorArray[2] = strip.Color(0,0,64);
   colorArray[3] = strip.Color(32,32,0);
   colorArray[4] = strip.Color(32,0,32);
   colorArray[5] = strip.Color(0,32,32);
   */

   colorArray[0] = strip.Color(255,0,0);
   colorArray[1] = strip.Color(0,0,255);
   colorArray[2] = strip.Color(128,128,0);
   colorArray[3] = strip.Color(0,255,0);


   currentColor = colorArray[0];
   currentPixel = 0;
   strip.begin();
   strip.show(); // Initialize all pixels to 'off'
   strandPattern = COLORWIPE;
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.begin(115200);
   Serial.println("DEBUGGING ENABLED!");
#endif
   previousMillis = millis();
   interval = 10;
   avgAnalogVal = 1000;
   locateComplete = 1;
}


// Processor loop
void loop(){

   // Check to see if the value has been updated
   if (readFlag >= 5){

      // Perform whatever updating needed

      avgAnalogVal = analogVal/readFlag;
      readFlag = 0;
      analogVal = 0;

   }

   // Whatever else you would normally have running in loop().
   if(locateComplete && avgAnalogVal<=100)
   {
      //Serial.println(avgAnalogVal);
      locateComplete = 0;
#if defined ( SERIAL_DEBUG_ENABLE )
      Serial.println("IMPACT!");
#endif
      storeContext();
      clearStrand();
#if defined ( LED_DEBUG_ENABLE )
      digitalWrite(ledPin, HIGH);
      ledDelay = millis()+5000;
#endif
      previousMillis = millis();
      strandPattern = LOCATE;
      currentPixel = 0;
      interval = 20;
   }
#if defined ( LED_DEBUG_ENABLE )
   if(ledDelay <= millis())
   {
      digitalWrite(ledPin, LOW);
   }
#endif
   if ((unsigned long)(millis() - previousMillis) >= interval) {
      updatePattern();
#if defined ( SERIAL_DEBUG_ENABLE )
      Serial.print("Pattern: ");
      Serial.println(strandPattern);
      Serial.print("Interval: ");
      Serial.println(interval);
#endif
      previousMillis = millis();
   }

}

void updatePattern(){
   switch (strandPattern) {
      case COLORWIPE:
         colorWipe();
         break;
      case NONE:
         clearStrand();
         break;
      case RAINBOW:
         rainbowCycle();
         break;
      case LOCATE:
         centerOut();
         break;
      case THEATERCHASE:
         theaterChase();
      default:
         break;
   }
}

void clearStrand(){
   strip.clear();
}

void centerOut(){
   static uint8_t pixelUp = NUM_PIXELS/2;
   static uint8_t pixelDown = (NUM_PIXELS/2)-1;
   static uint8_t counter = 0;
   static uint8_t modeState = 0;
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.println("centerOut!");
#endif
   if((pixelUp == NUM_PIXELS/2 ) && ( NUM_PIXELS%2 )){
      strip.setPixelColor(pixelUp++,currentColor);
   }
   else
   {
      strip.setPixelColor(pixelUp++,currentColor);
      strip.setPixelColor(pixelDown--,currentColor);
   }
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.println(pixelUp, DEC);
   Serial.println(pixelDown, DEC);
   Serial.println("");
#endif
   strip.show();
   if((pixelUp >= NUM_PIXELS) && (modeState == 0)){
      modeState++;
      interval = 150;
      clearStrand();
   }
   else if((pixelUp >= NUM_PIXELS) && ((modeState%2) == 1)){
      modeState++;
      for(int i=0;i<NUM_PIXELS;i++){
         strip.setPixelColor(i, currentColor);
      }
      if(modeState>=8)
      {
         modeState = 10;
      }
   }
   else if((pixelUp >= NUM_PIXELS) && ((modeState%2) == 0)){
      modeState++;
      clearStrand();
   }
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.print("modeState: ");
   Serial.println(modeState);
#endif


   if((pixelUp >= NUM_PIXELS) && (modeState == 10)){
      pixelUp=NUM_PIXELS/2;
      pixelDown=(NUM_PIXELS/2)-1;
      counter++;
      clearStrand();
      modeState = 0;
      interval = 20;
      currentColor = colorArray[(++colorCounter%NUM_COLORS)];
#if defined ( SERIAL_DEBUG_ENABLE )
      Serial.print("Locate Cnt: ");
      Serial.println(counter);
#endif
   }
   if(counter >= 10){
      counter = 0;
      locateComplete = 1;
      restoreContext();
   }
}

void colorWipe(){
   static uint16_t counter = 0;
   strip.setPixelColor(currentPixel,currentColor);
   strip.show();
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.print("Wipe Cnt: ");
   Serial.println(counter);
#endif
   if(currentPixel++ >= NUM_PIXELS){
      currentPixel = 0;
      counter++;
      currentColor = colorArray[(++colorCounter%NUM_COLORS)];
   }
   if(counter >= 20){
#if defined ( SERIAL_DEBUG_ENABLE )
      Serial.println("Wipe Reset!");
#endif
      strandPattern = RAINBOW;
      interval = 20;
      counter=0;
   }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
   uint16_t i;
   static uint16_t counter = 0;
   for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + counter) & 255));
   }
   strip.show();
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.print("Rainbow Cnt: ");
   Serial.println(counter);
#endif
   if(counter++ >= 256*5){
      strandPattern = THEATERCHASE;
      interval = 50;
      counter = 0;
#if defined ( SERIAL_DEBUG_ENABLE )
      Serial.println("Rainbow Reset!");
#endif
   }

}

//Theatre-style crawling lights.
void theaterChase() {
   static uint16_t counter = 0;
   for(int i=0; i< NUM_PIXELS; i++)
   {
      if ((i + currentPixel) % 3 == 0)
      {
         strip.setPixelColor(i, currentColor);
      }
      else
      {
         strip.setPixelColor(i, 0);
      }
   }
   strip.show();
   currentPixel++;
   if(currentPixel>=NUM_PIXELS){
      counter++;
      currentPixel=0;
      currentColor = colorArray[(++colorCounter%NUM_COLORS)];
   }
   if(counter>=10){
      counter=0;
      strandPattern = COLORWIPE;
      interval=10;
   }
}


// Input a value 0 to 255 to get a color value.
// // The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
   WheelPos = 255 - WheelPos;
   if(WheelPos < 85) {
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
   } else if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
   } else {
      WheelPos -= 170;
      return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
   }
}

void storeContext(){
   lastPattern = strandPattern;
   lastInterval = interval;
}

void restoreContext(){
   strandPattern = lastPattern;
   interval = lastInterval;
#if defined ( SERIAL_DEBUG_ENABLE )
   Serial.println("Restore...");
   Serial.print("Pattern: ");
   Serial.println(strandPattern);
   Serial.print("Interval: ");
   Serial.println(interval);
#endif
}

// Interrupt service routine for the ADC completion
ISR(ADC_vect){

   // Done reading
   readFlag++;

   // Must read low first
   analogVal += ADCL | (ADCH << 8);

   // Not needed because free-running mode is enabled.
   // Set ADSC in ADCSRA (0x7A) to start another ADC conversion
   // ADCSRA |= B01000000;
}




