
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> 
#include <TimeLib.h>
#include <Timezone.h>
#include "LedControl.h"

#define DISP_DATA_IN 5
#define DISP_CLOCK 4
#define DISP_LOAD 12
#define DISP_NUM_MAX_DEVICES 1
#define PIR 14 


//declartions
void printNumber(byte pos, uint32_t v, int base, int digits);
void printNumber(byte pos, int16_t v, int base);
void printTime(byte pos);

//data in, clock, latch, num of devices
LedControl lc=LedControl(DISP_DATA_IN,DISP_CLOCK,DISP_LOAD, DISP_NUM_MAX_DEVICES); 
WiFiUDP ntpUDP;
WiFiManager wifiManager;
WiFiClientSecure espClient;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone tz(CEST, CET);


int i = 0;
int dir = 0;

void setup_wifi() {
  String espIdHexa = String(ESP.getChipId(), HEX);
  String ap = String(WiFi.SSID());
  delay(10);

  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WiFi.SSID());

  WiFi.begin();
  for(int i=0;i<10 && WiFi.status() != WL_CONNECTED; i++){
    Serial.print(".");
    delay(1000);
  }

  //fallback to web config - only if btn pressed
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("starting wifimanager");
    
    

    ap = "clk-"+espIdHexa;
    String pwd = espIdHexa + "000";
    wifiManager.setConfigPortalTimeout(60);
    Serial.println(ap);
    Serial.println(pwd);
    wifiManager.autoConnect(ap.c_str(), pwd.c_str());  
  }else{
    WiFi.mode(WIFI_STA);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while(!timeClient.update()){
    timeClient.forceUpdate();
    
  }

  espClient.setX509Time(timeClient.getEpochTime());
  setTime(timeClient.getEpochTime()); // set current system time
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR, INPUT);
  
  lc.shutdown(0, false); 
  lc.setIntensity(0,15); // set max brightness for the Leds
  lc.setScanLimit(0,8); // all digits in use

  setup_wifi();

  lc.setDigit(0, 0, 8, 1);
  delay(250);
  lc.setDigit(0, 1, 8, 1);
  delay(250);
  lc.setDigit(0, 2, 8, 1);
  delay(250);
  lc.setDigit(0, 3, 8, 1);
  delay(250);
  lc.setDigit(0, 4, 8, 1);
  delay(250);
  lc.setDigit(0, 5, 8, 1);
  delay(250);
  lc.setDigit(0, 6, 8, 1);
  delay(250);
  lc.setDigit(0, 7, 8, 1);
  delay(1000);
  uint32_t id = ESP.getChipId();
  Serial.print("chip id ");
  Serial.println(id, HEX);
  printNumber(0, id & 0xFFFF, 16, 4);
  printNumber(4, id >> 16, 16, 4);
  delay(5000);
  lc.clearDisplay(0);
}

bool onOff = true;
void loop() {
  time_t t = tz.toLocal(timeClient.getEpochTime());
  printNumber(2, (uint32_t)second(t), 10, 2 );
  printTime(4, t);
  //printNumber(3, ((millis()/100)%10), 10, 1);
  lc.setIntensity(0, analogRead(A0)/64);

  bool pirState = digitalRead(PIR);
  if(pirState != onOff){
      onOff = digitalRead(PIR);
      lc.shutdown(0, !pirState); 
  }
}

void printTime(byte pos, time_t t) {  
    int ones;  
    int tens;  
    int hundreds; 
    int thousands;
    
    uint32_t v = (hour(t) * 100)+minute(t);
    
    ones=v%10;  
    v=v/10;  
    tens=v%10;  
    v=v/10;
    hundreds=v%10;
    v=v/10;
    thousands = v;  
    
    bool dp = millis()%1000 > 500;

    if(thousands>0){
      lc.setDigit(0,pos,(byte)thousands,false);        
    }else{
      lc.setChar(0, pos, 32, false);
    }
    //Now print the number digit by digit 
    if(hundreds>0 || thousands > 0){
      lc.setDigit(0,pos+1,(byte)hundreds,dp);
    }else{
      lc.setChar(0, pos+1, 32, dp);
    }
    if(tens>0 || hundreds >0 || thousands > 0){
      lc.setDigit(0,pos+2,(byte)tens,false);
    }else{
      lc.setChar(0, pos+2, 32, false);
    }
    lc.setDigit(0,pos+3,(byte)ones,false); 
} 

void printNumber(byte pos, uint32_t v, int base, int digits) {  
    int ones;  
    int tens;  
    int hundreds; 
    int thousands;
    int p = pos;
    
    ones=v%base;  
    v=v/base;  
    tens=v%base;  
    v=v/base;
    hundreds=v%base;
    v=v/base;
    thousands = v;  
    
    if(digits>=4){
      if(thousands>0){
        lc.setDigit(0,p,(byte)thousands,false);        
      }else{
        lc.setChar(0, p, 32, false);
      }
      p++;
    }
    //Now print the number digit by digit 
    if(digits>=3){
      if(hundreds>0 || thousands > 0){
        lc.setDigit(0,p,(byte)hundreds,false);
      }else{
        lc.setChar(0, p, 32, false);
      }
      p++;
    }

    if(digits>=2){
      if(tens>0 || hundreds >0){
        lc.setDigit(0,p,(byte)tens,false);
      }else{
        lc.setChar(0, p, 32, false);
      }
      p++;
    }
    
    lc.setDigit(0,p,(byte)ones,false);
}

void printNumber(byte pos, int16_t v, int base) {  
    int ones;  
    int tens;  
    int hundreds; 
    int thousands;
    boolean negative=false;
    /*
    if(v < -999 || v > 9999)  
        return;  
        */
    if(v<0) {  
        negative=true; 
        v=v*-1;  
    }
    ones=v%base;  
    v=v/base;  
    tens=v%base;  
    v=v/base;
    hundreds=v%base;
    v=v/base;
    thousands = v;  
    if(negative) {  
        //print character '-' in the leftmost column  
        lc.setChar(0,pos,'-',false);  } 
    else {
        //print a blank in the sign column  
        lc.setChar(0,pos,(byte)thousands,false);  
    }  
    //Now print the number digit by digit 
    lc.setDigit(0,pos+1,(byte)hundreds,false);
    lc.setDigit(0,pos+2,(byte)tens,false); 
    lc.setDigit(0,pos+3,(byte)ones,false); 
} 
