#include <HX711.h>
#include <Joystick.h>
#include <EEPROM.h>

#define DT_PIN   4
#define SCK_PIN  5
#define BTN_TARE 10

#define EEPROM_MAGIC 0xAB

struct Config {
  uint8_t magic;
  int     forcaMax;
  int     deadzone;
  int     alphaX100;
};

Config config = { EEPROM_MAGIC, 15000, 150, 20 };

HX711 scale;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, 0, 0,
  true, false, false, false, false, false,
  false, false, false, false, false);

float emaValor    = 0;
int   ultimoValor = 0;
unsigned long ultimoEnvio = 0;

void salvarConfig() {
  EEPROM.put(0, config);
}

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
  if (cmd.startsWith("MAX:")) {
    config.forcaMax = cmd.substring(4).toInt();
    salvarConfig();
  } else if (cmd.startsWith("DEAD:")) {
    config.deadzone = cmd.substring(5).toInt();
    salvarConfig();
  } else if (cmd.startsWith("ALPHA:")) {
    config.alphaX100 = cmd.substring(6).toInt();
    salvarConfig();
  } else if (cmd == "TARE") {
    scale.tare();
    emaValor = 0;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_TARE, INPUT_PULLUP);

  carregarConfig();

  Joystick.setXAxisRange(-32767, 32767);
  Joystick.begin();

  scale.begin(DT_PIN, SCK_PIN);
  delay(2000);
  scale.tare();
  scale.set_scale(54.8);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    processarComando(cmd);
  }

  if (digitalRead(BTN_TARE) == LOW) {
    scale.tare();
    emaValor = 0;
    delay(500);
  }

  if (scale.is_ready()) {
    float alpha  = config.alphaX100 / 100.0f;
    float leitura = scale.get_units(1);
    emaValor = (alpha * leitura) + ((1.0f - alpha) * emaValor);

    int f = constrain((int)emaValor, -config.forcaMax, config.forcaMax);
    if (f > -config.deadzone && f < config.deadzone) f = 0;

    int valor = map(f, -config.forcaMax, config.forcaMax, -32767, 32767);

    if (abs(valor - ultimoValor) > 10) {
      Joystick.setXAxis(valor);
      ultimoValor = valor;
    }

    unsigned long agora = millis();
    if (agora - ultimoEnvio >= 50) {
      Serial.print("F:");  Serial.print((int)emaValor);
      Serial.print(",A:"); Serial.print(valor);
      Serial.print(",MAX:"); Serial.print(config.forcaMax);
      Serial.print(",DEAD:"); Serial.print(config.deadzone);
      Serial.print(",ALPHA:"); Serial.println(config.alphaX100);
      ultimoEnvio = agora;
    }
  }
}
