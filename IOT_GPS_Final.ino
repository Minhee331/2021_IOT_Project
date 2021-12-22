/////////// GPS ///////////
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
/////////// GPS ///////////

/////////// AWS and WiFi ///////////
#include <AWS_IOT.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <Wire.h>

AWS_IOT GPS;
/////////// AWS and WiFi ///////////

/////////// 전역 변수 선언  ///////////
// gps rx, tx
static const int RXPin = 17, TXPin = 16;
// gps baud
static const uint32_t GPSBaud = 9600;
// button 
static const int buttonPin = 15;
int buttonState = 0;
// led
const int ledPin = 22;
// control: 0이면 시작 대기, 1이면 종료 대기, 2면 와이파이 대기? & 시작 대기
int controlFlag = 0;
// aws, wifi connect flag
int wifiNawsFlag = 0;
// aws publishFlag
int publishFlag = 0;
// 경도 위도 저장 
double x = 0, y = 0, xx = 0, yy = 0;

double t_distance = 0; //총 산책 거리
int t_time = 0; //총 산책 시간
int l_start_time = 0; //최근 산책 시간
int l_end_time = 0; //최근 산책 시간
double l_distance = 0; //최근 산책 거리

// wifi and aws setup
const char* ssid = "jy-iptime";
const char* password = "westlangley23!";
//const char* ssid = "Jun";
//const char* password = "";
char HOST_ADDRESS[]="a2lbqogkjpa0xw-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "21_GPS";
char sTOPIC_NAME[]= "$aws/things/21_Server2/shadow/update/delta"; // subscribe topic name
char pTOPIC_NAME[]= "21_GPS/new"; 

// subscribe msg
int msgReceived = 0;
char payload[512];
char rcvdPayload[512];

int flag = 0;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
/////////// 전역 변수 선언  //////////

/////////// mySubCallBackHandler //////////
void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1; //flag
}
/////////// mySubCallBackHandler //////////

/////////// setUp //////////
void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPS++ with an attached GPS module"));
  Serial.print(F("Testing TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
}
/////////// Setup //////////

/////////// loop //////////
void loop() {
  // subscribe msg received 
  if(msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message:");
    Serial.println(rcvdPayload);
  }
  // 시작 대기 상태 
  if(controlFlag == 0){ // 시작대기
    buttonState = digitalRead(buttonPin); // button state read
    Serial.println("시작 대기 상태입니다. 버튼을 눌러주세요");
    if (buttonState == HIGH){
      flag = 0;
      digitalWrite(ledPin, HIGH);
      while(l_start_time==0){
        while (ss.available() > 0)
          if (gps.encode(ss.read()))
              if (gps.time.isValid())
                {
                  l_start_time = (int(gps.time.hour()))%24 * 60 + int(gps.time.minute());
                  Serial.print("산책 시작 시각: ");
                  l_start_time += 540;
                  Serial.println(l_start_time);
                }
      }
      controlFlag = 1; // 진행 중 and 종료 대기 상태로 넘어감 
      l_distance = 0; // 거리 초기화 
      digitalWrite(ledPin, HIGH);
      delay(1000);
    }
    delay(2000);
  }
  // 진행 중 and 종료 대기 상태 
  else if (controlFlag == 1) {
    
    buttonState = digitalRead(buttonPin); // button state read
    if (buttonState == HIGH){
      controlFlag = 2; // 와이파이 대기 상태로 넘어감
      while(l_end_time==0){
        while (ss.available() > 0)
          if (gps.encode(ss.read()))
              if (gps.time.isValid())
                {
                  l_end_time = (int(gps.time.hour()))%24 * 60 + int(gps.time.minute());
                  Serial.print("산책 종료 시각: ");
                  l_end_time += 540;
                  Serial.println(l_end_time);
                }
      }
      t_distance = t_distance + l_distance;
      t_time = t_time + (l_end_time - l_start_time);
      Serial.print("총 산책 거리");
      Serial.println(t_distance);
      Serial.print("총 산책 시간");
      Serial.println(t_time);\
    }
    digitalWrite(ledPin, HIGH);
    while (ss.available() > 0){ // gps read
      if (gps.encode(ss.read())){
        displayInfo();
      }
    }
    if (millis() > 5000 && gps.charsProcessed() < 10){ // gps no detected 
      Serial.println(F("No GPS detected: check wiring."));
      while(true);
    }
  }
  // 종료 상태 및 aws 연결 중
  else{
    digitalWrite(ledPin, LOW); // led off때
    if(wifiNawsFlag == 0){ // wifi와 aws 연결 안했을 때 
      //// wifi 연결 //////
      Serial.print("WIFI status = ");
      Serial.println(WiFi.getMode());
      WiFi.disconnect(true);
      delay(1000);
      WiFi.mode(WIFI_STA);
      delay(1000);
      Serial.print("WIFI status = ");
      Serial.println(WiFi.getMode());
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
      }
      Serial.println("Connected to wifi");
      Serial.println(WiFi.localIP());
      //// wifi 연결 //////
      //// aws  연결 //////
      if(GPS.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
        Serial.println("Connected to AWS");
        delay(1000);
        if(0==GPS.subscribe(sTOPIC_NAME,mySubCallBackHandler)) {
          Serial.println("Subscribe Successfull");
          wifiNawsFlag = 1;
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
      //// aws  연결 //////
    }
    else if(publishFlag == 0){ // aws 퍼블리쉬 안했을 때 
      sprintf(payload,"{\"state\": {\"reported\": {\"t_distance\":%d, \"t_time\":%d }}}", int(t_distance), t_time);
      if(GPS.publish(pTOPIC_NAME,payload) == 0) {
        Serial.print("Publish Message:");
        Serial.println(payload);
        publishFlag = 1;
        // control: 0이면 시작 대기, 1이면 종료 대기, 2면 와이파이 대기? & 시작 대기
        controlFlag = 0;
        // aws, wifi connect flag
        wifiNawsFlag = 0;
        // aws publishFlag
        publishFlag = 0; 
        // 경도 위도 저장 
        x = 0;y = 0; xx = 0; yy = 0;
        
        l_start_time = 0; //최근 산책 시간
        l_end_time = 0; //최근 산책 시간
        l_distance = 0; //최근 산책 거리
      }
      else{
        Serial.println("Publish failed"); 
      }
    }
      
  }
}
/////////// loop //////////

////////// display location //////////
void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid()) {
    Serial.print("현재 위치: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.println(gps.location.lng(), 6);
    x = gps.location.lat() * 1000000;
    y = gps.location.lng() * 1000000;
    Serial.print(x*0.000001, 6);
    Serial.print(F(","));
    Serial.print(y*0.000001, 6);
    Serial.println();
    if(flag == 0){
      xx = x;
      yy = y;
      flag = 1; 
      Serial.print("flag == 0");
    }else if(flag == 1){
      l_distance = l_distance + (double)TinyGPSPlus::distanceBetween(x*0.000001, y*0.000001, xx*0.000001, yy*0.000001);
      xx = 0;
      flag = 0;
      Serial.print("flag == 1");
    }
    Serial.print("l_distance: ");
    Serial.println(l_distance);
    delay(10000);
  }else {
    Serial.print(F("INVALID"));
  }
  Serial.println();
}
////////// display location //////////
