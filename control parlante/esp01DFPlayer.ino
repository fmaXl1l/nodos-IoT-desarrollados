#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

SoftwareSerial mySoftwareSerial(0, 2); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

WiFiClient esp8266MQ2Client;
PubSubClient mqttClient(esp8266MQ2Client);

const char* ssid = "ALDRINCNT";
const char* password = "0400916805";
const char* server = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "ESP01_DFPLAYER";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

const int LED = 3;

int alarma = 0;
int timbre = 0;
int bienvenido = 0;
int gas = 0;
String bomba = "INDEF";
int vol = -1;
int volAntes = -1;
int nextSong = 0;
int song = 0;

void reconnectMqtt(){
  while(!mqttClient.connected()){
    if(mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_pass)){
      mqttClient.subscribe("ESP01/DFPLAYER/ALARMA");
      mqttClient.subscribe("PIC/TIMBRE");
      mqttClient.subscribe("PIC/ACCESO");
      mqttClient.subscribe("ESP01/DFPLAYER/BOMBA");
      mqttClient.subscribe("ESP01/DFPLAYER/GAS");
      mqttClient.subscribe("ESP01/DFPLAYER/VOL");
      mqttClient.subscribe("ESP01/DFPLAYER/CANCION");
      digitalWrite(LED, HIGH);
    }else{
      digitalWrite(LED, LOW);
    }
  }
}

void wifiInit(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  char payload_char[length + 1];
  
  memcpy(payload_char, payload, length);
  payload_char[length] = '\0';

  if(strcmp(topic, "ESP01/DFPLAYER/ALARMA") == 0){
    alarma = atoi(payload_char);
  } else if (strcmp(topic, "PIC/TIMBRE") == 0){
    timbre = atoi(payload_char);
  } else if (strcmp(topic, "PIC/ACCESO") == 0){
    bienvenido = atoi(payload_char);
  } else if (strcmp(topic, "ESP01/DFPLAYER/BOMBA") == 0){
    bomba = payload_char;
  } else if (strcmp(topic, "ESP01/DFPLAYER/GAS") == 0){
    gas = atoi(payload_char);
  } else if (strcmp(topic, "ESP01/DFPLAYER/VOL") == 0){
    vol = atoi(payload_char);
  } else if (strcmp(topic, "ESP01/DFPLAYER/CANCION") == 0){
    nextSong = atoi(payload_char);
  }
}

void publishSongMqtt(){
  switch (song) {
  case 9:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "EQUILIVRE-CUENTAME");
    break;
  case 10:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "FLYPSIDE-SOMEDAY");
    break;
  case 11:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "FMK-PERDONAME");
    break;
  case 12:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "ALAN WALKER-FADED");
    break;
  case 13:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "AVENTURA FT. DON OMAR-ELLA Y YO");
    break;
  case 14:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "JULIO JARAMILLO-AYER Y HOY");
    break;
  case 15:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "LA YOUNG-MIS GANAS DE VOLAR");
    break;
  case 16:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "PAULO LONDRA-CHANCE");
    break;
  case 17:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "NIRVANA-SMELLS LIKE TEEN SPIRIT");
    break;
  default:
    mqttClient.publish("ESP01/DFPLAYER/CANCION/NOMBRE", "ERROR");
  }
}

void setup() {
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    while(true);
  }

  //------WiFi MQTT--------
  wifiInit();
  mqttClient.setServer(server, port);
  mqttClient.setCallback(callback);
  
  //Led: Indica que la configuracion inicial se realizo correctamente
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void loop() {
  if(!mqttClient.connected()){
    reconnectMqtt();
  }
  mqttClient.loop();

  if(alarma == 1){
    myDFPlayer.play(1);
    delay(1000);
    alarma = 0; 
  }
  if(timbre == 1){
    myDFPlayer.play(2);
    delay(1000);
    timbre = 0;
  }
  if(bienvenido == 1){
    myDFPlayer.play(3);
    delay(1000);
    bienvenido = 0;
  }
  if(bomba == "ON"){
    myDFPlayer.play(4);
    delay(1000);
    bomba = "INDEF";
  } else if(bomba == "OFF"){
    myDFPlayer.play(5);
    delay(1000);
    bomba = "INDEF";
  }
  if(gas == 1){
    myDFPlayer.play(6);
    delay(1000);
    gas = 0;
  }
  if(vol != volAntes){
    myDFPlayer.volume(vol);  //Set volume value. From 0 to 30
    if(vol > volAntes){
      myDFPlayer.play(7);
      delay(1000);
    } else {
      myDFPlayer.play(8);
      delay(1000);
    }
    volAntes = vol;
  }

  if(nextSong == 1){
    if(song == 0){
      song = 9;
      myDFPlayer.play(song);
      publishSongMqtt();
    } else {
      song = song + nextSong;
      if(song > 17){
        song = 9;
      }
      myDFPlayer.play(song);
      publishSongMqtt();
    }
    nextSong = 0;
  } else if (nextSong == -1){
    if(song == 0){
      song = 17;
      myDFPlayer.play(song);
      publishSongMqtt();
    } else {
      song = song + nextSong;
      if(song < 9){
        song = 17;
      }
      myDFPlayer.play(song);
      publishSongMqtt();
    }
    nextSong = 0;
  }
  
  delay(100);
}
