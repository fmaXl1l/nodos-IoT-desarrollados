#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <spartan-edge-esp32-boot.h>

// initialize the spartan_edge_esp32_boot library
spartan_edge_esp32_boot esp32Cla;

// the bitstream name which we loading
#define LOADING_DEFAULT_FIEE "/overlay/i2c_leds.bit"
#define SLV_FPGA 0x01

#define OUTESP0_PIN 27
#define OUTESP1_PIN 18
#define INESP0_PIN 5
#define INESP1_PIN 19
#define DATA_READY_PIN 23

//Direccion I2C de la IMU
#define MPU 0x6B
int16_t rawTemp;
float Temp;
unsigned long tiempo = 0;

WiFiClient FPGAClient;
PubSubClient mqttClient(FPGAClient);

const char* ssid = "ALDRINCNT";
const char* password = "0400916805";
const char* server = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "FPGA";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

byte reles = 0;
int reles_b[10] = {0, 0, 0, 0, 0, 0, 0, 0};
int resultRele[10] = {2, 2, 2, 2, 2, 2, 2, 2};
int pastReles_b[10] = {0, 0, 0, 0, 0, 0, 0, 0};
char datos[8];

void wifiInit() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi conectado, IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char payload_char[length + 1];

  memcpy(payload_char, payload, length);
  payload_char[length] = '\0';
  int resultI = atoi(payload_char);

  if (strcmp(topic, "FPGA/RELE0") == 0) {
    resultRele[0] = resultI;
  } else if (strcmp(topic, "FPGA/RELE1") == 0) {
    resultRele[1] = resultI;
  } else if (strcmp(topic, "FPGA/RELE2") == 0) {
    resultRele[2] = resultI;
  } else if (strcmp(topic, "FPGA/RELE3") == 0) {
    resultRele[3] = resultI;
  } else if (strcmp(topic, "FPGA/RELE4") == 0) {
    resultRele[4] = resultI;
  } else if (strcmp(topic, "FPGA/RELE5") == 0) {
    resultRele[5] = resultI;
  } else if (strcmp(topic, "FPGA/RELE6") == 0) {
    resultRele[6] = resultI;
  } else if (strcmp(topic, "FPGA/RELE7") == 0) {
    resultRele[7] = resultI;
  } else if (strcmp(topic, "FPGA/RELE8") == 0) {
    resultRele[8] = resultI;
  } else if (strcmp(topic, "FPGA/RELE9") == 0) {
    resultRele[9] = resultI;
  } else {
    Serial.println("Topic NO encontrado!");
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
      for (int i = 0; i < 10; i = i + 1) {
        String tema = "FPGA/RELE" + String(i);
        const char* mqttSubscribeTopic = tema.c_str();
        mqttClient.subscribe(mqttSubscribeTopic);
      }
    }
    else {
      Serial.println("Fallo, rc=" + String(mqttClient.state()) + " intentar de nuevo en 5seg");
      delay(5000);
    }
  }
}

void setup() {
  // initialization
  esp32Cla.begin();

  // XFPGA pin Initialize
  esp32Cla.xfpgaGPIOInit();

  // loading the bitstream
  esp32Cla.xlibsSstream(LOADING_DEFAULT_FIEE);

  Wire.begin(A4, A5);

//  Inicializar MPU
  Wire.beginTransmission(MPU);
  Wire.write(0x10); //registro CTRL1_XL
  Wire.write(0x60); //valor a escribir
  Wire.endTransmission(false);
  Wire.beginTransmission(MPU);
  Wire.write(0x0D); //registro INT1_CTRL
  Wire.write(0x01); //valor a escribir
  Wire.endTransmission(true);
  
  //PINES
  pinMode(INESP0_PIN, INPUT);
  pinMode(INESP1_PIN, INPUT);
  pinMode(OUTESP0_PIN, OUTPUT);
  pinMode(OUTESP1_PIN, OUTPUT);
  pinMode(DATA_READY_PIN, OUTPUT);

  //-------WiFi MQTT--------
  wifiInit();
  mqttClient.setServer(server, port);
  mqttClient.setCallback(callback);
}

void loop() {
  digitalWrite(DATA_READY_PIN, LOW);
  
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  for (int i = 0; i < 8; i = i + 1) {
    if (resultRele[i] != pastReles_b[i] && resultRele[i] != 2) {
      Wire.beginTransmission(SLV_FPGA);
      bitWrite(reles, i, resultRele[i]);
      Wire.write(reles);
      Wire.endTransmission(true);

      pastReles_b[i] = resultRele[i];
      resultRele[i] = 2;
    }
  }

  if (resultRele[8] != pastReles_b[8] && resultRele[8] != 2) {
    digitalWrite(OUTESP0_PIN, bool(resultRele[8]));
    digitalWrite(DATA_READY_PIN, HIGH);
    pastReles_b[8] = resultRele[8];
    resultRele[8] = 2;
  }
  if (resultRele[9] != pastReles_b[9] && resultRele[9] != 2) {
    digitalWrite(OUTESP1_PIN, bool(resultRele[9]));
    digitalWrite(DATA_READY_PIN, HIGH);
    pastReles_b[9] = resultRele[9];
    resultRele[9] = 2;
  }

  Wire.requestFrom(SLV_FPGA, 1, true);
  reles = Wire.read();

  for (int i = 0; i < 8; i = i + 1) {
    reles_b[i] = bitRead(reles, i);
  }
  reles_b[8] = digitalRead(INESP0_PIN);
  reles_b[9] = digitalRead(INESP1_PIN);;

  for (int i = 0; i < 10; i = i + 1) {
    if (reles_b[i] != pastReles_b[i]) {
      sprintf(datos, "%d", reles_b[i]);
      String tema = "FPGA/RELE" + String(i);
      const char* mqttPublishTopic = tema.c_str();
      mqttClient.publish(mqttPublishTopic, datos);
      delay(250); //Cuando es publicador y suscriptor
      pastReles_b[i] = reles_b[i];
    }
  }

  if(millis() - tiempo >= 60000){ //60000 mseg = 1 min
    //Publicar temperatura MPU
      Wire.beginTransmission(MPU);
      Wire.write(0x20);
      Wire.endTransmission(false);
      Wire.requestFrom(MPU, 2, true);
      rawTemp = Wire.read() | Wire.read() << 8;
//      Serial.println("Temp: " + String(rawTemp));
      Temp = 19.5 + (rawTemp / 340.0);
//      Serial.println("Temperatura: " + String(Temp));
      mqttClient.publish("FPGA/TEMP", String(Temp).c_str());

      tiempo = millis();
  }
  
  delay(50);
}
