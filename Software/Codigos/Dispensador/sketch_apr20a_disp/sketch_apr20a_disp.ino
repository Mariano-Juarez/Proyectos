#define BLYNK_TEMPLATE_ID "TMPL2xfJDexad"
#define BLYNK_TEMPLATE_NAME "Dispensador de comida"
#define BLYNK_AUTH_TOKEN "8yKlAWbRWKOkqiLpHtfQAl062bfHvIUr"

#include <WiFi.h>
#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>  // MQTT

// --- Configuración WiFi ---
const char* ssid = "MisDatos";
const char* password = "uricomefideoscontuco";

// --- Configuración MQTT ---
const char* mqtt_server = "broker.hivemq.com";   // Podés usar Mosquitto o HiveMQ
const int mqtt_port = 1883;
const char* mqtt_client_id = "dispensadorComida_ESP32";
const char* mqtt_user = "";   // si tu broker lo pide
const char* mqtt_pass = "";   // si tu broker lo pide

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- Pines ---
const int trigPin = 33;
const int echoPin = 34;
const int relePin = 16;
const int servoPin = 23;
const int umbralNivel = 10;  // cm

Servo servoMotor;

// --- Prototipos ---
void reconnectMQTT();
int leerNivel();
void dispensarComida(int porciones);

// --- Setup ---
void setup() {
  Serial.begin(115200);

  // Conexión a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  // MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Servo
  servoMotor.attach(servoPin);
  servoMotor.write(0);

  // Sensor ultrasónico y relé
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW);
}

// --- Funciones MQTT ---
void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  // Comando para dispensar desde MQTT
  if (String(topic) == "dispensador/comida") {
    if (msg == "1") dispensarComida(1);
    else if (msg == "2") dispensarComida(2);
    else if (msg == "3") dispensarComida(3);
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      mqttClient.subscribe("dispensador/comida"); // escuchar pedidos de comida
    } else {
      Serial.print("falló, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" reintento en 5s");
      delay(5000);
    }
  }
}

// --- Funciones de control ---
void dispensarComida(int porciones) {
  int tiempo = 1000 * porciones;
  servoMotor.write(90);  // Abrir compuerta
  delay(tiempo);
  servoMotor.write(0);   // Cerrar compuerta
}

// --- Botones desde Blynk ---
BLYNK_WRITE(V0) { if (param.asInt() == 1) dispensarComida(1); }
BLYNK_WRITE(V2) { if (param.asInt() == 1) dispensarComida(2); }
BLYNK_WRITE(V1) { if (param.asInt() == 1) dispensarComida(3); }

// --- Loop ---
void loop() {
  Blynk.run();

  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  int nivel = leerNivel();
  Serial.print("Nivel de comida (cm): ");
  Serial.println(nivel);

  // Publicar nivel cada segundo
  String payload = String(nivel);
  mqttClient.publish("dispensador/nivel", payload.c_str());

  if (nivel > umbralNivel) {
    digitalWrite(relePin, HIGH);
    Blynk.virtualWrite(V3, 255);
  } else {
    digitalWrite(relePin, LOW);
    Blynk.virtualWrite(V3, 0);
  }

  delay(1000);
}

// --- Medición ultrasónica ---
int leerNivel() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH);
  int distancia = duracion * 0.034 / 2;
  return distancia;
}
