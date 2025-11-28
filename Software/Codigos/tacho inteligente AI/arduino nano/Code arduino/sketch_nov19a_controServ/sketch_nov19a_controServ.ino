#include <Servo.h>

Servo servo360;   
Servo servo180;   
String input = "";

#define PIN_GIRO 5
#define PIN_TIRA 9
int pinSwitch = 7;
unsigned long cooldown = 8000;
unsigned long ultimoComando = 0;

#define SEG_METAL 1425
#define SEG_PLASTICO 800
#define SEG_ORG 400
#define SEG_NR 1750  

#define VEL_GIROanti 110
#define VEL_GIROhora 70
#define GIRO_PARA 90

#define POS_TIRAR 155
#define POS_INI 170

void esperarConDeteccion(int tiempo) {
  unsigned long inicio = millis();
  while (millis() - inicio < (unsigned long)tiempo) {
    if (digitalRead(pinSwitch) == LOW) {         
      Serial.println("FIN DE CARRERA DETECTADO");
      servo360.write(90);                        
      while (digitalRead(pinSwitch) == LOW) {
        delay(10);
      }
      delay(500);
      return;
    }
    delay(1);
  }
}

void moverGiro(int tiempoAnti) {
  servo360.attach(PIN_GIRO);
  servo360.write(VEL_GIROanti);
  esperarConDeteccion(tiempoAnti);

  servo360.write(GIRO_PARA);
  delay(400);

  servo360.detach();
  delay(200);
}

void volverInicio() {
  servo360.attach(PIN_GIRO);
  servo360.write(VEL_GIROhora);
  esperarConDeteccion(5000);

  servo360.write(GIRO_PARA);
  delay(300);

 
  servo360.detach();
}

void tirarBasura() {
  servo180.attach(PIN_TIRA);
  servo180.write(POS_TIRAR);
  delay(3000);
  servo180.write(POS_INI);
  delay(500);
  servo180.detach();   
}

void servosMetal() {
  moverGiro(SEG_METAL);
  tirarBasura();
  volverInicio();
}

void servosPlastico() {
  moverGiro(SEG_PLASTICO);
  tirarBasura();
  volverInicio();
}

void servosOrganico() {
  moverGiro(SEG_ORG);
  tirarBasura();
  volverInicio();
}

void servoNoReciclable() {
  moverGiro(SEG_NR);
  tirarBasura();
  volverInicio();
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);

  pinMode(pinSwitch, INPUT_PULLUP);

  // Adjuntamos solo el de 180 al inicio
  servo180.attach(PIN_TIRA);
  servo180.write(POS_INI);
  servo180.detach();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      input.trim();
      procesarComando(input);
      input = "";
    } else {
      input += c;
    }
  }
}

void procesarComando(String cmd) {

  unsigned long ahora = millis();
  if (ahora - ultimoComando < cooldown) {
    Serial.println("IGNORADO: cooldown");
    return;
  }
  ultimoComando = ahora;

  if (cmd == "metal") {
    Serial.println("Recibido: metal");
    servosMetal();  
  }
  else if (cmd == "plastico") {
    Serial.println("Recibido: plastico");
    servosPlastico();
  }
  else if (cmd == "organico") {
    Serial.println("Recibido: organico");
    servosOrganico();
  }
  else if (cmd == "no reciclable") {
    Serial.println("Recibido: noreciclable");
    servoNoReciclable();
  }
  else {
    Serial.println("Comando desconocido: " + cmd);
  }
}
