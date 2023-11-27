#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <WiFi.h>


// Credenciales WI-FI
const char *ssid = "Redmi 10 2022";
const char *password = "fabian123";
const char *mqtt_server = "docker.coldspace.cl";

// Iniciacion Del Cliente WI-FI
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char mensaje[MSG_BUFFER_SIZE];
// int value = 0;

// Pines de conexion para el RFID
#define SS_PIN 5   // Entrega señal al RFID por medio de SPI
#define RST_PIN 22 // encender y apagar modulo RFID
#define SERVO_PIN 26

Servo servomotor;

// Utilizar lso pines del RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

//lcd
LiquidCrystal_I2C lcd(0x27,16,2);

// Utilizar pines para los led
int ledVerde = 15;
int ledRojo = 4;
int buzzer=32;


// ID de las tarjetas y llaveros
byte tag1[4] = {0x39, 0xf8, 0xbb, 0xb8}; // card blanca
byte tag2[4] = {0x39, 0xF8, 0xBB, 0xB7}; // ID AZUL FABI

// ID del tag actual
byte tagActual[4];

void setup() {
  // inicializaer los led
  pinMode(ledVerde, OUTPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(buzzer,OUTPUT);

  servomotor.attach(SERVO_PIN);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0,0);
  lcd.print("hola");
  

  Serial.begin(115200); // init serial monitor
  SPI.begin();          // init SPI
  mfrc522.PCD_Init();   // init RFID

  setup_wifi(); // Llama a la Funcion de Conexion al Wi-fi

  client.setServer(mqtt_server, 1883); // Relaciona el Servidor con la IP
}

void loop() {
  servomotor.write(180);


  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  // Aqui se publica el Topic
  /*
  unsigned long now = millis();
  if (now - lastMsg > 2000) {// cada 2 segundos ahcer algo sin bloquear
    lastMsg = now;

  }
  */

  // Comprueba si hay tarjeta
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Lee ID de la tarjeta
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("Card UID:"); // Dump UID
  String rfidUid = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfidUid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    rfidUid += String(mfrc522.uid.uidByte[i], HEX);
  }

  Serial.println(rfidUid);
  client.publish("grupo14RFID/entrada", rfidUid.c_str());
  Serial.println("Publicado a MQTT");

  // comprobamos el acceso para el primer usuario
  if (rfidUid == "45aa702a") {

    //======================================
    Serial.println(" Acceso Permitido...");
    digitalWrite(ledVerde, HIGH);
    digitalWrite(buzzer, LOW);
    servomotor.write(0);
    delay(6000);

  } else if (rfidUid == "c672302b") {

    //=========================================
    Serial.println(" Acceso Permitido...");
    digitalWrite(ledVerde, HIGH);
    servomotor.write(0);
    delay(6000);
    servomotor.write(0);

  } else {

    //==============================
    Serial.println(" Desconocido"); // si el codigo no esta registrado denegar acceso
    digitalWrite(ledRojo, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(2000);
    digitalWrite(buzzer, LOW);
  }
  // Ubicar los led en low
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledRojo, LOW);
}

// Genera la comparacion entre la ID obtenido por el RFID con lso ID que esta en el programa
boolean compararArray(byte array1[], byte array2[]) {
  if (array1[0] != array2[0])
    return (false);
  if (array1[1] != array2[1])
    return (false);
  if (array1[2] != array2[2])
    return (false);
  if (array1[3] != array2[3])
    return (false);
  return (true);
}

// Funcion que establece la Conexion
void setup_wifi() {

  delay(10);
  // se conecta a una red WI-FI
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

// Funcion Para Reconectar
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);

    // TOPIC DEL SUBSCRIPTOR========================================================================
    //  Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("C", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
