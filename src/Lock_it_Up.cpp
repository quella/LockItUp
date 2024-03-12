#include <Arduino.h>                                          // Include Arduino lib for previous calls
#include <SPI.h>                                              // SPI lib required for display
#include <Wire.h>                                             // Wire lib required for display
#include <Adafruit_GFX.h>                                     // Adafruit.GFX required for display
#include <Adafruit_SSD1306.h>                                 // Adafruit.SSD1306 required for display
#define OLED_RESET     -1                                     // Reset pin # (or -1 if sharing Arduino reset pin)

void FlashLED(int x);                                         // Declare Flash LED Routine
void StartEvent();                                            // Declare the Start of an Event Routine
void RelayOff();                                              // Declare Turn Relay Off Routine
void RelayOn();                                               // Declare Turn Relay On Routine
void Lock();                                                  // Declare Lock Device Routine
void unLock();                                                // Declare Unlock Device Routine
uint32_t generateRandomSeed();                                // Declare Random Seed Generation Function

Adafruit_SSD1306 display(128, 64, &Wire, -1);                 // Init the OLED display 
//
// Define GPIO Pins
// NOTE: GPIO D5 and D6 are additionally used for I2C communicatioins for the OLED Screen
//
const int LED2 = 16;                                          // Assign LED1 to pin GPIO16 (Event on/off) GPIO: D0
const int SPEAKER = D7;                                       // Speaker Output digital Pin D7
const int Lock1 = D1;                                         // Lock 1 on D1
const int RelayPWM = D2;                                      // Relay #1 PWM on D3
const uint8_t seedPin = A0;                                   // Define pin to get RandomSeed from
//
// Varibles to tweek outcome of the session, event length, and probibility event is active
//
const int MinRandomNumber = 45;                               // Min Random Number in Minutes
const int MaxRandomNunber = 180;                              // Max Random Number in Minutes (max 240 due to arrays)
const int EventDurationMin = 5;                               // Min value to have the event occur in Seconds
const int EventDurationMax = 90;                              // Max value to have the event occur in Seconds
const int Probability = 35;                                   // Modify the probability an event will be true in percent (%0-%100) Higher number better chance
//
//
//
unsigned long EventStartDelay = 120000;                       // 2 miniute delay in Milliseconds
unsigned long MaxEventNumber = 120000;                        // 2 minute event MAX minus a few Milliseconds
unsigned long LockTimer = 60000;                              // LockTime in Milliseconds
unsigned long DeviceStartTime;                                // Mills value at start of locking

unsigned int temp;

int CurrentEvent = 0;                                         // Store the current event number
int EventProbability = 0;                                     // Temp Store the events probability 
int NumberOfEvents = 0;                                       // This holds the number of events based on total time
int EventCounter = 0;                                         // This is to keep track of what event we are on
int MinuteTimer = 10;                                         // LockTime in Minutes
int LockStatus = 0;                                           // LockStatus 0=unlocked 1=locked
int MinutesRemaining = 0;                                     // Be able to flash LED when ready to unlock
int DisplayHours = 0;                                         // User to display remaining Hours (from MinutesRemaining)
int DisplayMinutes = 0;                                       // Used to display remaining Munutes (from MinutesRemaining)
int counter = 0;                                              // Loop Counter
int beeped = 0;                                               // Varible to say if the beep was done.
int EventPWM = 0;                                             // Varible to store PWM Motor Value
bool ActiveEvent = false;                                     // Varible if the buzz is activated.
//
// setup array to store event data
//
unsigned long EventTimeArray[120];                            // An array to store the millis of the different events
int EventDurationArray[120];                                  // An array to store the duration of the event in Seconds
int EventPowerArray[120];                                     // An array to store the motor power level (0=Low, 1=Med, 2=High)
bool EventEnabledArray[120];                                  // An array to store if an event is true or not
//
// Start Setup Loop
//
void setup() {                                                
  Wire.begin(14,12);                                          // Change I2C pins to D5 / D6
  analogWriteRange(1023);                                     // Set Motor Range between 0-1023
  pinMode(LED2, OUTPUT);                                      // Init Onboard LED
  pinMode(SPEAKER, OUTPUT);                                   // Init Speaker Output
  pinMode(Lock1, OUTPUT);                                     // Init Lock 1
  pinMode(RelayPWM, OUTPUT);                                  // Init Relay #1
 
  digitalWrite(LED2, HIGH);                                   // Turn Off LED
  digitalWrite(Lock1, LOW);                                   // Disable locks when starting
  digitalWrite(RelayPWM, LOW);                                // Disable relay when starting
 
  Serial.begin(9600);                                         // Enable serial logging

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);                  // Init the Display 
  display.clearDisplay();                                     // Clear the Display
  
  randomSeed(generateRandomSeed());                           // Set Random Number Generator Seed
  MinuteTimer = random(MinRandomNumber, MaxRandomNunber);     // Generate Random Number based on MIN/MAX Assigned
  LockTimer = MinuteTimer * 60000;                            // Set the duration of lock from Minutes to Milliseconds

  Serial.print("Timer Amount:");                              // Serial Print DEBUG Message on Minutes Left
  Serial.println(MinuteTimer);                                // Serial Print DEBUG Message on Minutes Left
  
  DeviceStartTime = millis();                                 // This is the time the device started up in Milliseconds 
//
// Loop and populate the events, duration, true/false and power level details
//
  NumberOfEvents = ((MinuteTimer-(EventStartDelay/60000)) / (MaxEventNumber/60000));  // Calculate the number of total events during the lock event
  Serial.print("Number of Events: ");                          //
  Serial.println(NumberOfEvents);                              //  SERIAL PRINT DEBUG MESSAGES
  Serial.print("Start Time: ");                                //
  Serial.println(DeviceStartTime);                             //

  for (counter = 0; counter != NumberOfEvents; counter++) {    // Populate the array of event segments, true/false, duration arrays
    EventTimeArray[counter] = (DeviceStartTime + EventStartDelay + (MaxEventNumber * counter));  //Find the start of each segment and store it
    EventProbability = random(1,100);                          // This selects a random nummber to allow chance of event being live (higher better)
    if (EventProbability >= Probability) {                     // Compares the Probability variable to random number and use this to create probability
      EventEnabledArray[counter] = false;                      // Set the event to be false (no action)
      EventDurationArray[counter] = 0;                         // Set duration of an active event to Zero
    } else {
      EventEnabledArray[counter] = true;                       // If the event is TRUE (Active) mark it in the array
      randomSeed(generateRandomSeed());                        // Set Random Number Generator Seed
      EventDurationArray[counter] = random(EventDurationMin,EventDurationMax);  // If event true, select a random time on it being on (variiales)
//
// Randomly weighted set Power Level from 0=Low to 2=High
//
      randomSeed(generateRandomSeed());                        // Set random seed each time through for better numbers
      temp=random(1000);                                       // Set a random number from 1 - 100 for probability of power 
      if (temp >= 950) {                                       // Set High to 5% (950-1000) values
        EventPowerArray[counter] = 2;                          // Set power varible to 2 for HIGH
      }
      else if (temp < 700) {                                   // Set Low to 70% (0-700) values
        EventPowerArray[counter] = 0;                          // Set power varible to 0 for LOW
      }
      else {                                                   // Set Low to 25% (701-950) values
        EventPowerArray[counter] = 1;                          // Set power varible to 1 for MED
      }                   
    }
//
// Dump Debug data to serial output
//
    Serial.print(counter);                                     //
    Serial.print(": ");                                        //
    Serial.print(EventTimeArray[counter]);                     //
    Serial.print(" : ");                                       // DEBUG Serial Output Can Be removed when done
    Serial.print(EventEnabledArray[counter]);                  //
    Serial.print(" : ");                                       //
    Serial.print(EventDurationArray[counter]);                 //
    Serial.print(" : ");                                       //
    Serial.println(EventPowerArray[counter]);                  //
  }
 
  Lock();                                                      // Ater all SetUp Process, Start the Lock process
}
//
// The loop function runs forever
//
void loop() {                                                  // Enter MAIN LOOP Function
 
  MinutesRemaining = ((LockTimer - millis())/60000+1);         // Calculate and store the Minutes locked remaining
  DisplayHours = MinutesRemaining / 60;                        // Convert MinutesRemaining to Hours for Display
  DisplayMinutes = MinutesRemaining % 60;                      // Convert MinutesRemaining to Minutes for Display

  display.clearDisplay();                                      // Clear the display in the loop
  
  if (LockStatus == 1) {                                       // If LockStatus is 1 display LOCKED message
    display.setCursor(0,0);                                    // Set Cursor to print message
    display.setTextSize(2);                                    // Set text size for display
    display.print("LOCKED  ");                                 // Print "LOCKED" to OLED
    display.setTextSize(2);                                    // Setup Minutes Remaining Message
    display.setTextColor(WHITE);                               // Set text color to WHITE
    display.setCursor(60, 16);                                 // Move cursor to next area to print
    display.print("Left");                                     // Print the text on the display
    display.setCursor(0, 16);                                  // Move cursor to the place for minutes locked
    display.print(DisplayHours);                               // Print to the display the lock hours remaining
    display.print(":");                                        // Display a : (colon) between the houns and minutes
    display.print(DisplayMinutes);                             // Print to the display the lock minutes remaining
    if (ActiveEvent) {                                         // If it is an ActiveEvent print the following
      display.setTextSize(1);                                  // Text size set smaller 
      display.setCursor(0, 34);                                // Move the cursor to a new location
      display.print("Active  ");                               // Print ACTIVE to display
      display.setCursor(0, 45);                                // Set the new cursor position
      if (EventPowerArray[CurrentEvent-1] == 0) {              // If the value in EventPowerArray is 0 print Low
        display.print("Low");                                  // Write Low to the display
      } else if (EventPowerArray[CurrentEvent-1] == 1) {       // If the value in EventPowerArray is 1 print Medium
        display.print("Medium");                               // Write Medium to the display
      } else if (EventPowerArray[CurrentEvent-1] == 2) {       // If the value in EventPowerArray is 2 print High
        display.print("High");                                 // Write High to the display
      }
      display.setCursor(45, 34);                               // Set the new cursor position
      display.print((((EventTimeArray[CurrentEvent-1]+(EventDurationArray[CurrentEvent-1]*1000))-millis())/1000)+1); // Display the seconds remaining in event
    } else {
      display.setTextSize(1);                                  // Set text size smaller
      display.setCursor(0, 34);                                // Move the cursor to a new locaiton
      display.print("InActive");                               // Print INACTIVE to display
      display.setCursor(0, 45);                                // Set cursor position
      display.print("         ");                              // Print spaces over any previous text
      display.setCursor(45, 34);                               // Set cursor position
      display.print("         ");                              // Clear Timer of the event in seconds
    }
    if (millis() < EventTimeArray[0]) {                        // If in the Start Delay Period print below
      display.setTextSize(1);                                  // Set text size to small
      display.setCursor(0, 45);                                // Move the cursor to print location
      display.print("Delay: ");                                // Print the word DELAY to display
      display.setCursor(40, 45);                               // Move the cursor to a new location
      display.print((EventTimeArray[0]-millis())/1000);        // Display the delay remaining in Seconds
    } else {                                                   // If past the DELAY time, do the following
      display.setTextSize(1);                                  // Set text size to small
      display.setCursor(0, 45);                                // Move the cursor to the new location
      display.print("             ");                          // Display blanks to cover up the past DELAY text
    }
  } else  {                                                    // If LockStatus is 0 Display UNLOCKED message
    display.setTextSize(2);                                    // Set TEXT size to #2
    display.setTextColor(WHITE);                               // Set Text Color to WHITE
    display.setCursor(0,0);                                    // Set the cursor for the message
    display.print("UNLOCKED");                                 // Print "UNLOCKED" to OLED
  }
  
  display.display();                                           // Write the data to the display
  
  if ((MinutesRemaining == 1) && (LockStatus == 1)) {          // If the minutes remaining is 1 Flash the Onboard LED FAST
    FlashLED(300);                                             // Call flash routine
  }
  
  if ((MinutesRemaining <= 5) && (MinutesRemaining > 1) && (LockStatus == 1)) {  // If the minutes remaining is 5 Flash the Onboard LED SLOW
    FlashLED(1000);                                            // Call flash routine
  }
  if ((millis() >= LockTimer) && (LockStatus == 1)) {          // If the Lock Timer runs out AND the lock is locked, call unlock Function
    unLock();
  }
  if ((millis() >= EventTimeArray[CurrentEvent]) && (millis() < EventTimeArray[CurrentEvent+1])) {  // Check to see if an event is active and if so, jump to function
    StartEvent();
  }
  if ((ActiveEvent) && (EventTimeArray[CurrentEvent-1] + (EventDurationArray[CurrentEvent-1]*1000) < millis())) {  // If the relay is on and the event is done, turn off relay
    RelayOff();
  }
}
//
// END of Main Loop Function
//
//
// NOTE: Below are functons called by the program
//
//
// Engage the locks and return
//
void Lock() {                                                  // FUNCTION to engage the lock(s)
  int NumberOfTones = 5;                                       // Set number of tones to play
  int tones[] = {226,247,220,196,196};                         // Tones to play when locked
  digitalWrite(Lock1, HIGH);                                   // Lock1 Locked
    LockStatus = 1;                                            // Set Locked Status to TRUE (1)
    for (int i=0; i < NumberOfTones; i++) {                    //
    tone(SPEAKER, tones[i]);                                   // Loop to play scale when locked
    delay(100);                                                //
  }
   noTone(SPEAKER);                                            // Once locked stop speaker tone
   Serial.println("LOCKED");                                   // Print LOCKED to Serial interface
  return;                                                      // Return from calling Function
}
//
// Time to unlock and play unlock tone
//
void unLock() {                                                // FUNCTION to unlock the lock(s)
  int NumberOfTones = 5;                                       // Set Number of tones to play
  int tones[] = {196,196,220,247,262};                         // Tones used to play when unlocked

  LockStatus = 0;                                              // Set Locked Status to FALSE (0)
  for (int i=0; i < NumberOfTones; i++) {                      //
    tone(SPEAKER, tones[i]);                                   // Loop to play scale when unlocked
    delay(100);                                                // Delay before playing next tone
  }
   noTone(SPEAKER);                                            // Once unlocked stop speaker tone
   digitalWrite(Lock1, LOW);                                   // Lock1 Unlocked
   digitalWrite(LED2, HIGH);                                   // Set Onboard LED High to signal an event
   Serial.println("UNLOCKED");                                 // Print UNLOCKED to serial interface
  return;                                                      // Retun from calling function
}
//
// Event On (enable relay)
//
void RelayOn() {                                               // Engage the Relay for an active event
  
  if (!ActiveEvent) {
    analogWrite(RelayPWM, 1023);                               // Start the motor primmed at high speed
    delay(100);                                                // Delay for 10ms to get the motors to speed
  }
  if (EventPowerArray[CurrentEvent] == 0) {                    // If the stored array value is 0 then PWM = 500 (Low)
    EventPWM = 500;
  } else if (EventPowerArray[CurrentEvent] == 1) {             // If the stored array value is 1 then PWM = 700 (Med)
    EventPWM = 700; 
  } else if (EventPowerArray[CurrentEvent] == 2) {             // If the stored array value is 2 then PWM = 1023 (High)
    EventPWM = 1023;
  }
  analogWrite(RelayPWM, EventPWM);                             // Write that random value out to the PWM port
  digitalWrite(LED2, LOW);                                     // Set Event LED Signal ON
  Serial.println("Relays Active Event");                       // 
  Serial.print("Random PWM = ");                               // Serial print some debug info 
  Serial.println(EventPWM);                                    //
  ActiveEvent = true;                                          // Set the active event value to TRUE
  return;                                                      // Return fromt he calling function
}
//
// Event Off (disable Relay)
//
void RelayOff() {                                              // Function used to turn off the active relay event
  Serial.println("Relays Disengage Event");                    // Serial print some debug details
  digitalWrite(RelayPWM, LOW);                                 // Disengage the two relay pins
  digitalWrite(LED2, HIGH);                                    // Set Event LED Signal OFF
  ActiveEvent = false;                                         // Set active event flag back to FALSE
  return;                                                      // Return from th function
}
//
// Event Check to see what event and if it is on or not
//
void StartEvent() {                                            // Did we just enter an event period
  Serial.print("Started Event: ");                             // Serial Output for Debug Reasons
  Serial.println(CurrentEvent);                                // 
  if (EventEnabledArray[CurrentEvent] == true) {               // Check to see if the stored event is true or not
    Serial.print("Event for seconds: ");                       // If an ACTIVE event Print some Debug info
    Serial.println(EventDurationArray[CurrentEvent]);          //
    RelayOn();                                                 // Call to engage the relay to turn ON
  } else {                                                     // 
    Serial.println("False Event");                             // If not an active event, move on
  }                                                            // 
  CurrentEvent = CurrentEvent + 1;                             // Increment the event and wait for next one
  return;                                                      // Return from the function
}
//
// Function that flashes and beeps speaker if 2min or less
//
void FlashLED(int x) {                                         // FUNCTION to Flash Onboard LED and beep Speaker (if used)
  digitalWrite(LED2, (millis() / x ) % 2);                     // Toggle on or off the LED based on Modulus of Millis and Duration (x)
  if (MinutesRemaining <= 1) {                                 // One minute and below, warn with a beep if unable to see flash
    if (((millis() / x ) % 2) == 0) {                          // If there is less than a minute of locked time, tone the speaker every second
      tone(SPEAKER, 349);                                      // Tone the speaker 
    } else noTone(SPEAKER);                                    // If not a Tone time, ensure speaker is not left in a tone state
  }
  return;                                                      // Return from the function
}
//
// Function that best generates a random number without an external source
//
uint32_t generateRandomSeed()
{
  uint8_t  seedBitValue  = 0;
  uint8_t  seedByteValue = 0;
  uint32_t seedWordValue = 0;
 
  for (uint8_t wordShift = 0; wordShift < 4; wordShift++)                          // 4 bytes in a 32 bit word
  {
    for (uint8_t byteShift = 0; byteShift < 8; byteShift++)                        // 8 bits in a byte
    {
      for (uint8_t bitSum = 0; bitSum <= 8; bitSum++)                              // 8 samples of analog pin
      {
        seedBitValue = seedBitValue + (analogRead(seedPin) & 0x01);                // Flip the coin eight times, adding the results together
      }
      delay(1);                                                                    // Delay a single millisecond to allow the pin to fluctuate
      seedByteValue = seedByteValue | ((seedBitValue & 0x01) << byteShift);        // Build a stack of eight flipped coins
      seedBitValue = 0;                                                            // Clear out the previous coin value
    }
    seedWordValue = seedWordValue | (uint32_t)seedByteValue << (8 * wordShift);    // Build a stack of four sets of 8 coins (shifting right creates a larger number so cast to 32bit)
    seedByteValue = 0;                                                             // Clear out the previous stack value
  }
  return (seedWordValue);
}