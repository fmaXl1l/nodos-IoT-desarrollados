#include <WiFi.h>
#include <PubSubClient.h>
#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;
BH1750 lightMeter;

const int sensorPin = 34;
const int AirValue = 3750;
const int WaterValue = 1775;
int sensorValue = 0;
int moisture = 0; //humedad

const int bombaPin = 32;
int bombaPower = 0;

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* server = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "ESP32_HUERTO";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

//MQTT
void reconnect(){
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

void wifiInit() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi conectado, IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lightMeter.begin();
  aht.begin();
  pinMode(bombaPin, OUTPUT);
  digitalWrite(bombaPin, LOW);

  //-------WiFi MQTT--------
  wifiInit();
  mqttClient.setServer(server, port);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
//  AHT10
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println("%rH");
  mqttClient.publish("ESP32/HUERTO/TEMPERATURA", String(temp.temperature).c_str());
  mqttClient.publish("ESP32/HUERTO/HUMEDAD", String(humidity.relative_humidity).c_str());
  
//  BH1750
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  mqttClient.publish("ESP32/HUERTO/LUMINOSIDAD", String(lux).c_str());

//  Humedad Suelo
  sensorValue = analogRead(sensorPin);
  if(sensorValue > AirValue){
    sensorValue = AirValue;
  } else if(sensorValue < WaterValue){
    sensorValue = WaterValue;
  }
  moisture = map(sensorValue, AirValue, WaterValue, 0, 100);
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print(" - percentage = ");
  Serial.println(moisture);
  mqttClient.publish("ESP32/HUERTO/SUELO", String(moisture).c_str());
  
//  Bomba de agua
  if(moisture < 60 and bombaPower == 0){
    bombaPower = 1;
    digitalWrite(bombaPin, HIGH);
    mqttClient.publish("ESP01/DFPLAYER/BOMBA", "ON");
  } else if (moisture >= 60 and bombaPower == 1) {
    bombaPower = 0;
    digitalWrite(bombaPin, LOW);
    mqttClient.publish("ESP01/DFPLAYER/BOMBA", "OFF");
  }
  
  delay(30000);
}
