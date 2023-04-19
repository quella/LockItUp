Project Purpose:
The purpose of this project was to create a portable low blocking (as few delay functions)
locking event that would run the duration of the program. I wanted to additionally have a
random event occurs at a set increment (every 2 minutes) and if that event was an active 
event to randomly trigger the relay for a period of time.  Lastly, all of this was to run using
low power requirements; 5v for motors and 3.3v for the MCU.   There is also a built-in
delay function that allows the user to prepare for the events without one triggering while
setting up.  

Many of the main variables are configurable so the user can change:
 - Delay Start
 - Total time locked (Min/Max)
 - Event time During an active event (Min/Max)
 - Probability of an event being active (0 = None / 100 = Always)

Project Built using:
- ESP8266 NodeMCU v0.9
- HW-588A Motor Driver Shield
- 5v to 24v Boots Converter Module
- Electromagnetic Fire Door Holder (12v)
- Relay Module to Trigger Event
- 1.5" OLED Display
- Any power/battery bank to drive the motors
