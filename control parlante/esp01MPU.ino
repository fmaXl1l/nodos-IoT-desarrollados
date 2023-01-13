#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

//Direccion I2C de la IMU
#define MPU 0x68

//Ratios de conversion
#define A_R 16384.0 // 32768/2
#define G_R 131.0 // 32768/250

//Conversion de radianes a grados 180/PI
#define RAD_A_DEG = 57.295779

WiFiClient esp01MPUClient;
PubSubClient mqttClient(esp01MPUClient);

const char* ssid = "ALDRINCNT";
const char* password = "0400916805";
const char* server = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "ESP01_MPU";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

char datos[8];

//MPU-6050 da los valores en enteros de 16 bits
//Valores RAW
int16_t AcX, AcY, AcZ, rawTemp;
float Acc[2], Temp;
bool validate = false;

void wifiInit() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi conectado, IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
    }
    else {
      Serial.println("Fallo, rc=" + String(mqttClient.state()) + " intentar de nuevo en 5seg");
      delay(5000);
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  //-------WiFi MQTT--------
  wifiInit();
  mqttClient.setServer(server, port);
  
  Wire.begin(0, 2); // GPIO0=SDA / GPIO2=SCL
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop(void) {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  Wire.beginTransmission(MPU);
  Wire.write(0x3B); //Pedir el registro 0x3B - corresponde al AcX
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); //A partir del 0x3B, se piden 6 registros
  AcX = Wire.read() << 8 | Wire.read(); //Cada valor ocupa 2 registros
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  //A partir de los valores del acelerometro, se calculan los angulos X, Y
  //respectivamente, con la formula de la tangente.
  Acc[0] = atan((AcY / A_R) / sqrt(pow((AcX / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;
  Acc[1] = atan(-1 * (AcX / A_R) / sqrt(pow((AcY / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;

  Wire.beginTransmission(MPU);
  Wire.write(0x41);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 2, true);
  rawTemp = Wire.read() << 8 | Wire.read(); //Cada valor ocupa 2 registros
  Temp = (rawTemp / 340.0) + 34.53; //formula obtenida de la libreria Adafruit_MPU6050 temperature = (rawTemp / 340.0) + 36.53;

  if (Acc[0] >= 30) { //IZQUIERDA
    if(validate == true){
      mqttClient.publish("ESP01/DFPLAYER/VOL/DEC", "1");
      mqttClient.publish("ESP01/MPU6050/TEMP", String(Temp).c_str());
      validate = false;
    }
  } else if (Acc[0] <= -30) { //DERECHA
    if(validate == true){
      mqttClient.publish("ESP01/DFPLAYER/VOL/INC", "1");
      mqttClient.publish("ESP01/MPU6050/TEMP", String(Temp).c_str());
      validate = false;
    }
  } else if (Acc[1] >= 30) { //ABAJO
    if(validate == true){
      mqttClient.publish("ESP01/DFPLAYER/CANCION", "-1");
      mqttClient.publish("ESP01/MPU6050/TEMP", String(Temp).c_str());
      validate = false;
    }
  } else if (Acc[1] <= -30) { //ARRIBA
    if(validate == true){
      mqttClient.publish("ESP01/DFPLAYER/CANCION", "1");
      mqttClient.publish("ESP01/MPU6050/TEMP", String(Temp).c_str());
      validate = false;
    }
  } else {
    validate = true;
  }
  delay(500);
}
