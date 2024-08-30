#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHTesp.h"
#include <NTPClient.h> //to take time
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define DHT_PIN 15
#define BUZZER 12

#define LDR_LEFT 32
#define LDR_RIGHT 33
#define SERVMOTOR 4

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     3600*5.5
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);
DHTesp dhtSensor;



WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);



char tempAr[6];
char ldrLeftArr[4];
char ldrRightArr[4];

Servo servo;

bool isScheduledON= false;
unsigned long scheduledOnTime;

int t_off = 30;
float gamma_i = 0.75;

void setup() {

  Serial.begin(115200);
  setupWiFi();
  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  servo.attach(SERVMOTOR);

  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);  

  
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER,LOW);
}

void loop() {
  if(!mqttClient.connected()){
    Serial.println("Reconnecting to MQTT Broker");
    connectToBroker();
  }
  mqttClient.loop();


  updateTemperature();
  Serial.println(tempAr);
  mqttClient.publish("ENTC-AS-TEMP", tempAr);

  checkSchedule();
  delay(100);
  
  updateLDR(); // Function to update LDR sensor data

  mqttClient.publish("ENTC-LIGHT-L-AS", ldrLeftArr);
  delay(50);
  mqttClient.publish("ENTC-LIGHT-R-AS", ldrRightArr);
  delay(50);
}


void buzzerOn(bool on){
  if (on){
    tone(BUZZER,256);

  }else{
    noTone(BUZZER);
  }
}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883);
  mqttClient.setCallback(receiveCallback);
}

void receiveCallback(char* topic,byte* payload, unsigned int length){

  // Function to handle received MQTT messages
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];
  Serial.print("Message Received: ");
  for (int i=0; i<length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i]=(char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, "ENTC-AS-MAIN-ON-OFF")==0){
    buzzerOn(payloadCharAr[0]=='1');
  }else if(strcmp(topic, "ENTC-AS-SCH-ON")==0){
    if(payloadCharAr[0]=='N'){
      isScheduledON = false;
    }else{
      isScheduledON = true;
      scheduledOnTime=atol(payloadCharAr);
    }
  }else if(strcmp(topic, "SERVO-ADJUST-MIN.ANGLE")==0) {
    t_off = String(payloadCharAr).toInt();
  }else if(strcmp(topic, "SERVO-ADJUST-Cnt.Factor")==0) {
    gamma_i = String(payloadCharAr).toFloat();
  }
}
void connectToBroker(){
  // Function to establish connection with MQTT broker
  while (!mqttClient.connected()){
    Serial.print("Attempting MQTT connection...");
    if(mqttClient.connect("ESP32- 12345645454")){
      Serial.println("MQTT connected ");
      mqttClient.subscribe("ENTC-AS-MAIN-ON-OFF");
      mqttClient.subscribe("ENTC-AS-SCH-ON");
      mqttClient.subscribe("SERVO-ADJUST-MIN.ANGLE");
      mqttClient.subscribe("SERVO-ADJUST-Cnt.Factor");

      //subscribe
    }else{
      Serial.print("Failed to connect to the MQTT Broker");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void updateTemperature(){
  // Function to read temperature data from DHT sensor
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(tempAr,6);
}

void setupWiFi(){
  Serial.println();
  Serial.print("Connecting to WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime(){
  // Function to get current time from NTP server
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule(){
  // Function to check if scheduled ON time is reached
  if(isScheduledON){
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime ){
      buzzerOn(true);
      isScheduledON= false;
      mqttClient.publish("ENTC-AS-MAIN-ON-OFF-ESP","1");
      mqttClient.publish("ENTC-AS-SCH-ESP-ON","0");

      Serial.println("Scheduled ON");
    }
  }
}

void updateLDR() {
  float lsv = analogRead(LDR_LEFT) * 1.00;
  float rsv = analogRead(LDR_RIGHT) * 1.00;
  
  float lsv_cha = (float)(lsv - 4063.00)/(32.00-4063.00);
  float rsv_cha = (float)(rsv - 4063.00)/(32.00-4063.00);

  updateAngle(lsv_cha,rsv_cha);

  String(lsv_cha).toCharArray(ldrLeftArr,4);
  String(rsv_cha).toCharArray(ldrRightArr,4);
}

void updateAngle(float lsv, float rsv){
  // Function to update servo motor angle based on LDR sensor data
  float max_I = lsv;
  float D = 1.5;

  if(rsv > max_I){
    max_I = rsv;
    D = 0.5;
  }

  int theta = t_off * D + (180 - t_off) * max_I * gamma_i;
  theta = min(theta,180);
  servo.write(theta);
}