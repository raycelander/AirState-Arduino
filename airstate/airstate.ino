#include <MemoryFree.h>
#include <SI7021.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SSD1306_text.h>
#include <stdlib.h>
#include <math.h>

SI7021 sensor;
SoftwareSerial wifi(2, 3); // RX, TX
si7021_env data;

#define OLED_DATA 9
#define OLED_CLK 10
#define OLED_DC 11
#define OLED_CS 12
#define OLED_RST 13
SSD1306_text oled(OLED_DATA, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

const String POM = "Aussen";
const String IP = "191.233.85.165";
const String SSID = "rayNet";
const String PW = "rton-hhw3-931b-o6c2";

String cmd = "";
int temp = 0;
float h = 0.00;
float t = 0.00;
float hpa = 0.00;
float hum = 0.00;
String status = "";
boolean isDataSent = false;
boolean isSensorOk = false;

void setup(){
  oled.init();
  oled.clear();   
  Wire.begin();
  lcdPrint("Airstate 1.0");
}

void loop() {
  Serial.begin(115200);
  Serial.setTimeout(15000);

  if (checkWiFiModule()){
    if (connectWiFi()){
        delay(1000);
        if (readDatas()){
          if (sendDatas()){
            printSucc();
            Serial.end();
            delay(600000);
          }else{
            printErr(status);
          }
        }else{
          printErr("temp sensor failed");
        }
      }else{
        printErr("wifi con failed");
      }
  }else{
    printErr("Module not ready");
  }
}

void printErr(String message){
    lcdPrintLine(message,0,0);
    Serial.end();
    delay(1000);
}

void printSucc(){
  oled.clear();
  oled.setCursor(2, 0);        // move cursor to row 3, pixel column 10
  oled.setTextSize(2, 2);       // 3X character size, spacing 5 pixels
  oled.print(String(t) + " C");
  oled.setCursor(5, 0);  
  oled.print(String(h) + " %");
  oled.setTextSize(1,1);
  oled.setCursor(0,0);
  oled.print("Ram:" + String(freeMemory()) + " (" + status + ")");
}

boolean checkWiFiModule(){
  Serial.println("AT+RST"); // reset and test if module is ready
  delay(1000);
  return Serial.find("OK");
}

boolean connectWiFi() {
  lcdPrint("connecting WiFi..");
  for (int i = 0; i < 5; i++) {
    Serial.println("AT+CWMODE=1");
    Serial.flush();
    delay(1000);
    cmd = "AT+CWJAP=\"";
    cmd += SSID;
    cmd += "\",\"";
    cmd += PW;
    cmd += "\"";
    Serial.println(cmd);
    Serial.flush();
    delay(1000);
    if (Serial.find("OK")) {
      return true;
    }
  }
  return false;
}

boolean readDatas(){

  for (int i = 0;i<10;i++){
      lcdPrintLine("reading " + String(i+1) + "/10",0,0);
     if (sensor.begin()){
        isSensorOk = true;
        break;
     }
     delay(2000);
  }
  
  if (!isSensorOk){
    return false;
  }
  
  lcdPrintLine("reading temp...",0,0);
  delay(1000);
  temp = sensor.getCelsiusHundredths();
  t = temp/100;
  lcdPrintLine("reading hum...",0,0);
  delay(1000);
  h = sensor.getHumidityPercent();
  lcdPrintLine("measuring done",0,0);
  return true;
}

boolean sendDatas() {
 lcdPrint("sending datas ");
  Serial.println("AT+CIPMUX=1"); // set to multi connection mode
  Serial.flush();
  if ( Serial.find( "Error")) {
    lcdPrint( "Err connection" );
    return false;
  }

  cmd = "AT+CIPSTART=4,\"TCP\",\"" + IP + "\",80";
  Serial.println(cmd);
  Serial.flush();
  if (Serial.find("OK")) {
    lcdPrint("Connection open");
    delay(1000);
  }else{
    lcdPrint("Connection failed");
    delay(1000);
    return false;
  }

  cmd = "GET /?pom="+POM+"&temp="+String(t)+"&hum="+String(h)+ "&hpa="+String(hpa) + "  HTTP/1.0\r\nHost: fireproxy.azurewebsites.net\r\n\r\n";

  delay(2000);
  Serial.println("AT+CIPSEND=4," + String(cmd.length()));
  Serial.flush();
  isDataSent = false;
  if (Serial.find(">")) {
    lcdPrint("Sending datas...");
    Serial.println(cmd);
    delay(100);
    if (Serial.find("OK")) {
      status = "Ok";
      isDataSent = true;
      //delay(5000);
    } else {
      status = "data sent";
    }
  } else {
    status = "timeout";
  }
  
  delay(1000);
  lcdPrint("disconnecting");
  Serial.println("AT+CIPCLOSE");
  delay(1000);
  Serial.end();
  
  return isDataSent;
}

void lcdPrint(String msg) {
  oled.clear();
  oled.setCursor(0, 0);
  oled.setTextSize(1,1);        // 5x7 characters, pixel spacing = 1
  oled.print(msg);
}

void lcdPrintLine(String msg, int row, int column) {
  oled.setTextSize(1, 1);
  for (int i = 0; i< 128;i++){
    oled.setCursor(row,i);
    oled.print(" ");
  }
  oled.setCursor(row, column);        // move cursor to row 3, pixel column 10
  oled.print(msg);
}
