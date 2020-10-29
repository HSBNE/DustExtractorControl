// Beau's totally newb code 24102019
// note the control_extractor and control_cleaner outputs are inverted due to relay board configuration
//
// 01082020 - bugfix - max run timer was looking for state_request_start HIGH rather than state_request_start LOW and causing unresponsiveness issues after an hour. fixed bug.
// 21102020 - bugfix - added debounce on bin sensor for 15 seconds as it was causing issues from sporadic triggering. Adjusted spin down time from 10 to 15 seconds and control_cleanering time from 10 to 15 seconds.

// Minimum run timer of 1 minute
// Takes 40 seconds to restart after stopping due to control_cleanering
// Will time out after half an hour of continuous use
// Dust filter is cleaned for 15 seconds every time the machine is stopped.

#include <ezButton.h>
#include <JLed.h>

const int sensor_binfull = 2;     // the pin of the bin level sensor
const int button_start =  3;      // the the pin of the start button
const int button_stop =  4;       // the pin of the stop button
const int control_extractor =  8; // the pin of the control_extractoror contactor
const int control_cleaner =  13;  // the pin of the control_cleanering contactor
const int indicate_red =  10;     // the pin of the bin full indicator
const int indicate_yellow = 11;   // the pin of the delay control_cleanering indicator
const int indicate_green = 12;    // the pin of the running indicator

int state_request_start = LOW;    // status of start button, latches until control_extractoror starts
int state_request_stop = LOW;     // status of stop button, latches until control_extractoror stops
int state_binfull = LOW;          // status of bin - full or empty

unsigned long TIME_E1 = 0;
unsigned long TIME_0_E1 = 0;

unsigned long RUNTIME_E1_MIN = (1000UL * 60 * 5);  // 5min
unsigned long RUNTIME_E1_MAX = (1000UL * 60 * 30); // 30min

ezButton sensor_binfull_debounced(sensor_binfull); // attaches ezButton object to bin sensor pin
ezButton button_start_debounced(button_start); // attaches ezButton object to bin sensor pin
ezButton button_stop_debounced(button_stop); // attaches ezButton object to bin sensor pin

auto led_red = JLed(indicate_red);
auto led_yellow = JLed(indicate_yellow);
auto led_green = JLed(indicate_green);

int DEBUG = 0;

#define FULLOFF 0
#define BOOTING 1
#define WAITING 2
#define STARTING 4
#define RUNNING 8
#define RUNDOWN 16
#define CLEANING 32
#define BINFULL 64


int STATE_E1 = BOOTING;
int STATE_PREV_E1 = FULLOFF;

#define CLEAN_SPINDOWN 0
#define CLEAN_START 1
#define CLEAN_RUN 2

unsigned long RUNTIME_E1_SPINDOWN = (1000UL * 15); // 15sec
unsigned long RUNTIME_E1_CLEAN = (1000UL * 15); // 15sec

int CLEAN_STATE = CLEAN_START;

unsigned long TIME_E1_CLEAN = 0;
unsigned long TIME_0_E1_CLEAN = 0;


void setup() {
  sensor_binfull_debounced.setDebounceTime(1000 * 15); // bin full sensor debounce time of 15 seconds
	button_start_debounced.setDebounceTime(100);
	button_stop_debounced.setDebounceTime(100);

  pinMode(sensor_binfull, INPUT);     // Active High
  pinMode(button_start, INPUT);       // Active High
  pinMode(button_stop, INPUT);        // Active High
  pinMode(control_extractor, OUTPUT); // Active Low
  pinMode(control_cleaner, OUTPUT);   // Active Low
  // pinMode(indicate_red, OUTPUT);      // Active High
  // inMode(indicate_yellow, OUTPUT);   // Active High
  // inMode(indicate_green, OUTPUT);    // Active High

  // Turn off the extractor and cleaner to start.
  digitalWrite(control_cleaner, HIGH);
  digitalWrite(control_extractor, HIGH);
}

int SM_S_BOOTING() {
	// This state is a placeholder for any steps we want to take
	// before transitioning to ready. For example, running a cleaning cycle.
	return WAITING;
}

int SM_S_WAITING() {
	if (sensor_binfull_debounced.isPressed()) {
		return BINFULL;
	}
	if (button_start_debounced.isPressed()) {
		return STARTING;
	}
	return WAITING;
}

int SM_S_STARTING() {
	led_green.On().Update();
	digitalWrite(control_extractor, LOW);
	TIME_0_E1 = millis();
	return RUNNING;
}

int SM_S_RUNNING() {
	if (sensor_binfull_debounced.isPressed()) {
		led_red.Blink(250, 250).Forever();
		if (TIME_E1 - TIME_0_E1 < RUNTIME_E1_MIN) {
			led_green.Blink(250, 250).Forever();
			return RUNDOWN;
		}
		return CLEANING;
	}
	if (button_stop_debounced.isPressed()) {
		if (TIME_E1 - TIME_0_E1 < RUNTIME_E1_MIN) {
			led_green.Blink(250, 250).Forever();
			return RUNDOWN;
		}
		return CLEANING;
	}
	if (TIME_E1 - TIME_0_E1 > RUNTIME_E1_MAX) {
		return CLEANING;
	}
	return RUNNING;
}

int SM_S_RUNDOWN() {
		if (TIME_E1 - TIME_0_E1 > RUNTIME_E1_MIN) {
			return CLEANING;
		}
		return RUNDOWN;
}

int SM_S_CLEANING() {
	switch (CLEAN_STATE) {
		case CLEAN_START:
			// Initialised phase, start the timer
			TIME_0_E1_CLEAN = millis();
			// Indicate cleaning state
			led_green.Off().Update();
			led_yellow.Blink(250, 250).Forever();
			// Turn off extractor
			digitalWrite(control_extractor, HIGH);
			CLEAN_STATE = CLEAN_SPINDOWN;
		break;
		case CLEAN_SPINDOWN:
			// Check if we've waited for the extractor to spindown
			if (TIME_E1_CLEAN - TIME_0_E1_CLEAN > RUNTIME_E1_SPINDOWN) {
				// Reset the timer start
				TIME_0_E1_CLEAN = millis();
				// Turn on cleaner
				led_yellow.On().Update();
				digitalWrite(control_cleaner, LOW);
				// Transition state
				CLEAN_STATE = CLEAN_RUN;
			}
		case CLEAN_RUN:
			// Check if cleaner has been running long enough
			if (TIME_E1_CLEAN - TIME_0_E1_CLEAN > RUNTIME_E1_CLEAN) {
				// Reset cleaner state for next run
				CLEAN_STATE = CLEAN_START;
				// Turn off cleaner
				led_yellow.Off().Update();
				digitalWrite(control_cleaner, HIGH);
				return WAITING;
			}
		break;
	}

	return CLEANING;
}

int SM_S_BINFULL() {
	// Turn off everything, indicate bin full
	digitalWrite(control_cleaner, HIGH);
	digitalWrite(control_extractor, HIGH);
	led_red.On().Update();
	return BINFULL;
}

void SM_E1() {

	STATE_PREV_E1 = STATE_E1;

	switch (STATE_E1) {
		case BOOTING:
			STATE_E1 = SM_S_BOOTING();
		break;
		case WAITING:
			STATE_E1 = SM_S_WAITING();
		break;
		case STARTING:
			STATE_E1 = SM_S_STARTING();
		break;
		case RUNNING:
			STATE_E1 = SM_S_RUNNING();
		break;
		case RUNDOWN:
			STATE_E1 = SM_S_RUNDOWN();
		break;
		case CLEANING:
			STATE_E1 = SM_S_CLEANING();
		break;
		case BINFULL:
			STATE_E1 = SM_S_BINFULL();
		break;
		default:
		break;
	}
}

void loop() {
	if (DEBUG) {
		if (STATE_E1 != STATE_PREV_E1) {
			Serial.println(STATE_E1);
		}
	}

	TIME_E1 = millis();
	TIME_E1_CLEAN = millis();

	led_red.Update();
	led_yellow.Update();
	led_green.Update();

	SM_E1();
}
