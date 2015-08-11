#include <SI7021.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <math.h>
#define ONE_WIRE_BUS 2
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

SI7021 sensor;
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
SoftwareSerial wifi(2, 3); // RX, TX
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

const int powerDHT22 = 12;
String POM = "Aussen";
String IP = "191.233.85.165";
String SSID = "rayNet";
String PW = "rton-hhw3-931b-o6c2";
String cmd = "";
float h = 0.00;
float t = 0.00;
float hpa = 0.00;
float hum = 0.00;
String status = "";
si7021_env data;
boolean isDataSent = false;

void setup(){
  Wire.begin();
  lcdPrint("Starting up...");
  lcd.begin(20, 4);
}

void loop() {
  Serial.begin(115200);
  Serial.setTimeout(15000);

  if (checkWiFiModule()){
      if (connectWiFi()){
        delay(1000);
        if (bmp.begin()){
          readPressure();
        }
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

void readPressure(){
  lcdPrint("reading hpa");
  delay(1000);
  sensors_event_t event;
  bmp.getEvent(&event);
 
  /* Display the results (barometric pressure is measure in hPa) */
  if (event.pressure)  {
    /* Display atmospheric pressure in hPa */
    hpa = event.pressure; 
  }  else  {
    hpa = 0;
  }
}

void printErr(String message){
    lcdPrintLine(message,0,0);
    lcdPrintLine("retrying..",1,0);
    Serial.end();
    delay(1000);
}

void printSucc(){
  lcdPrint("");
  lcdPrintLine("Temp: " + String(t),0,0);
  lcdPrintLine("Hum : " + String(h),1,0);
  lcdPrintLine("hPa: " + String(hpa),2,0);
  lcdPrintLine(status,3,0);
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
  sensor.begin();
  lcdPrint("measuring...");
  data = sensor.getHumidityAndTemperature();
  t = data.celsiusHundredths/float(100);
  hum = data.humidityBasisPoints;
  h = int(round(float(hum)/float(100)));
  lcdPrint("measuring done");
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
    lcdPrint("conn open");
    delay(1000);
  }else{
    lcdPrint("conn failed");
    delay(1000);
    return false;
  }

  cmd = "GET /?pom="+POM+"&temp="+String(t)+"&hum="+String(h)+ "&hpa="+String(hpa) + "  HTTP/1.0\r\nHost: fireproxy.azurewebsites.net\r\n\r\n";

  delay(2000);
  Serial.println("AT+CIPSEND=4," + String(cmd.length()));
  Serial.flush();
  isDataSent = false;
  if (Serial.find(">")) {
    lcdPrint("con ready. sending.");
    Serial.println(cmd);
    delay(100);
    if (Serial.find("OK")) {
      status = "Success";
      isDataSent = true;
      delay(5000);
    } else {
      status = "datas transmitted";
    }
  } else {
    status = "Connection timeout";
  }
  
  delay(1000);
  lcdPrint("disconnecting");
  Serial.println("AT+CIPCLOSE");
  delay(1000);
  Serial.end();
  
  return isDataSent;
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
