#include "esp_camera.h"
#include "FS.h"
#include "SPI.h"
#include "SD.h"
#include "EEPROM.h"
#include "driver/rtc_io.h"
#include "ESP32_MailClient.h"
#include <WebServer.h>
#include <WiFiClient.h>
#include "src/OV2640.h"
#include <PubSubClient.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

#define COUNT_ADDRESS         0x01
#define EEPROM_SIZE           0x0F

uint16_t nextImageNumber = 0;

#define WIFI_SSID             "ALDRINCNT"
#define WIFI_PASSWORD         "0400916805"
// Configuración de la IP estática.
IPAddress local_IP(192, 168, 30, 30);
IPAddress gateway(192, 168, 30, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //opcional
IPAddress secondaryDNS(8, 8, 4, 4); //opcional

#define emailSenderAccount    "emailDelEmisor@gmail.com"    //To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1
#define emailSenderPassword   "aaaa11111" //Contraseña aplicacion no segura

#define emailRecipient        "asdfsdffd@gmail.com" //Email destino

//The Email Sending data object contains config and data to send
SMTPData smtpData;

OV2640 cam;
WebServer server(80);
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTYPE);

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);
const char* serverMqtt = "192.168.30.50";
const int port = 1883;
const char *MQTT_CLIENT_NAME = "Cam1";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt-password";

int result_Cam1Flash = 0;
int result_Notification = 0;

void handle_jpg_stream(void)
{
  char buf[32];
  int s;

  WiFiClient client = server.client();

  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while(true){
    if (!client.connected()) break;
    cam.run();
    s = cam.getSize();
    client.write(CTNTYPE, cntLen);
    sprintf(buf, "%d\r\n\r\n", s);
    client.write(buf, strlen(buf));
    client.write((char *)cam.getfb(), s);
    client.write(BOUNDARY, bdrLen);
  }
}

const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

void handle_jpg(void){
  WiFiClient client = server.client();
  cam.run();
  if(!client.connected()) return;
  client.write(JHEADER, jhdLen);
  client.write((char *)cam.getfb(), cam.getSize());
}

void handleNotFound(){
  String message = "Server is running!\n\n";
  message += "URI:";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

void restartPictureCount(){
  Serial.println("Restarting picture count");
  nextImageNumber = 0;
  EEPROM.put(COUNT_ADDRESS, nextImageNumber); 
  EEPROM.commit(); 
}

//MQTT
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
      mqttClient.subscribe("CAM/01/FLASH");
      mqttClient.subscribe("CAM/01/NOTIFICATION");
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

  if(strcmp(topic, "CAM/01/FLASH") == 0){
    result_Cam1Flash = resultI;
  } else if(strcmp(topic, "CAM/01/NOTIFICATION") == 0){
    result_Notification = resultI;
  }
  else{
    Serial.println("Topic no encontrado!");
  }
}

//Callback function to get the Email sending status
void sendCallback(SendStatus info);

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  // Establecimiento de la IP estática.
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Fallo en la configuración.");
  }  
  //Connect to WiFi network  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Stream Link: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/mjpeg");
  server.on("/mjpeg", HTTP_GET, handle_jpg_stream);
  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();
    
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  //init with high specs to pre-allocate larger buffers
  if(psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else 
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  //initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //set the camera parameters
  sensor_t * s = esp_camera_sensor_get();
  s->set_contrast(s, 2);    //min=-2, max=2
  s->set_brightness(s, 2);  //min=-2, max=2
  s->set_saturation(s, 2);  //min=-2, max=2
  delay(100);               //wait a little for settings to take effect
  
  //mount SD card
  Serial.println("Mounting SD Card...");
  MailClient.sdBegin(14,2,15,13);

  if(!SD.begin())
  {
    Serial.println("Card Mount Failed");
    return;
  }

  //initialize EEPROM & get file number
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialise EEPROM"); 
    Serial.println("Exiting now"); 
    while(1);   //wait here as something is not right
  }
//  restartPictureCount();

  pinMode(16, INPUT);   //PIR sensor
  pinMode(4, OUTPUT);              //GPIO for LED flash
  digitalWrite(4, LOW);            //turn OFF flash LED

  //MQTT
  mqttClient.setServer(serverMqtt, port);
  mqttClient.setCallback(mqttCallback);
}

void loop() 
{
  server.handleClient();  //Streaming

  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
    
  if(result_Cam1Flash == 1){
    digitalWrite(4, HIGH);
  } else {
    digitalWrite(4, LOW);
  }
  
  if(digitalRead(16) == HIGH && result_Notification == 1){
    mqttClient.publish("CAM/01/PIR", "1");
    
    digitalWrite(4, HIGH);
    EEPROM.get(COUNT_ADDRESS, nextImageNumber);
    nextImageNumber +=  1;    
    Serial.print("Next image number:");
    Serial.println(nextImageNumber);
  
    //take new image
    camera_fb_t * fb = NULL;
      
    //obtain camera frame buffer
    fb = esp_camera_fb_get();
    if (!fb) 
    {
      Serial.println("Camera capture failed");
      Serial.println("Exiting now"); 
      while(1);   //wait here as something is not right
    }
  
    //save to SD card
    //generate file path
    String path = "/IMG" + String(nextImageNumber) + ".jpg";
      
    fs::FS &fs = SD;
  
    //create new file
    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file)
    {
      Serial.println("Failed to create file");
      Serial.println("Exiting now"); 
      while(1);   //wait here as something is not right    
    } 
    else 
    {
      file.write(fb->buf, fb->len); 
      EEPROM.put(COUNT_ADDRESS, nextImageNumber);
      EEPROM.commit();
    }
    file.close();
  
    //return camera frame buffer
    esp_camera_fb_return(fb);
    Serial.printf("Image saved: %s\n", path.c_str());
  
    //send email
    Serial.println("Sending email...");
    //Set the Email host, port, account and password
    smtpData.setLogin("smtp.gmail.com", 587, emailSenderAccount, emailSenderPassword);
    
    //Set the sender name and Email
    smtpData.setSender("ESP32-CAM #1", emailSenderAccount);
    
    //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
    smtpData.setPriority("Normal");
  
    //Set the subject
    smtpData.setSubject("Movimiento Detectado");
      
    //Set the message - normal text or html format
    smtpData.setMessage("<div style=\"color:#003366;font-size:15px;\">Se adjunta la imagen capturada.</div>", true);
  
    //Add recipients, can add more than one recipient
    smtpData.addRecipient(emailRecipient);
    
    //Add attach files from SD card
    smtpData.addAttachFile(path);
    
    //Set the storage types to read the attach files (SD is default)
    smtpData.setFileStorageType(MailClientStorageType::SD);
    
    smtpData.setSendCallback(sendCallback);
    
    //Start sending Email, can be set callback function to track the status
    if (!MailClient.sendMail(smtpData))
      Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
  
    //Clear all data from Email object to free memory
    smtpData.empty();
    
    digitalWrite(4, LOW);
    mqttClient.publish("CAM/01/PIR", "0");
    delay(3000);
  }
}

//Callback function to get the Email sending status
void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
    Serial.println("----------------");
  }
}
