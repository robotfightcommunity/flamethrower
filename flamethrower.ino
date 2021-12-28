// input pin, that controls ignition
#define PWM_PIN_IN_IGNITION 3
// input pin controls power
#define PWM_PIN_IN_POWER 2

// output pin ignition
#define OUTPUT_PIN_IGNITION 8
// output pin power
#define OUTPUT_PIN_POWER 9

#define LOW_LOW_BOUNDARY 900
#define LOW_HIGH_BOUNDARY 1100
#define HIGH_LOW_BOUNDARY 1900
#define HIGH_HIGH_BOUNDARY 2100

// timeout on enable safety if signal was lost
#define NO_SIGNAL_SAFETY_TIMEOUT 50000

volatile unsigned long pwm_in_ignition_start = 0;
volatile boolean pwm_in_ignition_new_throttle_signal = false;
volatile int ignition_throttle_len = 0;
volatile long last_signal_ignition_time = 0;

volatile unsigned long pwm_in_power_start = 0;
volatile boolean pwm_in_power_new_throttle_signal = false;
volatile int power_throttle_len = 0;
volatile long last_signal_power_time = 0;

boolean was_ignition_pin_low = false;

long current_time = 0;


void setup() {
    digitalWrite(OUTPUT_PIN_IGNITION, LOW);
    analogWrite(OUTPUT_PIN_POWER, 0);

    pinMode(OUTPUT_PIN_IGNITION, OUTPUT);
    pinMode(OUTPUT_PIN_POWER, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(PWM_PIN_IN_IGNITION), calc_ignition_pwm, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PWM_PIN_IN_POWER), calc_power_pwm, CHANGE);
}


void loop() {
    current_time = micros();
    if (pwm_in_ignition_new_throttle_signal) {
        if(in_boundary(ignition_throttle_len, LOW_LOW_BOUNDARY, LOW_HIGH_BOUNDARY && !was_ignition_pin_low)) {
            was_ignition_pin_low = true;
            digitalWrite(OUTPUT_PIN_IGNITION, LOW);
        } else if (in_boundary(ignition_throttle_len, HIGH_LOW_BOUNDARY, HIGH_HIGH_BOUNDARY) && was_ignition_pin_low) {
            was_ignition_pin_low = false;
            digitalWrite(OUTPUT_PIN_IGNITION, HIGH);
        }

        pwm_in_ignition_new_throttle_signal = false;
    }

    if(pwm_in_power_new_throttle_signal) {
        if(power_throttle_len >= LOW_HIGH_BOUNDARY) {
            int power_value = map(power_throttle_len, LOW_HIGH_BOUNDARY, HIGH_HIGH_BOUNDARY, 0, 255);
            analogWrite(OUTPUT_PIN_POWER, power_value);
        } else {
            analogWrite(OUTPUT_PIN_POWER, 0);
        }
      pwm_in_power_new_throttle_signal = false;
    }

    if(
        current_time - last_signal_power_time > NO_SIGNAL_SAFETY_TIMEOUT
        or current_time - last_signal_ignition_time > NO_SIGNAL_SAFETY_TIMEOUT
    ) {
      analogWrite(OUTPUT_PIN_POWER, 0);
      digitalWrite(OUTPUT_PIN_IGNITION, LOW);
    }
}


void calc_ignition_pwm() {
    long current_micros = micros();

    if (digitalRead(PWM_PIN_IN_IGNITION) == HIGH) {
        pwm_in_ignition_start = current_micros;
    } else {
        if (pwm_in_ignition_start && (pwm_in_ignition_new_throttle_signal == false)) {
            ignition_throttle_len = (int)(current_micros - pwm_in_ignition_start);
            pwm_in_ignition_start = 0;

            pwm_in_ignition_new_throttle_signal = true;
            last_signal_ignition_time = current_micros;
        }
    }
}


void calc_power_pwm() {
    long current_micros = micros();

    if (digitalRead(PWM_PIN_IN_POWER) == HIGH) {
        pwm_in_power_start = current_micros;
    } else {
        if (pwm_in_power_start && (pwm_in_power_new_throttle_signal == false)) {
            power_throttle_len = (int)(current_micros - pwm_in_power_start);
            pwm_in_power_start = 0;

            pwm_in_power_new_throttle_signal = true;
            last_signal_power_time = current_micros;
        }
    }
}


boolean in_boundary(int value, int low, int high) {
    return value > low && value < high;
}
