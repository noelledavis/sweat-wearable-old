#include <Arduino.h>
#include <stdarg.h>
#include <stdint.h>


// These are the pins used to shut down the switching regulator
//  and for PWM, respectively
#define IONTOPHORESIS_SHDN 9
#define IONTOPHORESIS_PWM 10
#define CH1_SENSE_SWITCH 6
#define SENSE_COMMON_MODE_SWITCH 7
#define CH2_SENSE_SWITCH 5
#define IONTOPHORESIS_HIGH_SIDE_SWITCH 3
#define IONTOPHORESIS_LOW_SIDE_SWITCH 8

#define AVERAGING_LENGTH 1024


// These are the pins for the ADC inputs
#define CH1_ADC_PIN A0
#define CH2_ADC_PIN A1
#define CURRENT_SENSE_ADC_PIN A2

#define ADG711_ON LOW
#define ADG711_OFF HIGH

/* IMPLEMENTATION NOTE

    Set the iontophoresis state bits to 0x1 to enable iontophoresis

    Set the iontophoresis state bits to 0x0 to disable iontophoresis
*/
#define IONTOPHORESIS_ENABLE_MASK 0x1
#define IONTOPHORESIS_DISABLE_MASK 0x0




/* IMPLEMENTATION NOTE

    Set the measurement state bits to 0x0 to disable measurement
    
    Set the measurement state bits to 0x1 to enable measurement


*/
#define MEASURE_ENABLE_MASK 0x1
#define MEASURE_DISABLE_MASK 0x0


/* IMPLEMENTATION NOTE

    Set the measure state bits to 0x0 to disable sensor channel sensing
    Set the measure state bits to 0x1 to enable sensor channel sensing

    Values will be continuously output from the device no matter what

*/

#define OPCODE_MASK 0xF0
#define PAYLOAD_MASK 0x0F
#define OPCODE_WIDTH 4
#define PAYLOAD_WIDTH 4



/* IMPLEMENTATION NOTE

    Board behavior will now be much simpler and more automatic

    Board receives only three commands now:
        * OPCODE_ENABLE_IONTOPHORESIS_INCREMENT_BY enables iontophoresis and increments
            the 8-bit register value by the 4-bit immediate. DOES NOT OVERFLOW, the
            current register will max out at 255
        * OPCODE_ENABLE_IONOTPHORESIS_DECREMENT_BY enables iontophoresis and decrements
            the 8-bit register value by the 4-bit immediate. DOES NOT UNDERFLOW, the
            current register will drop only to 0
        * OPCODE_DISABLE_IONTOPHORESIS sets iontophoresis to 0, opens iontophoresis
            switches, closes sensing switches

    Switching behavior:
        * OPCODE_ENABLE* commands will close iontophoresis switches, open sensing
            switches
        * OPCODE_DISABLE_IONOTPHORESIS will open iontophoresis switches, close
            sensing switches

    Data reporting behavior:
    
        Outputs the following values in the following format
        VoltageCh1,VoltageCh2,MeasuredCurrentDeliv,8bitCurrentRegister
        mV,mV,uA,integer

        Terminated by a newline
        
        garbage values denoted by character 'x'

        Will always output all values, though they may be garbage (i.e. sensing
            switches may be open)

*/
#define OPCODE_ENABLE_IONTOPHORESIS_INCREMENT_BY 0x5 
#define OPCODE_ENABLE_IONOTPHORESIS_DECREMENT_BY 0x6
#define OPCODE_DISABLE_IONTOPHORESIS 0x4

int test_printf (const char* str, ...) {

    char nextChar;
    int i = 0;
    va_list argp;

    va_start(argp, str);
  
  
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


void processBluetoothByte (unsigned char fromBluetooth,
                            unsigned char* iontophoresisStatus_pointer,
                            unsigned char* measurementStatus_pointer,
                            unsigned char* iontophoresisValue_pointer) {

    unsigned char opcode = (fromBluetooth & OPCODE_MASK) >> PAYLOAD_WIDTH;
    unsigned char payload = (fromBluetooth & PAYLOAD_MASK);

    unsigned char previousValue = *iontophoresisValue_pointer;

    if (opcode == OPCODE_ENABLE_IONTOPHORESIS_INCREMENT_BY) {

        *iontophoresisStatus_pointer = IONTOPHORESIS_ENABLE_MASK;
        *measurementStatus_pointer = MEASURE_DISABLE_MASK;
        *iontophoresisValue_pointer += payload;

        if (*iontophoresisValue_pointer < previousValue) { // OVERFLOW!!!
            *iontophoresisValue_pointer = 0xFF;
        }    


    } else if (opcode == OPCODE_ENABLE_IONOTPHORESIS_DECREMENT_BY) {

        *iontophoresisStatus_pointer = IONTOPHORESIS_ENABLE_MASK;
        *measurementStatus_pointer = MEASURE_DISABLE_MASK;
        *iontophoresisValue_pointer -= payload;

        if (*iontophoresisValue_pointer > previousValue) { // UNDERFLOW!!!
            *iontophoresisValue_pointer = 0x00;
        }

        if (*iontophoresisValue_pointer == 0x00) {
            *iontophoresisStatus_pointer = IONTOPHORESIS_DISABLE_MASK;
            *measurementStatus_pointer = MEASURE_ENABLE_MASK;
        }

    } else if (opcode == OPCODE_DISABLE_IONTOPHORESIS) {
        *iontophoresisStatus_pointer = IONTOPHORESIS_DISABLE_MASK;
        *measurementStatus_pointer = MEASURE_ENABLE_MASK;
        *iontophoresisValue_pointer = 0x00;

    }
}


void setup (unsigned char* iontophoresisStatus_pointer, 
                    unsigned char* measurementStatus_pointer,
                    unsigned char* iontophoresisCurrent_pointer) {

    Serial.begin(9600);

    *iontophoresisStatus_pointer = IONTOPHORESIS_DISABLE_MASK;
    *measurementStatus_pointer = MEASURE_ENABLE_MASK;
    
    pinMode(IONTOPHORESIS_SHDN, OUTPUT);
    pinMode(IONTOPHORESIS_PWM, OUTPUT);

    pinMode(CH1_SENSE_SWITCH, OUTPUT);
    pinMode(SENSE_COMMON_MODE_SWITCH, OUTPUT);
    pinMode(CH2_SENSE_SWITCH, OUTPUT);
    pinMode(IONTOPHORESIS_HIGH_SIDE_SWITCH, OUTPUT);
    pinMode(IONTOPHORESIS_LOW_SIDE_SWITCH, OUTPUT);

    pinMode(CH1_ADC_PIN, INPUT);
    pinMode(CH2_ADC_PIN, INPUT);
    pinMode(CURRENT_SENSE_ADC_PIN, INPUT);

    // disable iontophoresis for now
    digitalWrite(IONTOPHORESIS_SHDN, HIGH);
    digitalWrite(IONTOPHORESIS_HIGH_SIDE_SWITCH, LOW);

    // disable sensing for now
    digitalWrite(CH1_SENSE_SWITCH, ADG711_OFF);
    digitalWrite(SENSE_COMMON_MODE_SWITCH, ADG711_OFF);
    digitalWrite(CH2_SENSE_SWITCH, ADG711_OFF);
    digitalWrite(IONTOPHORESIS_LOW_SIDE_SWITCH, ADG711_OFF);
    
    *iontophoresisCurrent_pointer = 0;
    analogWrite(IONTOPHORESIS_PWM, 0);
}


inline float compensateCh1 (float rawCh1reading) {
    return 2.5 - rawCh1reading;
}


inline float compensateCh2 (float rawCh2reading) {
    return 2.5 - rawCh2reading;
}

static inline float compensateCurrent (float rawCurrentReading) {
    return 1e6 * rawCurrentReading;
}

int main (void) {

    static int channelOneReading;
    static int channelTwoReading;
    static int currentSenseAmpValue;

    static float channelOneVoltage;
    static float channelTwoVoltage;
    static float currentSenseCurrent;

    static float temporaryChannelOneVoltage;
    static float temporaryChannelTwoVoltage;

    /* Status unsigned char containing iontophoresis PWM current value */
    static unsigned char iontophoresisCurrentValue;

    /* Status unsigned char containing iontophoresis status */
    static unsigned char iontophoresisStatusBits;

    /* Status unsigned char containing measurement status */
    static unsigned char measurementStatusBits;

    static unsigned char serialValue;

    init(); // required Arduino call

    // call setup function to set initial state
    setup(&iontophoresisStatusBits, 
                &measurementStatusBits, 
                &iontophoresisCurrentValue);

    for (;;) {

        while (Serial.available() > 0) {
            serialValue = Serial.read();
            processBluetoothByte(serialValue,
                                &iontophoresisStatusBits,
                                &measurementStatusBits,
                                &iontophoresisCurrentValue);            
        }

        // set switches
        if (iontophoresisStatusBits == IONTOPHORESIS_ENABLE_MASK) {

            // enable iontophoresis, disable sensing
            digitalWrite(IONTOPHORESIS_HIGH_SIDE_SWITCH, HIGH);
            digitalWrite(IONTOPHORESIS_LOW_SIDE_SWITCH, ADG711_ON);

            digitalWrite(CH1_SENSE_SWITCH, ADG711_OFF);
            digitalWrite(CH2_SENSE_SWITCH, ADG711_OFF);
            digitalWrite(SENSE_COMMON_MODE_SWITCH, ADG711_OFF);

            // set current level
            analogWrite(IONTOPHORESIS_PWM, iontophoresisCurrentValue);

        } else {

            // disable iontophoresis, enable sensing
            digitalWrite(IONTOPHORESIS_HIGH_SIDE_SWITCH, LOW);
            digitalWrite(IONTOPHORESIS_LOW_SIDE_SWITCH, ADG711_OFF);

            analogWrite(IONTOPHORESIS_PWM, 0);

            digitalWrite(CH1_SENSE_SWITCH, ADG711_ON);
            digitalWrite(CH2_SENSE_SWITCH, ADG711_ON);
            digitalWrite(SENSE_COMMON_MODE_SWITCH, ADG711_ON);

        }

        // measure and output data
        if (measurementStatusBits == MEASURE_ENABLE_MASK) {
            // measure everything

            channelOneVoltage = 0;
            channelTwoVoltage = 0;
            currentSenseCurrent = 0;


            for (uint16_t i = 0; i < AVERAGING_LENGTH; ++i) {

                channelOneReading = analogRead(CH1_ADC_PIN);
                channelTwoReading = analogRead(CH2_ADC_PIN);
                
                // calculate voltages
                // assuming ADC is 10 bits and ranges from 0V to 5V
                temporaryChannelOneVoltage = 5.0 * channelOneReading / 1024.0;
                temporaryChannelTwoVoltage = 5.0 * channelTwoReading / 1024.0;

                channelOneVoltage += temporaryChannelOneVoltage;
                channelTwoVoltage += temporaryChannelTwoVoltage;
            }

            channelOneVoltage = channelOneVoltage / AVERAGING_LENGTH;
            channelTwoVoltage = channelTwoVoltage / AVERAGING_LENGTH;


            // measure current
            for (int i = 0; i < AVERAGING_LENGTH; ++i) {

                currentSenseAmpValue = analogRead(CURRENT_SENSE_ADC_PIN);
                currentSenseCurrent += (currentSenseAmpValue / 409600.0);

            }

            currentSenseCurrent = currentSenseCurrent / AVERAGING_LENGTH;

            // dump out all values
            test_printf("%f,%f,%f,%d\n",
                    compensateCh1(channelOneVoltage),
                    compensateCh2(channelTwoVoltage),
                    compensateCurrent(currentSenseCurrent),
                    iontophoresisCurrentValue);

        } else {

            // measure current
            currentSenseCurrent = 0;

            for (int i = 0; i < AVERAGING_LENGTH; ++i) {

                currentSenseAmpValue = analogRead(CURRENT_SENSE_ADC_PIN);
                currentSenseCurrent += (currentSenseAmpValue / 409600.0);

            }

            currentSenseCurrent = currentSenseCurrent / AVERAGING_LENGTH;

            // dump out all values
            test_printf("%s,%s,%f,%d\n",
                    "x",
                    "x",
                    compensateCurrent(currentSenseCurrent),
                    iontophoresisCurrentValue);
        }
    }

    return 0;
}

