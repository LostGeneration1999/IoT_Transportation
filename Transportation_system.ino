#include <SR04.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Servo.h>
#include "ArduinoJson.h" 

#define ID "ESP32"
#define OUT_TOPIC "ESP32"
#define IN_TOPIC "ESP32"

//wifi接続設定
const char* ssid = [ssid];
const char* password = [password];

//AWS MQTT接続設定
const char* server = "[end point]";
const int port = 8883;

//初期化時に送信するJson
const char* json = "{\"led\":\"off\"}";

/変数設定
Servo myservo;
StaticJsonDocument<1024> doc;
const int ANALOG_MAX = 4096; 
char situation;
long duration;
int distance;
int sum;
int average;
int distance_initialize;

//ピン設定
const int motorPin = 14;
const int brightnessPin = 25;
const int LED_pin = 15;
const int LED_pin2 = 2;
const int LED_pin3 = 17;
const int trigPin = 19;
const int echoPin = 18;
const int VOLT = 3.3; 

//モーター固有の静止値
const int motor_static = 85;
//モーターを動作させる時に必要な値
const int motor_kinetic_plus = 1000;
//モーターを動作させる時に必要な値
const int motor_kinetic_minus = 0;

//MQTT証明書-RootCA
const char* Root_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"-----END CERTIFICATE-----\n";

//MQTT証明書-Certification
const char* Client_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"-----END CERTIFICATE-----\n";

//MQTT証明書-PrivateKey
const char* Client_private = \
"-----BEGIN RSA PRIVATE KEY-----\n" \

"-----END RSA PRIVATE KEY-----\n";
  
WiFiClientSecure client;
PubSubClient mqttClient(client);

//AWS IoT接続関数
void connectAWSIoT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect("ESP32")) {
            Serial.println("Connected.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

//初期化関数
void setup()
{
  Serial.begin(115200);
  pinMode(LED_pin, OUTPUT);
  pinMode(LED_pin2, OUTPUT);
  pinMode(LED_pin3, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); 

  myservo.attach(motorPin);
  pinMode(motorPin,OUTPUT);
   
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  delay(1000);

  Serial.println(Root_CA);
  Serial.println(Client_cert);
  Serial.println(Client_private);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setCACert(Root_CA);
  client.setCertificate(Client_cert);
  client.setPrivateKey(Client_private);
  mqttClient.setServer(server, port);  
  mqttClient.setCallback(callback);
  
  connectAWSIoT();
  
  //qos -> 0 or 1
  int qos = 0;
  Serial.println(MQTT_MAX_PACKET_SIZE);
  if(mqttClient.subscribe(IN_TOPIC, qos)){
    Serial.println("Subscribed.");
    Serial.println("Success!!");
  }
  
   //Json化
   deserializeJson(doc, json);
   JsonObject& obj = doc.as<JsonObject>();
  
　   //Publish
　　　　　　if(mqttClient.publish(OUT_TOPIC, json)){
    Serial.println("Published!!");
   }
   delay(2000);
   distance_initialize = sr04();
}
  
//MQTT loop時に　Subcribeすると、呼び出す関数
void callback (char* topic, byte* payload, unsigned int length) {
    Serial.println("Received. topic=");
    Serial.println(topic);
    char subsc[length];
    for (int i = 0; i < length; i++) {
        subsc[i] = (char)payload[i];
        subsc[length]='\0';
    }
    Serial.println(subsc);
    deserializeJson(doc, subsc);
    JsonObject& obj = doc.as<JsonObject>();
    int data = int(obj["led"]);
    Serial.println(data);
    
    //Subcribeするデータによる条件分岐-適宜変更してください
    if(data==1){
      digitalWrite(LED_pin2, HIGH);
      digitalWrite(LED_pin, LOW);
      myservo.write(motor_kinetic_plus);
      Serial.println("LED_pin2 bright");
      delay(100);  
    }
    else if(data==2){
      digitalWrite(LED_pin, HIGH);       
      digitalWrite(LED_pin2, LOW);
      myservo.write(motor_kinetic_minus);
      Serial.println("LED_pin bright");
      delay(100);  
    }
    else{
      Serial.println("error");
    }
    
    Serial.print("\n");
}

//通常ループ
void mqttLoop() {
    mqttClient.loop();
    delay(1000);
    myservo.write(motor_static);
    Serial.print(".");
}

//SR04センサー関数
int sr04(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  distance= duration*0.034/2;

  Serial.print("Distance: ");
  Serial.println(distance);
  delay(10);
  return distance;
}


void loop() {
  mqttLoop();
  distance = sr04(); 
  
  //物体があるかないかを識別
  if(distance < distance_initialize){
    const char* box_occupy = "{\"situation\":\"occupy\"}";
    digitalWrite(LED_pin3, HIGH); 
    mqttClient.publish(OUT_TOPIC, box_occupy);
  }
  else{
     const char* box_empty = "{\"situation\":\"empty\"}";
     digitalWrite(LED_pin3, LOW);
     mqttClient.publish(OUT_TOPIC, box_empty);
  }
  delay(100);
}
