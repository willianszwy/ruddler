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
  int     forcaMax;
  int     deadzone;
  int     alphaX100;
};

Config config = { EEPROM_MAGIC, 15000, 150, 20 };

HX711_ADC scale(DT_PIN, SCK_PIN);

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, 0, 0,
  true, false, false, false, false, false,
  false, false, false, false, false);

float         emaValor      = 0;
unsigned long ultimoEnvio   = 0;
unsigned long ultimoJoystick= 0;
unsigned long contadorHz    = 0;
unsigned long ultimoTare    = 0;

String serialBuf = "";

void salvarConfig() { EEPROM.put(0, config); }

void carregarConfig() {
  Config temp;
  EEPROM.get(0, temp);
  if (temp.magic    == EEPROM_MAGIC  &&
      temp.forcaMax  > 0             &&
      temp.forcaMax  <= 50000        &&
      temp.deadzone  >= 0            &&
      temp.deadzone  < temp.forcaMax &&
      temp.alphaX100 > 0             &&
      temp.alphaX100 <= 100) {
    config = temp;
  }
}

void processarComando(String cmd) {
  cmd.trim();
  if      (cmd.startsWith("MAX:"))   { config.forcaMax  = cmd.substring(4).toInt();  salvarConfig(); }
  else if (cmd.startsWith("DEAD:"))  { config.deadzone  = cmd.substring(5).toInt();  salvarConfig(); }
  else if (cmd.startsWith("ALPHA:")) { config.alphaX100 = cmd.substring(6).toInt();  salvarConfig(); }
  else if (cmd == "TARE")            { scale.tareNoDelay(); emaValor = 0; }
}

void lerSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processarComando(serialBuf);
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_TARE, INPUT_PULLUP);

  carregarConfig();

  Joystick.setXAxisRange(-32767, 32767);
  Joystick.begin();

  scale.begin();
  scale.start(500);
  scale.setCalFactor(54.8);
}

void loop() {
  lerSerial();

  if (digitalRead(BTN_TARE) == LOW && millis() - ultimoTare > 500) {
    scale.tareNoDelay();
    emaValor   = 0;
    ultimoTare = millis();
  }

  if (scale.update()) {
    contadorHz++;

    float alpha   = config.alphaX100 / 100.0f;
    float leitura = scale.getData();
    emaValor = (alpha * leitura) + ((1.0f - alpha) * emaValor);
  }

  // envia joystick a 100Hz fixo, independente do HX711
  unsigned long agora = millis();
  if (agora - ultimoJoystick >= 10) {
    int f = constrain((int)emaValor, -config.forcaMax, config.forcaMax);
    if (f > -config.deadzone && f < config.deadzone) f = 0;

    int valor = map(f, -config.forcaMax, config.forcaMax, -32767, 32767);
    Joystick.setXAxis(valor);
    ultimoJoystick = agora;
  }

  if (agora - ultimoEnvio >= 1000) {
    Serial.print("F:");      Serial.print((int)emaValor);
    Serial.print(",A:");     Serial.print(map(constrain((int)emaValor, -config.forcaMax, config.forcaMax), -config.forcaMax, config.forcaMax, -32767, 32767));
    Serial.print(",MAX:");   Serial.print(config.forcaMax);
    Serial.print(",DEAD:");  Serial.print(config.deadzone);
    Serial.print(",ALPHA:"); Serial.print(config.alphaX100);
    Serial.print(",HZ:");    Serial.println(contadorHz);
    contadorHz  = 0;
    ultimoEnvio = agora;
  }
}
