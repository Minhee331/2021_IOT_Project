/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <AWS_IOT.h>
#include "WiFi.h"
#include <Arduino_JSON.h>
#include <Wire.h>

AWS_IOT testButton;

//const char* ssid = "U+Net810B";
//const char* password = "32F4000304";
const char* ssid = "jy-iptime";
const char* password = "westlangley23!";
char HOST_ADDRESS[]="a2lbqogkjpa0xw-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "21_Water_Pump";
//char sTOPIC_NAME[]= "$aws/things/Water_Pump_new/shadow/update/delta"; // subscribe topic name
//char pTOPIC_NAME[]= "$aws/things/Water_Pump_new/shadow/update"; // publish topic name
char sTOPIC_NAME[] = "$aws/things/21_Water_Pump/shadow/update/delta";
char pTOPIC_NAME[] = "21_Water_Pump/new";
char pTOPIC2_NAME[] = "21_Water_Pump/button";

int status = WL_IDLE_STATUS;
int msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];
int touchpin_state = 0;
String button_state = "off";
int A1A = 23;
int A1B = 22;

int water_level = 0;
int respin = 33;
int flag = 0;

void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1; //flag
}

void setup()
{
  Serial.begin(115200);
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
  if(testButton.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
    Serial.println("Connected to AWS");
    delay(1000);
    if(0==testButton.subscribe(sTOPIC_NAME,mySubCallBackHandler)) {
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
  pinMode(respin, OUTPUT);
  pinMode(A1A, OUTPUT);
  pinMode(A1B, OUTPUT);
}


//수위가 센서보다 아래에 있을때는 pump값이 

void loop()
{
  flag = 0;
  water_level = analogRead(respin)-800;

  if(water_level<0){
    water_level=0;
  }
  
  Serial.print("터치핀 값 : ");
  Serial.println(touchRead(4));
  if(touchRead(4)>48){
    touchpin_state = 0;
  }else{
    touchpin_state = 1;
  }
  if(msgReceived == 1) {
      msgReceived = 0;
      Serial.print("Received Message:");
      Serial.println(rcvdPayload);
      // Parse JSON
      JSONVar myObj = JSON.parse(rcvdPayload);
      JSONVar state = myObj["state"];
      button_state = state["button"];
      Serial.println(button_state);
    }
    
  if(touchRead(4)>48 && (water_level>1000-800 || button_state == "on")){
      Serial.println("모터 동작");
    
      digitalWrite(A1A, HIGH);
      digitalWrite(A1B, LOW);
      delay(5000);
    
      Serial.println("모터 멈춤");
      digitalWrite(A1A, LOW);
      digitalWrite(A1B, LOW);
      delay(1000); 
    
//      water_level = analogRead(respin)-800;
      
      sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d, \"remain_water_bowl\": %d}}}", water_level, touchpin_state);
  
      if(testButton.publish(pTOPIC_NAME,payload) == 0) {
        Serial.print("Publish Message:");
        Serial.println(payload);
      }
      else{
        Serial.println("Publish failed"); 
      }
      
      Serial.print("물의 높이 : ");
      Serial.println(water_level);
      while(button_state == "on"){
            sprintf(payload,"{\"state\": {\"reported\": {\"button\": \"off\"}}}");
            if(testButton.publish(pTOPIC2_NAME,payload) == 0) {
                Serial.print("Publish Message:");
                Serial.println(payload);
                button_state = "off";
            }
            else{
                Serial.println("Publish failed"); 
            }
      }
  }else{
      if(water_level<=1000-800 || touchRead(4)<=48){
        Serial.println("물통에 물이 부족하거나, 그릇에 물이 차있습니다.");

//        water_level = analogRead(respin)-800;
        Serial.print("물의 높이 : ");
        Serial.println(water_level);
        Serial.print("터치핀 값 : ");
        Serial.println(touchRead(4));
      
        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d, \"remain_water_bowl\": %d}}}", water_level, touchpin_state);
        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
          Serial.print("Publish Message:");
          Serial.println(payload);
        }
        else{
          Serial.println("Publish failed"); 
        }
        delay(7000);
      }
  }
}
