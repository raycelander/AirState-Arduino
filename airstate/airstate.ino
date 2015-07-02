#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "DHT.h"
#define DHTPIN 13
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
SoftwareSerial wifi(2, 3); // RX, TX
String POM = "Aussen";
String IP = "191.233.85.165";
String SSID = "rayNet";
String PW = "rton-hhw3-931b-o6c2";
String cmd = "";
float h = 0.00;
float t = 0.00;
String status = "";
boolean connected = false;
void setup()
{
  lcdPrint("Starting up...");
  dht.begin();
  lcd.begin(20, 4);
}

void loop() {
  Serial.begin(115200);
  Serial.setTimeout(15000);
  Serial.println("AT+RST"); // reset and test if module is ready
  Serial.flush();
  delay(1000);

  if (Serial.find("OK")) {
    lcdPrint("Module ready");
  } else {
    lcdPrint("Module not ready");
    status = "module failed";
    return;
  }

  lcdPrint("connecting wifi");
  delay(1000);
  connected = false;
  for (int i = 0; i < 5; i++) {
    if (connectWiFi()) {
      connected = true;
      lcdPrint("Connected to WiFi…");
      break;
    }
  }
  if (!connected) {
    lcdPrint("could not connect to WiFi…");
    status = "wifi failed";
    return;
  }
  delay(1000);
  sendDatas();
  delay(1000);
  lcdPrint("disconnecting");
  Serial.println("AT+CIPCLOSE");
  delay(1000);
  Serial.end();
  lcdPrint("");
  lcdPrintLine("Temp: " + String(t),0,0);
  lcdPrintLine("Hum : " + String(h),1,0);
  lcdPrintLine("--------------------",2,0);
  lcdPrintLine(status,3,0);
  delay(600000);
}

boolean connectWiFi() {
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
  } else {
    return false;
  }
}

boolean sendDatas() {
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
    lcdPrint("conn open");
    delay(1000);
  }else{
    lcdPrint("conn failed");
    delay(1000);
    return false;
  }
  lcdPrint("measuring...");

  for (int i=0;i<10;i++){
    h = dht.readHumidity();
    delay(500);
    t = dht.readTemperature();
    delay(500);
    if (!isnan(t) && !isnan(h)){
      break;
    }
    delay(1000);
  }


  
  lcdPrintLine("Temp: " + String(t),1,0);
  lcdPrintLine("Hum : " + String(h),2,0);
  delay(2000);
  cmd = "GET /?pom="+POM+"&temp="+String(t)+"&hum="+String(h)+"  HTTP/1.0\r\nHost: fireproxy.azurewebsites.net\r\n\r\n";
  delay(2000);
  Serial.println("AT+CIPSEND=4," + String(cmd.length()));
  Serial.flush();
  if (Serial.find(">")) {
    lcdPrint("con ready. sending.");
    Serial.println(cmd);
    delay(100);
    if (Serial.find("OK")) {
      lcdPrint("datas received");
      status = "success";
      delay(5000);
    } else {
      lcdPrint("datas not received");
      status = "data failed";
    }
  } else {
    lcdPrint("Connection timeout!");
    return false;
  }
  return true;
}

void lcdPrint(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
}
void lcdPrintLine(String msg, int line, int row) {
  lcd.setCursor(row, line);
  lcd.print(msg);
}
