#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const int bits = 32;

//Mensajes IR Samsung TV
const unsigned long SamsungTv_power = 0x707030CF;
const unsigned long SamsungTv_mute = 0x7070B04F;
const unsigned long SamsungTv_source = 0x7070F00F;
const unsigned long SamsungTv_home = 0x70701EE1;
const unsigned long SamsungTv_back = 0x707026D9;
const unsigned long SamsungTv_exit = 0x7070AD52;
const unsigned long SamsungTv_ok = 0x7070629D;
const unsigned long SamsungTv_up = 0x707042BD;
const unsigned long SamsungTv_down = 0x7070C23D;
const unsigned long SamsungTv_left = 0x707022DD;
const unsigned long SamsungTv_right = 0x7070A25D;
const unsigned long SamsungTv_netflix = 0x7070AB54;
const unsigned long SamsungTv_youtube = 0x707007F8;
const unsigned long SamsungTv_volUp = 0x707028D7;
const unsigned long SamsungTv_volDown = 0x7070A857;

//Mensajes IR NEC TV
const unsigned long NECTv_power = 0x20DF10EF;
const unsigned long NECTv_source = 0x20DFD02F;
const unsigned long NECTv_volUp = 0x20DF40BF;
const unsigned long NECTv_volDown = 0x20DFC03F;
const unsigned long NECTv_chUp = 0x20DF00FF;
const unsigned long NECTv_chDown = 0x20DF807F;
const unsigned long NECTv_up = 0x20DF02FD;
const unsigned long NECTv_down = 0x20DF827D;
const unsigned long NECTv_left = 0x20DFE01F;
const unsigned long NECTv_right = 0x20DF609F;
const unsigned long NECTv_ok = 0x20DF22DD;
const unsigned long NECTv_exit = 0x20DFDA25;


const uint16_t kIrLed = 3; //GPIO4 (D2) NodeMCU   ESP-01 GPIO3
const uint16_t kRecvPin = 2; //GPIO14 (D5) NodeMCU   ESP-01 GPIO2
IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);
decode_results results;

WiFiClient esp01IrClient;
PubSubClient mqttClient(esp01IrClient);

const char* ssid = "ALDRINCNT";
const char* password = "0400916805";
const char* server = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "ESP01_IR";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

int result_SamsungTv = 0;
int result_NECTv = 0;

void SamsungTv(){
  irsend.begin();
  switch(result_SamsungTv){
    case 1:
      irsend.sendSAMSUNG(SamsungTv_power, bits);
      break;
    case 2:
      irsend.sendSAMSUNG(SamsungTv_mute, bits);
      break;
    case 3:
      irsend.sendSAMSUNG(SamsungTv_source, bits);
      break;
    case 4:
      irsend.sendSAMSUNG(SamsungTv_home, bits);
      break;
    case 5:
      irsend.sendSAMSUNG(SamsungTv_back, bits);
      break;
    case 6:
      irsend.sendSAMSUNG(SamsungTv_exit, bits);
      break;
    case 7:
      irsend.sendSAMSUNG(SamsungTv_ok, bits);
      break;
    case 8:
      irsend.sendSAMSUNG(SamsungTv_up, bits);
      break;
    case 9:
      irsend.sendSAMSUNG(SamsungTv_down, bits);
      break;
    case 10:
      irsend.sendSAMSUNG(SamsungTv_left, bits);
      break;
    case 11:
      irsend.sendSAMSUNG(SamsungTv_right, bits);
      break;
    case 12:
      irsend.sendSAMSUNG(SamsungTv_netflix, bits);
      break;
    case 13:
      irsend.sendSAMSUNG(SamsungTv_youtube, bits);
      break;
    case 14:
      irsend.sendSAMSUNG(SamsungTv_volUp, bits);
      break;
    case 15:
      irsend.sendSAMSUNG(SamsungTv_volDown, bits);
      break;     
    default:
      Serial.println("Samsung TV: Boton NO configurado");       
      break;
  }
  result_SamsungTv = 0;
  irrecv.enableIRIn();
}

void NECTv(){
  irsend.begin();
  switch(result_NECTv){
    case 1:
      irsend.sendNEC(NECTv_power, bits);
      break;
    case 2:
      irsend.sendNEC(NECTv_source, bits);
      break;
    case 3:
      irsend.sendNEC(NECTv_volUp, bits);
      break;
    case 4:
      irsend.sendNEC(NECTv_volDown, bits);
      break;
    case 5:
      irsend.sendNEC(NECTv_chUp, bits);
      break;
    case 6:
      irsend.sendNEC(NECTv_chDown, bits);
      break;
    case 7:
      irsend.sendNEC(NECTv_left, bits);
      break;
    case 8:
      irsend.sendNEC(NECTv_up, bits);
      break;
    case 9:
      irsend.sendNEC(NECTv_down, bits);
      break;
    case 10:
      irsend.sendSAMSUNG(NECTv_right, bits);
      break;
    case 11:
      irsend.sendSAMSUNG(NECTv_ok, bits);
      break;
    case 12:
      irsend.sendSAMSUNG(NECTv_exit, bits);
      break;
    default:
      Serial.println("NEC Tv: Boton NO configurado");       
      break;
  }
  result_NECTv = 0;
  irrecv.enableIRIn();
}

//MQTT
void reconnect(){
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
      mqttClient.subscribe("ESP01/IR/SAMSUNG/TV");
      mqttClient.subscribe("ESP01/IR/NEC/TV");
    }
    else {
      Serial.println("Fallo, rc=" + String(mqttClient.state()) + " intentar de nuevo en 5seg");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char payload_char[length + 1];

  Serial.print("Mensaje recibido [" + String(topic) + "]: ");
  memcpy(payload_char, payload, length);
  payload_char[length] = '\0';
  int resultI = atoi(payload_char);
  Serial.println(resultI);

  if(strcmp(topic, "ESP01/IR/SAMSUNG/TV") == 0){
    result_SamsungTv = resultI;
  } else if(strcmp(topic, "ESP01/IR/NEC/TV") == 0){
    result_NECTv = resultI;
  }
  else{
    Serial.println("Topic no encontrado!");
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
  irrecv.enableIRIn();  // Start the receiver
  Serial.begin(115200);
  Serial.println();
  Serial.println("-------------------IR-------------------");

  //-------WiFi MQTT--------
  wifiInit();
  mqttClient.setServer(server, port);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  if (irrecv.decode(&results)){
    if(results.value == SamsungTv_power) {
      mqttClient.publish("ESP01/IR/RX", "Samsung Tv");
    } else if (results.value == NECTv_power) {
      mqttClient.publish("ESP01/IR/RX", "NEC Tv");
    } else {
      mqttClient.publish("ESP01/IR/RX", "No encontrado");
    }
    irrecv.resume();
  }
  
  if (result_SamsungTv != 0) {
    SamsungTv();
  }
  if (result_NECTv != 0) {
    NECTv();
  }
  delay(200);
}
