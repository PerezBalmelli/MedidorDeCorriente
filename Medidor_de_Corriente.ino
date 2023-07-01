#include "SevSeg.h"
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

SevSeg sevseg;
#define SENSOR_PIN 35

char incomingChar;
String mensaje = "";
double newValue = -1;
BluetoothSerial SerialBT;
TaskHandle_t Task1;

int mVperAmp = 100;  // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
double Voltage = 0;
double Amps = 0;
uint32_t start_time = 0;
uint32_t start_time2 = 0;

int counter = 0;  //Contador de cuantas mediciones va
double AmpsSum = 0;
double AmpsProm = 0;
double AmpsPromArray[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

double valorInferior = 14;
double valorMuyInferior = 13;
double valorSuperior = 16;
double valorMuySuperior = 17;

double valorInferiorP = 14.5;
double valorMuyInferiorP = 14;
double valorSuperiorP = 15.5;
double valorMuySuperiorP = 16;

void turnOffLED() {
  digitalWrite(21, LOW);
}

void getValuesBT() {
  SerialBT.println("Amps: ");
  SerialBT.println(Amps);
  SerialBT.println("Amps promedios: ");
  SerialBT.println(AmpsProm);
}

void setVariable() {
  if (mensaje == "PMS") {
    valorMuySuperiorP = newValue;
  } else if (mensaje == "PMI") {
    valorMuyInferiorP = newValue;
  } else if (mensaje == "PS") {
    valorSuperiorP = newValue;
  } else if (mensaje == "PI") {
    valorInferiorP = newValue;
  } else if (mensaje == "VMS") {
    valorMuySuperior = newValue;
  } else if (mensaje == "VMI") {
    valorMuyInferior = newValue;
  } else if (mensaje == "VS") {
    valorSuperior = newValue;
  } else if (mensaje == "VI") {
    valorInferior = newValue;
  }
}

void valorExtricto() {
  if (Amps < valorMuyInferior or Amps > valorMuySuperior) {
    SerialBT.println("Valor extremo trasgredido: ");
    SerialBT.println(Amps);
  }
  if (AmpsProm < valorMuyInferiorP or AmpsProm > valorMuySuperiorP) {
    SerialBT.println("Valor promedio extremo tragredido: ");
    SerialBT.println(AmpsProm);
  }
}

// void BTLoop(void* parameter) {
//   uint32_t start_time2 = millis();
//   while (true) {
//     if (SerialBT.available()) {
//       getMessage();
//       Serial.println("Mensaje recibido");
//       Serial.println(mensaje);
//       if (mensaje
//           == "LED") {
//         turnOffLED();
//         Serial.println("prueba!");
//       } else if (mensaje
//                  == "GV") {
//         getValuesBT();
//       } else {
//         setVariable();
//       }
//     }
//     if ((millis() - start_time2) > 1000) {
//       valorExtricto();
//       start_time2 = millis();
//     }
//   }
// }

void getMessage() {
  mensaje = "";
  String Svalue = "";
  incomingChar = 'a';
  while (SerialBT.available() && incomingChar != '_') {
    Serial.println(incomingChar != '_');
    char incomingChar = SerialBT.read();
    // Serial.println(incomingChar);
    if (incomingChar != '\n' && incomingChar != '_') {
      mensaje += String(incomingChar);
      Serial.println(mensaje);
    } else {
      break;
    }
    if (!SerialBT.available()) {
      mensaje = mensaje.substring(0, mensaje.length() - 1);
    }
  }
  while (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n' && incomingChar != '_') {
      Svalue += String(incomingChar);
      Serial.println(Svalue);
    }
  }
  if (Svalue != "") {
    Svalue = Svalue.substring(0, Svalue.length() - 1);
    newValue = Svalue.toDouble();
  }
}

void IRAM_ATTR isr() {
  digitalWrite(21, LOW);
};

void setup() {

  analogReadResolution(9);

  SerialBT.begin("SensorCorriente");  //Bluetooth device name
  xTaskCreatePinnedToCore(
    AmpLoop,          /* Task function. */
    "AmpLoop",        /* name of task. */
    4096,             /* Stack size of task */
    NULL,             /* parameter of the task */
    tskIDLE_PRIORITY, /* priority of the task */
    &Task1,           /* Task handle to keep track of created task */
    0);               /* Core */


  pinMode(22, OUTPUT);  //LEDS
  pinMode(21, OUTPUT);

  pinMode(33, INPUT_PULLUP);  //BOTON
  attachInterrupt(33, isr, FALLING);

  byte numDigits = 4;
  byte digitPins[] = { 14, 15, 2, 5 };
  byte segmentPins[] = { 12, 4, 19, 26, 27, 13, 18, 25 };
  bool resistorsOnSegments = false;    // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE;  // See README.md for options
  bool updateWithDelays = true;        // Default 'false' is Recommended
  bool leadingZeros = false;           // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false;        // Use 'true' if your decimal point doesn't exist or isn't connected. Then, you only need to specify 7 segmentPins[]

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setNumberF(Amps, 3);
}


void AmpLoop(void* parameter) {
  while (true) {
    if ((millis() - start_time) > 1000) {
      Amps = getAmps();
      sevseg.setNumber(Amps);
      ledNormal() 
      getAmpsProm();
      start_time = millis();
    }

    sevseg.refreshDisplay();
  }
}
void loop() {
  Serial.begin(115200);
  if (SerialBT.available()) {
    getMessage();
    Serial.println("Mensaje recibido");
    // Serial.println(mensaje.length());
    if (mensaje == "LED") {
      turnOffLED();
      Serial.println("prueba!");
    } else if (mensaje
               == "GV") {
      getValuesBT();
    } else {
      setVariable();
    }
  }
  if ((millis() - start_time2) > 1000) {
    valorExtricto();
    start_time2 = millis();
  }
  //Serial.println(AmpsProm);
}
void getAmpsProm() {

  AmpsSum = AmpsSum + Amps;
  AmpsSum = AmpsSum - AmpsPromArray[counter];
  AmpsPromArray[counter] = Amps;
  AmpsProm = AmpsSum / 10;
  counter = counter + 1;
  if (counter == 10) {
    counter = 0;
  }
  ledExtremo();
}


void ledNormal() {
  if (Amps < valorInferior or Amps > valorSuperior) {
    digitalWrite(22, HIGH);
  } else if (digitalRead(22) == HIGH) {
    digitalWrite(22, LOW);
    digitalWrite(21, HIGH);
  }
}

void ledExtremo() {
  if (AmpsProm < valorInferiorP or AmpsProm > valorSuperiorP) {
    digitalWrite(22, HIGH);
  } else if (digitalRead(22) == HIGH) {
    digitalWrite(22, LOW);
    digitalWrite(21, HIGH);
  }
}

double getAmps() {
  double result;
  int readValue;     // value read from the sensor
  readValue = analogRead(SENSOR_PIN);

  Voltage = ((readValue)*3.3) / 512.0;  //ESP32 ADC resolution 512
  result = ((Voltage * 1000) / mVperAmp);
  result= round(result *10) / 10;

  return result;
}
