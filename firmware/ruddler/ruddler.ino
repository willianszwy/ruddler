#include <HX711_ADC.h>
#include <Joystick.h>
#include <EEPROM.h>

#define DT_PIN   4
#define SCK_PIN  5
#define BTN_TARE 10

// HX711 RATE pin connected directly to VCC (80Hz mode)

#define EEPROM_MAGIC 0xAB

struct Config {
  uint8_t magic;
  int     forceMax;
  int     deadzone;
  int     alphaX100;
};

Config config = { EEPROM_MAGIC, 15000, 150, 20 };

HX711_ADC scale(DT_PIN, SCK_PIN);

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, 0, 0,
  true, false, false, false, false, false,
  false, false, false, false, false);

float         emaValue      = 0;
unsigned long lastJoystick  = 0;
unsigned long lastSerial    = 0;
unsigned long lastHz        = 0;
unsigned long hzCounter     = 0;
unsigned long lastTare      = 0;
int           currentHz     = 0;

String serialBuf = "";

void saveConfig() { EEPROM.put(0, config); }

void loadConfig() {
  Config temp;
  EEPROM.get(0, temp);
  if (temp.magic    == EEPROM_MAGIC   &&
      temp.forceMax  > 0              &&
      temp.forceMax  <= 50000         &&
      temp.deadzone  >= 0             &&
      temp.deadzone  < temp.forceMax  &&
      temp.alphaX100 > 0              &&
      temp.alphaX100 <= 100) {
    config = temp;
  }
}

void processCommand(String cmd) {
  cmd.trim();
  if      (cmd.startsWith("MAX:"))   { config.forceMax  = cmd.substring(4).toInt(); saveConfig(); }
  else if (cmd.startsWith("DEAD:"))  { config.deadzone  = cmd.substring(5).toInt(); saveConfig(); }
  else if (cmd.startsWith("ALPHA:")) { config.alphaX100 = cmd.substring(6).toInt(); saveConfig(); }
  else if (cmd == "TARE")            { scale.tareNoDelay(); emaValue = 0; }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(serialBuf);
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_TARE, INPUT_PULLUP);

  loadConfig();

  Joystick.setXAxisRange(-32767, 32767);
  Joystick.begin();

  scale.begin();
  scale.start(500);
  scale.setCalFactor(54.8);
}

void loop() {
  readSerial();

  if (digitalRead(BTN_TARE) == LOW && millis() - lastTare > 500) {
    scale.tareNoDelay();
    emaValue = 0;
    lastTare = millis();
  }

  if (scale.update()) {
    hzCounter++;
    float alpha   = config.alphaX100 / 100.0f;
    float reading = scale.getData();
    emaValue = (alpha * reading) + ((1.0f - alpha) * emaValue);
  }

  unsigned long now = millis();

  // send joystick at 100Hz
  if (now - lastJoystick >= 10) {
    int f = constrain((int)emaValue, -config.forceMax, config.forceMax);
    if (f > -config.deadzone && f < config.deadzone) f = 0;

    int axis = map(f, -config.forceMax, config.forceMax, -32767, 32767);
    Joystick.setXAxis(axis);
    lastJoystick = now;
  }

  // measure Hz every second
  if (now - lastHz >= 1000) {
    currentHz = hzCounter;
    hzCounter = 0;
    lastHz    = now;
  }

  // send data to dashboard at 20Hz, only if serial is connected and buffer has space
  if (now - lastSerial >= 50 && Serial && Serial.availableForWrite() > 60) {
    int f = constrain((int)emaValue, -config.forceMax, config.forceMax);
    if (f > -config.deadzone && f < config.deadzone) f = 0;
    int axis = map(f, -config.forceMax, config.forceMax, -32767, 32767);

    Serial.print("F:");      Serial.print((int)emaValue);
    Serial.print(",A:");     Serial.print(axis);
    Serial.print(",MAX:");   Serial.print(config.forceMax);
    Serial.print(",DEAD:");  Serial.print(config.deadzone);
    Serial.print(",ALPHA:"); Serial.print(config.alphaX100);
    Serial.print(",HZ:");    Serial.println(currentHz);
    lastSerial = now;
  }
}
