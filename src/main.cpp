#include <Arduino.h>


//===================================================================
// Feature configuration

#ifndef LOG_PIN
 #define LOG_PIN  A1  // An Arduino pin number or pin definition macro.
 #define LOG_PIN_NAME  "A1"
#endif

#define LOG_BAUD  9600  // Baud rate to log on (may not matter for all serial ports on all chips).
#define REPORT_PERIOD_S  4  // How often to report the voltage.

// What to report for each reading:
#define LOG_AD  1  // the raw A/D value
#define LOG_VOLTAGE  2  // the voltage: requires setting SUPPLY_V and AD_MAX accurately.

#define LOG_MODE  LOG_VOLTAGE  // Choose one of the modes above.

#ifndef AD_MAX
 #define AD_MAX  1024  // the maximum value returned by the A/D on this chip.
#endif

#ifndef SUPPLY_V
 #define SUPPLY_V  3.3  // The reference for the A/D on this chip.
#endif

// #define LOG_COUNT  // Define to also note how many readings there were
  // (mainly useful initially, to see what the normal sample rate is)

// Define these to flag when the minimum V gets to these voltages.
// Currently requires LOG_MODE = LOG_VOLTAGE.
#ifndef WARNING_LEVEL_V
 #define WARNING_LEVEL_V  3.0
#endif
#ifndef ERROR_LEVEL_V
 #define ERROR_LEVEL_V  2.89
#endif

// Define these to log more quickly when we've seen a warning or error state.
// Can be floating-point expressions.
#define WARNING_PERIOD_S  1
#define ERROR_PERIOD_S 0.2


//===================================================================
// Internal use constants

#define STATE_NORMAL  1
#define STATE_WARNING  2
#define STATE_ERROR  3
int state = STATE_NORMAL;

// Symbols for A/D conversion
#define NO_READING  -1
#define MS_PER_SEC  1000


//===================================================================
// Reading variables

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

void set_state(int newState)
{
  state = newState;
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

void report(void)
{
  #if LOG_MODE == LOG_AD
  Serial.printf("%d", minReading);
  #elif LOG_MODE == LOG_VOLTAGE
  Serial.print(reading_to_float(minReading), 2);
  Serial.print(" V");
  #endif

  #ifdef LOG_COUNT
  Serial.print(" (");
  Serial.print(readingCount);
  Serial.print(")");
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
  }
}

void set_led_color(void)
{
  
}

void setup() {
  pinMode(LOG_PIN, INPUT);
  Serial.begin(LOG_BAUD);

  Serial.println();
  Serial.print("Saggydog: log min ");
  #if LOG_MODE == LOG_AD
    Serial.print("A/D value");
  #elif LOG_MODE == LOG_VOLTAGE
    Serial.print("voltage");
  #endif
  Serial.print(" on pin ");
  Serial.print(LOG_PIN);
  Serial.print(" = ");
  Serial.print(LOG_PIN_NAME);
  Serial.print(".\n");

  #ifdef ERROR_LEVEL_V
  Serial.print("Highlight errors below ");
  Serial.print(ERROR_LEVEL_V, 2);
  Serial.print(" V.\n");
  #endif
  #ifdef WARNING_LEVEL_V
  Serial.print("Warn below ");
  Serial.print(WARNING_LEVEL_V, 2);
  Serial.print(" V.\n");
  #endif

  reset_reading();
}

void loop() {
  accumulate_readings();
  set_led_color();
  maybe_report();
}
