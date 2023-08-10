#include <Arduino.h>

#define FASTLED_INTERNAL  // LED is not connected to hardware SPI pins; suppress warnings
#include <FastLED.h>


//===================================================================
// Feature configuration

#define LOG_PIN  A1  // An Arduino pin number or pin definition macro.
#define LOG_BAUD  9600  // Baud rate to log on (may not matter for all serial ports on all chips).
#define REPORT_PERIOD_S  4  // How often to report the voltage.

// What to report for each reading:
#define LOG_AD  1  // the raw A/D value

#define LOG_VOLTAGE  2  // the voltage: requires setting SUPPLY_V and AD_MAX accurately.
#define AD_MAX  4096  // the maximum value returned by the A/D on this chip.
#define SUPPLY_V  3.3  // The reference for the A/D on this chip.

#define LOG_MODE  LOG_VOLTAGE  // Choose one of the modes above.

// #define LOG_COUNT  // Define to also note how many readings there were

// Define these to flag when the minimum V gets to these voltages.
// Currently requires LOG_MODE = LOG_VOLTAGE.
#define WARNING_LEVEL_V  3.0
#define ERROR_LEVEL_V  2.89

// Define these to log more quickly when we've seen a warning or error state.
// Can be floating-point expressions.
#define WARNING_PERIOD_S  1
#define ERROR_PERIOD_S 0.2

// Define this to pulse an WS2812-compatible RGB LED on this pin.
#define RGB_LED_PIN  8


//===================================================================
// States

#define STATE_NORMAL  1
#define STATE_WARNING  2
#define STATE_ERROR  3
int state = STATE_NORMAL;

// Symbols for A/D conversion
#define NO_READING  -1
#define MS_PER_SEC  1000


void set_state(int newState)
{
  state = newState;
}


//===================================================================
// Readings

// The lowest reading we've seen recently, in D/A units.
int minReading;
unsigned long lastReport;
unsigned long readingCount;

void reset_reading(void)
{
  minReading = NO_READING;
  lastReport = millis();
  readingCount = 0;
  state = STATE_NORMAL;
}

float reading_to_float(int reading)
{
  return reading * 1.0 / AD_MAX * SUPPLY_V;
}

void accumulate_readings(void)
{
  int newValue = analogRead(LOG_PIN);
  if (minReading == NO_READING)
    minReading = newValue;
  else
    minReading = min(minReading, newValue);

  #if defined(ERROR_LEVEL_V) || defined(WARNING_LEVEL_V)
    float v = reading_to_float(newValue);
  #endif

  #ifdef ERROR_LEVEL_V 
  if (state < STATE_ERROR && v <= ERROR_LEVEL_V)
    set_state(STATE_ERROR);
  #endif

  #ifdef WARNING_LEVEL_V
    #ifdef ERROR_LEVEL_V
      else
    #endif
  if (state < STATE_WARNING && v <= WARNING_LEVEL_V)
    set_state(STATE_WARNING);
  #endif

  #ifdef LOG_COUNT
  ++readingCount;
  #endif
}


//===================================================================
// LEDs

#define MAX_LED_BRIGHTNESS  255
#define LED_FADE_STEP_MS  50

CRGB leds[1];
unsigned long lastLedFade = 0;

void setup_led(void)
{
  FastLED.addLeds<WS2812, RGB_LED_PIN, GBR>(leds, 1);
  FastLED.setBrightness(140);
  FastLED.showColor(CRGB::MediumVioletRed);
}

void set_led_to_state(void)
{
  switch (state) {
  case STATE_WARNING:
    leds[0] = CRGB::Gold;
    break;
  case STATE_ERROR:
    leds[0] = CRGB::Red;
    break;
  default:
    leds[0] = CRGB::Blue;
  }
  FastLED.show();

  lastLedFade = millis();
}

void fade_led(void)
{
  if (millis() - lastLedFade > LED_FADE_STEP_MS) {
    // fadeToBlackBy(leds, 1, 1);
    // FastLED.show();

    lastLedFade = millis();
  }
}


//===================================================================
// Reporting to console

void report(void)
{
  #if LOG_MODE == LOG_AD
  Serial.printf("%d", minReading);
  #elif LOG_MODE == LOG_VOLTAGE
  Serial.printf("%.2f V", reading_to_float(minReading));
  #endif

  #ifdef LOG_COUNT
  Serial.printf(" (%d)", readingCount);
  #endif

  #ifdef ERROR_LEVEL_V
    if (state == STATE_ERROR)
      Serial.print(" !!!!");
  #endif

  #ifdef WARNING_LEVEL_V
    #ifdef ERROR_LEVEL_V
      else
    #endif
    if (state == STATE_WARNING)
      Serial.print(" ??");
  #endif

  Serial.println();
}

void maybe_report(void)
{
  int periodMs = REPORT_PERIOD_S * MS_PER_SEC;

  #if defined(ERROR_LEVEL_V) && defined(ERROR_PERIOD_S)
    if (state == STATE_ERROR)
      periodMs = ERROR_PERIOD_S * MS_PER_SEC;
  #endif

  #if defined(WARNING_LEVEL_V) && defined(WARNING_PERIOD_S)
    #if defined(ERROR_LEVEL_V) && defined(ERROR_PERIOD_S)
      else
    #endif
    if (state == STATE_WARNING)
      periodMs = WARNING_PERIOD_S * MS_PER_SEC;
  #endif

  if (millis() - lastReport > periodMs) {
    report();
    reset_reading();
    set_led_to_state();
  }
}


//===================================================================
// Main

void setup() {
  pinMode(LOG_PIN, INPUT);

  setup_led();

  Serial.begin(LOG_BAUD);

  Serial.println();
  Serial.print("Saggydog: log min ");
  #if LOG_MODE == LOG_AD
    Serial.print("A/D value");
  #elif LOG_MODE == LOG_VOLTAGE
    Serial.print("voltage");
  #endif
  Serial.printf(" on pin %d.\n", LOG_PIN);

  #ifdef ERROR_LEVEL_V
  Serial.printf("Highlight errors below %.2f V.\n", ERROR_LEVEL_V);
  #endif
  #ifdef WARNING_LEVEL_V
  Serial.printf("Warn below %.2f V.\n", WARNING_LEVEL_V);
  #endif

  reset_reading();
}

void loop() {
  accumulate_readings();
  fade_led();
  maybe_report();
}
