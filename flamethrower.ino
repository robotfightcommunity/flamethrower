#include <Servo.h>

// input pin, that controls safety
#define PWM_PIN_SAFETY 2
// input pin controls whole process
#define PWM_PIN_INPUT 3

// output safety
#define OUTPUT_SAFETY 7
// output pin power
#define OUTPUT_PIN_SERVO 9
// output for ignition
#define OUTPUT_PIN_IGNITION 10

#define LOW_LOW_BOUNDARY 900
#define LOW_HIGH_BOUNDARY 1100
#define HIGH_LOW_BOUNDARY 1900
#define HIGH_HIGH_BOUNDARY 2100

// ignition delay in milliseconds
#define IGNITION_DELAY 500
// ignition power in degrees of servo position
#define IGNITION_SERVO_POSITION 5

#define FLAME_SERVO_START 5  // degrees
#define FLAME_SERVO_END 25  // degrees
#define FLAME_SERVO_UP_STEP 5  // degress
#define FLAME_SERVO_DOWN_STEP 1  // degrees
#define FLAME_SERVO_UP_DELAY 200  // milliseconds
#define FLAME_SERVO_DOWN_DELAY 300  // milliseconds

// state machine positions
#define INITIAL 0
#define READY 1
#define IGNITION 2
#define MOVING_UP_FAST 3
#define MOVING_DOWN_SLOW 4

volatile unsigned long pwm_safety_start = 0;
volatile boolean pwm_safety_throttle_signal = false;
volatile unsigned int safety_throttle_len = 0;
volatile unsigned long last_signal_safety_time = 0;

volatile unsigned long pwm_input_start = 0;
volatile boolean pwm_input_throttle_signal = false;
volatile unsigned int input_throttle_len = 0;
volatile unsigned long last_signal_input_time = 0;

boolean is_weapon_on = false;
boolean is_input_on = false;

unsigned long current_time = 0;

// servo for controlling the power
Servo power_servo;

// current state position
unsigned int current_state = INITIAL;

unsigned int current_servo_position = 0;


void setup() {
    pinMode(OUTPUT_SAFETY, OUTPUT);
    pinMode(OUTPUT_PIN_IGNITION, OUTPUT);
    power_servo.attach(OUTPUT_PIN_SERVO);

    digitalWrite(OUTPUT_SAFETY, LOW);
    digitalWrite(OUTPUT_PIN_IGNITION, LOW);
    power_servo.write(0);

    attachInterrupt(digitalPinToInterrupt(PWM_PIN_SAFETY), calc_safety, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PWM_PIN_INPUT), calc_input, CHANGE);
}


void loop() {
    current_time = micros();
    if (pwm_safety_throttle_signal) {
        if(safety_throttle_len >= HIGH_LOW_BOUNDARY) {
            is_weapon_on = true;
        } else {
            is_weapon_on = false;
        }

        pwm_safety_throttle_signal = false;
    }

    if (pwm_input_throttle_signal) {
        if(input_throttle_len >= HIGH_LOW_BOUNDARY) {
            is_input_on = true;
        } else {
            is_input_on = false;
        }

        pwm_input_throttle_signal = false;
    }

    if((current_state == INITIAL) && is_weapon_on) {
        to_ready();
    } else if((current_state == READY) && !is_weapon_on) {
        to_initial();
    } else if((current_state == READY) && is_input_on) {
        start_ignition();
    } else if((current_state == IGNITION) && !is_input_on) {
        to_ready();
    } else if((current_state == MOVING_UP_FAST) && is_input_on) {
        step_up();
    } else if((current_state == MOVING_UP_FAST) && !is_input_on) {
        to_ready();
    } else if((current_state == MOVING_DOWN_SLOW) && is_input_on) {
        step_down();
    } else if((current_state == MOVING_DOWN_SLOW) && !is_input_on) {
        to_ready();
    }
}


void step_up() {
    if (current_servo_position <= FLAME_SERVO_END) {
        current_servo_position += FLAME_SERVO_UP_STEP;
        power_servo.write(current_servo_position);
        delay(FLAME_SERVO_UP_DELAY);
    } else {
        current_state = MOVING_DOWN_SLOW;
    }
}


void step_down() {
    if(current_servo_position >= FLAME_SERVO_START) {
        current_servo_position -= FLAME_SERVO_DOWN_STEP;
        power_servo.write(current_servo_position);
        delay(FLAME_SERVO_DOWN_DELAY);
    } else {
        current_state = MOVING_UP_FAST;
    }
}


void to_initial() {
    current_servo_position = 0;
    current_state == INITIAL;
    power_servo.write(current_servo_position);
    digitalWrite(OUTPUT_PIN_IGNITION, LOW);
    digitalWrite(OUTPUT_SAFETY, LOW);
}


void to_ready() {
    current_servo_position = 0;
    current_state = READY;
    power_servo.write(current_servo_position);
    digitalWrite(OUTPUT_PIN_IGNITION, LOW);
    digitalWrite(OUTPUT_SAFETY, HIGH);
}


void start_ignition() {
    current_state = IGNITION;
    current_servo_position = IGNITION_SERVO_POSITION;
    digitalWrite(OUTPUT_PIN_IGNITION, HIGH);
    power_servo.write(current_servo_position);
    delay(IGNITION_DELAY);
    current_state = MOVING_UP_FAST;
    digitalWrite(OUTPUT_PIN_IGNITION, LOW);
}


void calc_safety() {
    long current_micros = micros();

    if (digitalRead(PWM_PIN_SAFETY) == HIGH) {
        pwm_safety_start = current_micros;
    } else {
        if (pwm_safety_start && (pwm_safety_throttle_signal == false)) {
            safety_throttle_len = (int)(current_micros - pwm_safety_start);
            pwm_safety_start = 0;

            pwm_safety_throttle_signal = true;
            last_signal_safety_time = current_micros;
        }
    }
}


void calc_input() {
    long current_micros = micros();

    if (digitalRead(PWM_PIN_INPUT) == HIGH) {
        pwm_input_start = current_micros;
    } else {
        if (pwm_input_start && (pwm_input_throttle_signal == false)) {
            input_throttle_len = (int)(current_micros - pwm_input_start);
            pwm_input_start = 0;

            pwm_input_throttle_signal = true;
            last_signal_input_time = current_micros;
        }
    }
}


boolean in_boundary(int value, int low, int high) {
    return value > low && value < high;
}
