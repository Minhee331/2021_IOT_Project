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
const char* ssid = "KAU-Guest";
const char* password = "";
char HOST_ADDRESS[]="a2lbqogkjpa0xw-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "ChoiESP32";
char sTOPIC_NAME[]= "$aws/things/Water_Pump_new/data"; // subscribe topic name
char pTOPIC_NAME[]= "$aws/things/Water_Pump_new/shadow/update"; // publish topic name

int status = WL_IDLE_STATUS;
int msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];

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


void loop()
{
  flag = 0;
  water_level = analogRead(respin);
  
  //개물통에 물이 비어있음
  if(touchRead(4)>50){
     if(water_level>1000){
   
      Serial.println("모터 동작");
    
      digitalWrite(A1A, HIGH);
      digitalWrite(A1B, LOW);
      delay(5000);
    
      Serial.println("모터 멈춤");
      digitalWrite(A1A, LOW);
      digitalWrite(A1B, LOW);
      delay(1000); 
    
      water_level = analogRead(respin);
  
  //    if(flag == 0){
  //        flag = 1;
  //        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
  //
  //        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
  //          Serial.print("Publish Message:");
  //          Serial.println(payload);
  //        }
  //        else{
  //          Serial.println("Publish failed"); 
  //        }
  //      }
      sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
  
      if(testButton.publish(pTOPIC_NAME,payload) == 0) {
        Serial.print("Publish Message:");
        Serial.println(payload);
      }
      else{
        Serial.println("Publish failed"); 
      }
      
      Serial.print("물의 높이 : ");
      Serial.println(water_level);
    
      if(msgReceived == 1) {
        msgReceived = 0;
        Serial.print("Received Message:");
        Serial.println(rcvdPayload);
        // Parse JSON
        JSONVar myObj = JSON.parse(rcvdPayload);
        JSONVar state = myObj["state"];
        Serial.println(myObj);
    //        JSONVar desired = state["desired"];
    //        String pump = (const char*) desired["pump"];
    //        Serial.print("pump will be ");
    //        Serial.println(pump);
    //        if (pump == "ON"){
    //          Serial.println("pump on");
    ////          digitalWrite(ledPin, HIGH);
    //        }
    //        else if (pump == "OFF"){
    //          Serial.println("pump off");
    ////          digitalWrite(ledPin, LOW);
    //        }
      }
    }
    else {
      while(water_level<=1000){
        if(touchRead(4)<=50){
          break;
        }
  //      if(flag == 0){
  //        flag = 1;
  //        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
  //
  //        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
  //          Serial.print("Publish Message:");
  //          Serial.println(payload);
  //        }
  //        else{
  //          Serial.println("Publish failed"); 
  //        }
  //      }
        Serial.print("물을 채워주세요. ");
        water_level = analogRead(respin);
        Serial.print("물의 높이 : ");
        Serial.println(water_level);
        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
        Serial.println(touchRead(4));
  
        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
          Serial.print("Publish Message:");
          Serial.println(payload);
        }
        else{
          Serial.println("Publish failed"); 
        }
        delay(3000);
      } 
    }
      
    delay(3000);
  }
  //물이 차있음
  else {
    Serial.println("강아지의 물통에 물이 차있습니다.");
  }
 
//  if(water_level>1000){
//   
//    Serial.println("모터 동작");
//  
//    digitalWrite(A1A, HIGH);
//    digitalWrite(A1B, LOW);
//    delay(5000);
//  
//    Serial.println("모터 멈춤");
//    digitalWrite(A1A, LOW);
//    digitalWrite(A1B, LOW);
//    delay(1000); 
//  
//    water_level = analogRead(respin);
//
////    if(flag == 0){
////        flag = 1;
////        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
////
////        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
////          Serial.print("Publish Message:");
////          Serial.println(payload);
////        }
////        else{
////          Serial.println("Publish failed"); 
////        }
////      }
//    sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
//
//    if(testButton.publish(pTOPIC_NAME,payload) == 0) {
//      Serial.print("Publish Message:");
//      Serial.println(payload);
//    }
//    else{
//      Serial.println("Publish failed"); 
//    }
//    
//    Serial.print("물의 높이 : ");
//    Serial.println(water_level);
//  
//    if(msgReceived == 1) {
//      msgReceived = 0;
//      Serial.print("Received Message:");
//      Serial.println(rcvdPayload);
//      // Parse JSON
//      JSONVar myObj = JSON.parse(rcvdPayload);
//      JSONVar state = myObj["state"];
//      Serial.println(myObj);
//  //        JSONVar desired = state["desired"];
//  //        String pump = (const char*) desired["pump"];
//  //        Serial.print("pump will be ");
//  //        Serial.println(pump);
//  //        if (pump == "ON"){
//  //          Serial.println("pump on");
//  ////          digitalWrite(ledPin, HIGH);
//  //        }
//  //        else if (pump == "OFF"){
//  //          Serial.println("pump off");
//  ////          digitalWrite(ledPin, LOW);
//  //        }
//    }
//  }
//  else {
//    while(water_level<=1000){
//      
////      if(flag == 0){
////        flag = 1;
////        sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
////
////        if(testButton.publish(pTOPIC_NAME,payload) == 0) {
////          Serial.print("Publish Message:");
////          Serial.println(payload);
////        }
////        else{
////          Serial.println("Publish failed"); 
////        }
////      }
//      Serial.print("물을 채워주세요. ");
//      water_level = analogRead(respin);
//      Serial.print("물의 높이 : ");
//      Serial.println(water_level);
//      sprintf(payload,"{\"state\": {\"reported\": {\"pump\":%d}}}", water_level);
//      Serial.println(touchRead(4));
//
//      if(testButton.publish(pTOPIC_NAME,payload) == 0) {
//        Serial.print("Publish Message:");
//        Serial.println(payload);
//      }
//      else{
//        Serial.println("Publish failed"); 
//      }
//      delay(3000);
//    } 
//  }
//    
//  delay(3000);
}
