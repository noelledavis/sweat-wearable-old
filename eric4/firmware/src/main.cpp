#include <Arduino.h>
#include <stdarg.h>
#include <stdint.h>

#define BAUDRATE 115200

// Analog channel names
#define CURRENT_CH2 A0
#define CURRENT_CH1 A1
#define VOLTAGE_CH1 A2
#define VOLTAGE_CH2 A3
#define VOLTAGE_CH3 A4
#define VOLTAGE_CH4 A5
#define TEMPERATURE_SENSOR A7

// Bluetooth RTS/CTS names
//#define PROCESSOR_CTS PD6
//#define PROCESSOR_RTS PD7

#define INDICATOR_PIN 5

#define PROCESSOR_CTS 6
#define PROCESSOR_RTS 7


#define AVERAGING_WINDOW 1024
#define AVERAGING_WINDOW_DIVIDE_BY 0.0009765625

#define TEN_BIT_FIVE_VOLT_ADC_SCALEFACTOR 0.0048828125

#define AT_BUFFER_SIZE 64
#define AT_BUFFER_MASK 0x3F

static char AT_BUFFER[64];

int printf (const char* str, ...) {

    char nextChar;
  
    va_list argp;
    va_start(argp, str);
  
    int i = 0;
  
    while (*(str+i) != '\0') {
	    if (*(str + i) != '%') {
            Serial.print(*(str + i));
            i += 1;
        } else {

            nextChar = *(str + i + 1);
      
            if (nextChar != '\0') {
                switch (nextChar) {
                    case 'd':
                        Serial.print(va_arg(argp, int));
                        i += 2;
                        break;
                    case 'f':
                        Serial.print(va_arg(argp, double), 6);
                        i += 2;
                        break;
                    case 's':
                        Serial.print(va_arg(argp, char*));
                        i += 2;
                        break;
                    default:
                        Serial.print(*(str + i));
                        i += 1;
                        break;
                }
            }
        }
    }
  
    return 0;
}


static inline double readAverage (int pin) {

    uint32_t accumulator = 0;
    uint16_t adc_reading;
    double rawVoltage;

    for (int i = 0; i < AVERAGING_WINDOW; ++i) {

        adc_reading = analogRead(pin);
        accumulator += adc_reading;
    }

    // need to scale ADC reading to a voltage
    rawVoltage = accumulator * TEN_BIT_FIVE_VOLT_ADC_SCALEFACTOR;
    return rawVoltage * AVERAGING_WINDOW_DIVIDE_BY;
}


static inline double compensateCurrentCh1 (double value) {
    // customize this later
    return (2.5 - value);
}

static inline double compensateCurrentCh2 (double value) {
    // customize this later
    return (2.5 - value);
}


static inline double compensateVoltageCh1 (double value) {
    // customize this later
    return 1e3 * 0.2 * (value - 2.5);
}

static inline double compensateVoltageCh2 (double value) {
    // customize this later
    return 1e3 * 0.2 * (value - 2.5);
}

static inline double compensateVoltageCh3 (double value) {
    // customize this later
    return 1e3 * 0.2 * (value - 2.5);
}

static inline double compensateVoltageCh4 (double value) {
    // customize this later
    return 1e3 * 0.2 * (value - 2.5);
}

void setup () {
    Serial.begin(BAUDRATE);

    pinMode(CURRENT_CH2, INPUT);
    pinMode(CURRENT_CH1, INPUT);
    pinMode(VOLTAGE_CH1, INPUT);
    pinMode(VOLTAGE_CH2, INPUT);
    pinMode(VOLTAGE_CH3, INPUT);
    pinMode(VOLTAGE_CH4, INPUT);
    pinMode(TEMPERATURE_SENSOR, INPUT);

    pinMode(PROCESSOR_RTS, OUTPUT);
    pinMode(PROCESSOR_CTS, INPUT);

    pinMode(INDICATOR_PIN, OUTPUT);

    analogReference(DEFAULT);
}

int main (void) {

    static double current_ch1_reading;
    static double current_ch2_reading;
    static double voltage_ch1_reading;
    static double voltage_ch2_reading;
    static double voltage_ch3_reading;
    static double voltage_ch4_reading;
    static double temperature_reading;



    static double current_ch1_raw;
    static double current_ch2_raw;
    static double voltage_ch1_raw;
    static double voltage_ch2_raw;
    static double voltage_ch3_raw;
    static double voltage_ch4_raw;
    static double temperature_raw;

    init ();
    setup ();

    digitalWrite(PROCESSOR_RTS, LOW);

    //digitalWrite(INDICATOR_PIN, HIGH);
    //digitalWrite(PROCESSOR_RTS, LOW);
    //waitForBypassMode();
    //digitalWrite(INDICATOR_PIN, LOW);

    for (;;) {
        
        // read each channel, then compensate to the correct value
        current_ch1_raw = readAverage(CURRENT_CH1);
        current_ch1_reading = compensateCurrentCh1(current_ch1_raw);

        current_ch2_raw = readAverage(CURRENT_CH2);
        current_ch2_reading = compensateCurrentCh2(current_ch2_raw);

        voltage_ch1_raw = readAverage(VOLTAGE_CH1);
        voltage_ch1_reading = compensateVoltageCh1(voltage_ch1_raw);

        voltage_ch2_raw = readAverage(VOLTAGE_CH2);
        voltage_ch2_reading = compensateVoltageCh2(voltage_ch2_raw);

        voltage_ch3_raw = readAverage(VOLTAGE_CH3);
        voltage_ch3_reading = compensateVoltageCh3(voltage_ch3_raw);

        voltage_ch4_raw = readAverage(VOLTAGE_CH4);
        voltage_ch4_reading = compensateVoltageCh4(voltage_ch4_raw);


        temperature_raw = readAverage(TEMPERATURE_SENSOR);
        temperature_reading = temperature_raw; // remove this later

        printf ("%f,%f,%f,%f,%f,%f,%f\n",
                    current_ch1_reading,
                    current_ch2_reading,
                    voltage_ch1_reading,
                    voltage_ch2_reading,
                    voltage_ch3_reading,
                    voltage_ch4_reading,
                    temperature_reading);
        
    }
}
