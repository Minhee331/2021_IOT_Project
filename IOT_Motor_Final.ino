#include <ESP32_Servo.h>
#include <AWS_IOT.h>
#include "WiFi.h"
#include <Arduino_JSON.h>
#include <Wire.h>

/////WIFI & AWS////////////////
const char* ssid = "jy-iptime";
const char* password = "westlangley23!";
char HOST_ADDRESS[]="a2lbqogkjpa0xw-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "21_Motor";
char sTOPIC_NAME[] = "$aws/things/21_Motor/shadow/update/delta";
char pTOPIC_NAME[]= "21_Motor/new"; 
char pTOPIC2_NAME[]= "21_Motor/button"; 

AWS_IOT Motor;
static const int servoPin = 27;
int buttonState = 0;
Servo servo1;
//AWS_IOT testButton; // water
/////WIFI & AWS////////////////


int status = WL_IDLE_STATUS;
int msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];


void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1; //flag
}

String motor_state = "off"; //motor_off
int feed_total = 0;
int feed_count = 0;


void setup() {
  Serial.begin(115200);
    servo1.attach(servoPin); // D27에 서보모터가 연결되었습니다.
//    digitalWrite(buttonPin, HIGH);
    servo1.write(180);
    //++choi This is here to force the ESP32 to reset the WiFi and initialize correctly.
    Serial.print("WIFI status = ");
    Serial.println(WiFi.getMode());
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    Serial.print("WIFI status = ");
    Serial.println(WiFi.getMode()); //++choi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to wifi");
    if(Motor.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
      Serial.println("Connected to AWS");
      delay(1000);
      if(0==Motor.subscribe(sTOPIC_NAME,mySubCallBackHandler)) {
        Serial.println("Subscribe Successfull");
      }
      else {
        Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
        while(1);
      }
    }
    else {
      Serial.println("AWS connection failed, Check the HOST Address");
      while(1);
    }
}

void loop() {
    if(msgReceived == 1){
      Serial.print("Received  Message:");
      Serial.println(rcvdPayload);
      JSONVar myObj = JSON.parse(rcvdPayload);
      JSONVar state = myObj["state"];
      motor_state = state["button"];
      delay(1000);
    }

    
//    buttonState = digitalRead(buttonPin);
    if(motor_state == "on") {
      Serial.print("pressed");
      feed_count += 1;
      feed_total += 15;
      servo1.write(8);
      delay(2000);
      servo1.write(172);
      delay(2000);
      buttonState = LOW;
      sprintf(payload,"{\"state\": {\"reported\": {\"button\":\"off\"}}}"); // 모터 본인 상태 변화 ON/OFF
      while(1){
        if(Motor.publish(pTOPIC2_NAME,payload) == 0) {
          Serial.print("Publish Message:");
          Serial.println(payload);
          break;
        }
        else{
          Serial.println("Publish failed");
        }
      }
      while(1){
        sprintf(payload,"{\"state\": {\"reported\": {\"feed_total\":%d, \"feed_count\":%d}}}",feed_total,feed_count); // 서버 델타값 변경 
        if(Motor.publish(pTOPIC_NAME,payload) == 0) {
          Serial.print("Publish Message:");
          Serial.println(payload);
          break;
        }
        else{
          Serial.println("Publish failed");
        }
      }
    }   
}
