#include <avr/sleep.h>
#include <Wire.h>

#define NONE 0
#define WAITING 1
#define VOLTAGE 2
#define SHUTDOWN 3

volatile uint8_t command = NONE;
volatile uint8_t seconds = 0;
volatile uint8_t timeout = 0;
volatile bool wakeFromRTC = false;

void setup() {
  initADC();
  initRTC();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode to POWER DOWN mode
  sleep_enable(); // Enable sleep mode, but not going to sleep yet
  
  Wire.begin(8);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  Serial.begin(115200);
  Serial.println("hello");
  pinMode(PIN_PA5, OUTPUT);
  digitalWrite(PIN_PA5, LOW);
}

void loop() {

  cli();
  
  if (command == SHUTDOWN) {
    command = NONE;
    Serial.println("powering down");
    digitalWrite(PIN_PA5, HIGH); // turn off the power to the ESP32
  }

  if (seconds == 60) {
    seconds = 0;
    
    startReadingVoltage();
    double voltage = finishReadingVoltage();

    if (voltage > 3.6) {
      Serial.println("powering up");
      digitalWrite(PIN_PA5, LOW); // turn on the ESP32 after 60 seconds
    }
    else {
      Serial.print("low voltage: ");
      Serial.println(voltage);
    }
  }

  if (command == NONE && !timeout) {
    sei(); // enable interrupts before we sleep
    
    Serial.flush();
    
    sleep_cpu(); // go to sleep until an i2c message comes in or the PIT interrupt wakes us up

    cli(); // disable again because we're using some volatile vars
    
    if (wakeFromRTC) {
        wakeFromRTC = false; // just clear this. don't try and stay awake longer than what's necessary
    }
    else {
      // assume that if we didn't wake from the RTC it was because of i2c, don't go to sleep until we load in the command and finish processing it.
      timeout = 2;
      // NOTE: we're not waking up fast enough, so the onReceive handler gets no data the first time it is called. Easiest workaround I've found is to i2c.scan() in the client before i2c.writeto() 
    }
  }

  sei(); // probably silly, but enable interrupts at the end of the loop
}


void initADC() {
  VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;
  ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV256_gc; // 78kHz clock
  ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;                  // Measure INTREF
  ADC0.CTRLA = ADC_ENABLE_bm;                          // Single, 10-bit
}

/*
double measureVoltage() {
  ADC0.COMMAND = ADC_STCONV_bm;                        // Start conversion
  while (ADC0.COMMAND & ADC_STCONV_bm);                // Wait for completion
  uint16_t reading = ADC0.RES;                         // ADC conversion result
  //uint16_t voltage = 11264/reading;
  //Buffer[0] = voltage/10; Buffer[1]= voltage%10;
  return 1126.4 / reading;
}
*/

void startReadingVoltage() {
  ADC0.COMMAND = ADC_STCONV_bm;                        // Start conversion
}

double finishReadingVoltage() {
  while (ADC0.COMMAND & ADC_STCONV_bm);                // Wait for completion
  uint16_t reading = ADC0.RES;                         // ADC conversion result
  return 1126.4 / reading;
 }

void initRTC() {
  // Initialize RTC
  while (RTC.STATUS > 0) {
    ;                                   // Wait for all register to be synchronized
  }
  
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;    // 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)

  RTC.PITINTCTRL = RTC_PI_bm;           // PIT Interrupt: enabled

  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc // RTC Clock Cycles 16384, resulting in 32.768kHz/32768 = 1Hz
  | RTC_PITEN_bm;                       // Enable PIT counter: enabled
}

ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;          // Clear interrupt flag by writing '1' (required)

  wakeFromRTC = true;
  
  ++seconds;
  if (timeout) {
    --timeout;
  }

  //Serial.print(command);
  //Serial.print(' ');
  //Serial.println(seconds);
}

void receiveHandler(int howMany) {
  //Serial.print("available: ");
  //Serial.print(Wire.available());
  //Serial.print(" howMany:");
  //Serial.println(howMany);
  
  while (Wire.available()) {
    char c = Wire.read();
    Serial.println(c);

    if (c == 'v') { // voltage?
      command = VOLTAGE; // only served noce the client reads
      startReadingVoltage(); // we'll wait for it in requestHandler
    }

    if (c == 's') { // shutdown
      command = SHUTDOWN; // served in the main loop
    }
  }
}

void requestHandler() {
  switch (command) {
      case VOLTAGE: {
        command = NONE;
        char buf[5];
        double voltage = finishReadingVoltage();
        Serial.println(voltage);
        dtostrf(voltage, 5, 3, buf);
        Wire.write(buf, 5);
        break;
      }

      default: {
        break;
      }
  }
}
