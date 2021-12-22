#include <LiquidCrystal_I2C.h>
#include "HX711.h"
#include <AWS_IOT.h>
#include "WiFi.h"
#include <Arduino_JSON.h>
#include <Wire.h>


/////WIFI & AWS////////////////
const char* ssid = "jy-iptime";
const char* password = "westlangley23!";
char HOST_ADDRESS[]="a2lbqogkjpa0xw-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "21_Server";
char sTOPIC_NAME[] = "$aws/things/21_Server2/shadow/update/delta";
char pTOPIC_NAME[]= "21_Weight/new"; 
char pTOPIC2_NAME[]= "21_Water_Pump/button"; 
char pTOPIC3_NAME[]= "21_Motor/button"; 

AWS_IOT Weight;
//AWS_IOT testButton; // water
/////WIFI & AWS////////////////

/////Web Server////////////////////////
WiFiServer server(12000);
String header;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
/////Web Server////////////////////////


//무게 측정 센서 ////////////////
#define calibration_factor -280397 //kg 기준
//#define calibration_factor -5397 //g 기준
#define DOUT  4 //데이터 핀
#define CLK  18 // 클럭 핀
HX711 scale;
//무게 측정 센서 ////////////////


//LCD 센서
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);   
///////////////////////////

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

int feed_total = 0;
int feed_count = 0;
int recommendedAmount = 0;

int remain_water_msg = 0;//0일때 부족 1일때 충분
int remain_water_bottle = 0;
int remain_water_bowl = 0;
int water_btn = 0;
int water_location = 0;
int motor_location = 0;

int r_distance = 0; //추천 산책 거리
int t_distance = 0; //총 산책 거리
int t_time = 0; //총 산책 시간
int l_start_time = 0; //최근 산책 시간
int l_end_time = 0; //최근 산책 시간
double l_distance = 0; //최근 산책 거리

int pump = 0;

void setup() {
  Serial.begin(115200);  // 값 모니터링 위해서…
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor); 
  scale.tare();  //영점잡기. 현재 측정값을 0으로 둔다.

  ////////////////////////////////////
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
  if(Weight.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
    Serial.println("Weight Connected to AWS");
    delay(1000);
    if(0==Weight.subscribe(sTOPIC_NAME,mySubCallBackHandler)) {
      Serial.println("Weight Subscribe Successfull");
    }
    else {
      Serial.println("Weight Subscribe Failed, Check the Thing Name and Certificates");
      while(1);
    }
  }
  else {
    Serial.println("Weight AWS connection failed, Check the HOST Address");
    while(1);
  }
  /////////////////////////////////////////
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  ////////////////////////////////////////
  lcd.begin();
  lcd.clear();                     
  lcd.backlight();  // turn on LCD backlight 
  /////////////////////////////////////////

  
}

int count = 0;
String nameVal = "";
String weightVal = "";
String ageVal = "";

void loop() {
  float weight = scale.get_units();
  sprintf(payload,"{\"state\": {\"reported\": {\"weight\":%.2f}}}",weight);
    delay(5000);
    if(Weight.publish(pTOPIC_NAME,payload) == 0) {
      Serial.print("Publish Message:");
      Serial.println(payload);
    }
    else{
      
      Serial.println("weight Publish failed");
    }
  delay(3000);
  if(msgReceived == 1){
    Serial.print("Received weight Message:");
    Serial.println(rcvdPayload);
    JSONVar myObj = JSON.parse(rcvdPayload);
    JSONVar state = myObj["state"];
    remain_water_bottle = int(state["pump"]);
    remain_water_bowl = int(state["remain_water_bowl"]);
    if(remain_water_bottle <= 1000){
      remain_water_msg = 0;
    }else{
      remain_water_msg = 1;
    }

    feed_total = state["feed_total"];
    feed_count = state["feed_count"];

    t_distance = state["t_distance"]; //총 산책 거리
    t_time = state["t_time"]; //총 산책 시간

  }
  WiFiClient client = server.available(); // Listen for incoming clients
  if (client) { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) { // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c); // print it out the serial monitor
        header += c;
        if (c == '\n') { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if(header.indexOf("GET /water/on") >= 0){
              water_btn = 1;
              while(1){
                sprintf(payload,"{\"state\": {\"reported\": {\"button\":\"on\"}}}");
                if(Weight.publish(pTOPIC2_NAME,payload) == 0) {
                  Serial.print("Publish Message:");
                  Serial.println(payload);
                  water_btn = 0;
                  water_location = 1;
                  break;
                }
                else{
                  Serial.println("water button Publish failed");
                }
              }
            }else if(header.indexOf("GET /motor/on") >= 0){
              while(1){
                sprintf(payload,"{\"state\": {\"reported\": {\"button\":\"on\"}}}");
                if(Weight.publish(pTOPIC3_NAME,payload) == 0) {
                  Serial.print("Publish Message:");
                  Serial.println(payload);
                  motor_location = 1;
                  break;
                }
                else{
                  Serial.println("motor button Publish failed");
                }
              }
            }
            else if (header.indexOf("GET /info") >= 0) {
              int nameIdx = header.indexOf("?name=");
              int weightIdx = header.indexOf("&weight=");
              int ageIdx = header.indexOf("&age=");
//              inputMessage = server.arg('name');
              nameVal = header.substring(nameIdx+6, weightIdx);
              weightVal = header.substring(weightIdx+8, ageIdx);
              ageVal = header.substring(ageIdx+5, header.indexOf(" HTTP"));
              Serial.print("name: ");
              Serial.println(nameVal);
              Serial.print("weight: ");
              Serial.println(weightVal);
              Serial.print("age: ");
              Serial.println(ageVal);
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; padding:0px; margin: 0px; text-align: center;} body{margin:0;}");
            client.println("p{font-weight:700;font-size:20px;color:#3c71c2;margin:8px}.container{display:flex;flex-direction:column;box-sizing:border-box;height:100vh;width:100%;padding:20px;background-color:#92b3e4;min-width:500px}.contain{display:flex;flex-direction:row;box-sizing:border-box;height:100%;width:100%;background-color:#92b3e4;min-width:500px}.header{display:flex;flex-direction:column;box-sizing:border-box;justify-content:center;align-items:center;width:100%;transition:color .3s ease 0s;background-color:#fff;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:20px;margin-right:20px;margin-bottom:20px}.form{display:flex;flex-direction:column;justify-content:center;align-items:center;font-weight:700}.button{margin-top:10px}.button>input{border:none;background-color:#82a7df;color:#fff;border-radius:10px;height:40px;font-weight:700;padding:4px 16px}.button>input:hover{background-color:#3c71c2;cursor:pointer}.food{display:flex;flex-direction:column;box-sizing:border-box;align-items:center;width:50%;height:100%;transition:color .3s ease 0s;background-color:#fff;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:40px;margin-right:20px}.water{display:flex;flex-direction:column;box-sizing:border-box;align-items:center;width:50%;height:100%;transition:color .3s ease 0s;background-color:#fff;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:40px;margin-right:20px}.walk{display:flex;flex-direction:column;box-sizing:border-box;align-items:center;width:50%;height:100%;transition:color .3s ease 0s;background-color:#fff;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:40px}.info{display:flex;flex-direction:row;justify-content:center;align-items:center;width:100%;height:70%}.feedamount{display:flex;flex-direction:column;box-sizing:border-box;justify-content:center;align-items:center;text-align:center;width:100%;height:80%;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:10px;margin-left:10px;margin-right:10px}.feedbutton{display:flex;box-sizing:border-box;align-items:center;color:#fff;font-weight:700;background-color:#82a7df;border:none;border-radius:8px;padding:10px 20px 10px 20px}.feedbutton:hover{background-color:#3c71c2;cursor:pointer}.water_remaining{display:flex;box-sizing:border-box;align-items:center;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:10px 20px 10px 20px}.waterfeeded{display:flex;box-sizing:border-box;align-items:center;box-shadow:0 2px 10px 1px rgba(0,0,0,.1);border-radius:20px;padding:10px 20px 10px 20px}</style></head>");
            
            client.println("<div class='container'><div class='header'><h3>똑똑한 도그생활</h3>");

            if(nameVal ==""){
              client.println("<form action='info' method='get' class='form'><div><label for='name'>반려견 이름:</label> <input type='text' name='name' id='name' required></div><div><label for='weight'>반려견 무게:</label> <input type='text' name='weight' id='weight' required></div><div><label for='age'>반려견 나이:</label> <input type='text' name='age' id='age' required></div><div class='button'><input class='save' type='submit' value='저장하기'></div></form></div>");
            }else{
              client.println("<p>이름: ");
              client.println(nameVal);
              client.println("</p><p>나이: ");
              client.println(ageVal);
              client.println("살</p><p>무게: ");
              client.println(weightVal);
              client.println("kg</p></div>");
            }

            if(ageVal ==""){
              client.println("<div class='contain'><div class='food'><h2>사료 급여 관리</h2>");
              client.println("<h3>");
              client.println(nameVal);
              client.println("의 추천 사료량은");
              client.println("0g입니다</h3>");
              client.println("<div class='info'><div class='feedamount'><h4>급여량</h4>");
              client.println("<p>");
              client.println("0");
              client.println(" / ");
//              client.println(recommendedAmount);
              client.println("0g</p></div>");
              client.println("<div class='feedamount'><h4>급여횟수</h4><p>");
              client.println("0");
              client.println("회</p></div></div>");
              client.println("<a href=\"/motor/on\"><button class='feedbutton'>사료 급여하기</button></a>");
              client.println("</div>");
            }
            else{
              client.println("<div class='contain'><div class='food'><h2>사료 급여 관리</h2>");
              client.println("<h3>");
              client.println(nameVal);
              client.println("의 추천 사료량은");
              if(ageVal.toInt()<1){
                recommendedAmount = weightVal.toInt()*1000*0.04;
                client.println(recommendedAmount);
              }
              else if(ageVal.toInt()<=2){
                recommendedAmount = weightVal.toInt()*1000*0.03;
                client.println(recommendedAmount);
              }
              else{
                recommendedAmount = weightVal.toInt()*1000*0.02;
                client.println(recommendedAmount);
              }
              client.println("g입니다</h3>");
              client.println("<div class='info'><div class='feedamount'><h4>급여량</h4>");
              client.println("<p>");
              client.println(feed_total);
              client.println(" / ");
              client.println(recommendedAmount);
              client.println("g</p></div>");
              client.println("<div class='feedamount'><h4>급여횟수</h4><p>");
              client.println(feed_count);
              client.println("회</p></div></div>");
              client.println("<a href=\"/motor/on\"><button class='feedbutton'>사료 급여하기</button></a>");
              
              client.println("</div>");  
            }
            
            client.println("<div class='water'> <h2>물 급수 관리</h2><h3>남은 물의 양이");
            if(remain_water_msg==0){
              client.println("부족");
            }
            else{
              client.println("충분");
            }
//            client.println(충분 아님 부족);
            client.println("합니다</h3>");
            client.println("<div class='info'><div class='feedamount'><h4>물통에 남은 물의 양</h4><p>");
            client.println(remain_water_bottle);
            client.println("ml</p></div>");
            client.println("<div class='feedamount'><h4>물 그릇 상태</h4><p>물");
            if(remain_water_bowl==0){
              client.println("없음");
            }
            else{
              client.println("있음");
            }
//            client.println(없음 / 있음);
            if(water_btn == 0){
              client.println("</p></div></div><a href=\"/water/on\"><button class='feedbutton'>물 급여하기</button></a>");
            }else{
              client.println("</p></div></div><button class='feedbutton'>물 급여 중</button>");
            }
            

            
            client.println("</div><div class='walk'><h2>산책 관리</h2><h3>");
            client.println(nameVal);
            client.println("의 추천 산책 거리는");
            if(weightVal != ""){
              int w = weightVal.toInt();
              if(w < 7){
                r_distance = 2500;
              }else if(w <15){
                r_distance  = 4000;
              }else{
                r_distance  = 6000;
              }
            }
            client.println(r_distance);
            client.println("m 입니다. </h3> <div class='info'><div class='feedamount'><h4>오늘 총 산책 거리</h4><p>");
            client.println(t_distance);
            client.println("/");
            client.println(r_distance);
            client.println("m</p></div>");
            client.println("<div class='feedamount'><h4>오늘 총 산책 시간</h4><p>");
            client.println(t_time/60);
            client.println(":");
            client.println(t_time%60);
            client.println("</p></div>");
            
            client.println("</div></div></div></div>");

            
//            client.println("<div class='contain'><div class='food'><h2>Food</h2><div class='info'><div class='feedamount'>사료 급여량</div><div class='feednumber'>급여 횟수</div></div><button class='feedbutton'>사료 급여하기</button></div><div class='water'><h2>Water</h2><div class='info'><div class='feedamount'>남은 물의 양</div><div class='feednumber'>물 급여 횟수</div></div><button class='feedbutton'>물 급여하기</button></div><div class='walk'><h2>Walk</h2></div></div>");
            client.println("</body><script>");
            if(water_location == 1){
              water_location = 0;
              client.println("location.href = \"http://192.168.0.33:12000\";");
            }
            if(motor_location == 1){
              motor_location = 0;
              client.println("location.href = \"http://192.168.0.33:12000\";");
            }
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } //** if (currentLine.length() == 0) {
          else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } //** if (c == '\n') {
        else if (c != '\r') { // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      } //* if (client.available()) {
    } //** while
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  
  
  Serial.print(weight);
  Serial.println(" kg");
  
  lcd.setCursor(0, 0);
  lcd.print("Today : ");
  lcd.print(feed_total);
  lcd.print(" g");
  lcd.setCursor(0,1);
  lcd.print("Total : ");
  lcd.print(recommendedAmount);
  lcd.print(" g");
  delay(3000);
  
  
  
  

  // 무게 받기 
  
  
    // Parse JSON
//    JSONVar myObj = JSON.parse(rcvdPayload);
//    JSONVar current = myObj["current"];
//    JSONVar state = current["state"];
//    JSONVar receive_a = state["reported"];
//    JSONVar pump = receive_a["weight"];
//    Serial.println(pump);
    // 무게 받기 끝 
  
}
