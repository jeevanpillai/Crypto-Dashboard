#include <Arduino.h>
#include <U8g2lib.h>
#define USING_AXTLS
#include <ESP8266WiFi.h>
#include <WiFiClientSecureAxTLS.h>
using namespace axTLS;

#include "Seeed_mbedtls.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
WiFiClientSecure client;
#pragma GCC diagnostic pop

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <NTPClient.h>
#include <WiFiUdp.h>

#define USING_AXTLS
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "RAMESH"
#define STAPSK  "seeg13331"
#endif

#include <ESP8266Ping.h>

const PROGMEM char* host_luno = "api.luno.com";
const PROGMEM char* host_bin = "api.binance.com";
const PROGMEM char* host_forex = "api.exchangeratesapi.io";

const int httpsPort = 443;

// Use web browser to view and copy
const PROGMEM char* auth_luno = "API_KEY";
const PROGMEM char* auth_bin = "API_KEY";
const PROGMEM char* key = "API_KEY";

const PROGMEM int cs = 17;
const PROGMEM int dc = 2;
const PROGMEM int rs = 0;

const PROGMEM char *ssid     = "RAMESH";
const PROGMEM char *password = "seeg13331";
const PROGMEM long offset_ny = -14400;

const PROGMEM char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
WiFiUDP ntpUDP;
NTPClient time_ny(ntpUDP, "asia.pool.ntp.org", offset_ny);
NTPClient time_bin(ntpUDP, "asia.pool.ntp.org", 0);

String asset[10];
int luno_quant = 0;
int valid_assets;
double er;
String ticker[10];
//String ticker[] = {"MYR", "ETHMYR", "XRPMYR", "USDT", "BNBUPUSDT", "BNBUSDT", "ETHUPUSDT"};
double amount[10];
double price[10];
double price_old[10];
int price_change_delay = 8; //in hours
double nav;
double inv = 4613;
double profit = 0;
boolean startup = true;

//time variables
char timeNY[28];
//display variables 
int scroll_assets = 0;
String textLines[10];
char lunoVal[35];
char binVal[35];
char total[35];

U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R0, cs, dc , rs);

//void loop timer values
long last = millis(); //update crypto prices in void loop
long prev = millis(); //update time and display in void loop
long t_pos = millis(); //update oled pos in void loop
int pos = 5;  //x-axis offset of the display to reduce oled burnin, ranges from 5 to 45

void u_pos(void) {
  pos++;
  if (pos == 36) {
    pos = 5;
  }
  Serial.println(pos);
}

void reboot(void) {
    //depreciated
}

String url_luno = "/api/1/ticker?pair=";
String url_luno2 = "/api/1/trades?pair=";
String url_bin = "/api/v3/ticker/price?symbol=";
String url_bin2 = "/api/v3/klines?interval=1m&limit=1";

void readPrice() {
  Serial.println("\n=========readPrice()========");

  char json[200];
  DynamicJsonDocument doc(200);
  
  Serial.println("\n===========Luno===========");

  if(startup) printOled("Reading Prices - Luno$Connecting to Luno API&...");
  Serial.println("Connection to Luno API - ...");
  if (!client.connect(host_luno, httpsPort)) {
    Serial.println("Connection to Luno API - Failed");
    if(startup) printOled("Reading Prices - Luno$Connecting to Luno API&Failed");
    return;
  } else {
    Serial.println("Connection to Luno API - Success");
    if(startup) printOled("Reading Prices - Luno$Connecting to Luno API&Success!");
  }

  if(!startup) u8g2.drawStr(pos, 24, ">>> Updating Prices.          ");
  u8g2.sendBuffer();

  for(int i=0; i<luno_quant; i++){
    if(ticker[i].equals("MYR")){
      price[i] = 1;
      price_old[i] = 1;
    }else{
      client.print(String("GET ") + url_luno + ticker[i] + " HTTP/1.1\r\n" +
                   "Host: " + host_luno + "\r\n" +
                   "User-Agent: ESP8266\r\n" +
                   "Connection: keep-alive\r\n" +
                   "Authorization: " + auth_luno + "\r\n\r\n");
    
    
      Serial.println("\n"+ticker[i] + " request sent");
  
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println(ticker[i] + " headers received");
          break;
        }
      }
      
      String line = client.readStringUntil('\n');
      while(line.length() < 10 ){
        line = client.readStringUntil('\n');
      }
      //Serial.println(line);
      
      while (client.available()) {  //flushes client
        client.read();
        delay(5);
      }

      long long ts;
      line.toCharArray(json, 200);
      DeserializationError err = deserializeJson(doc, json);
      price[i] = doc["last_trade"].as<double>();
      ts = doc["timestamp"].as<long long>();
      Serial.println(ticker[i] + " - " + price[i]);
      if(startup) printOled("Reading Prices - Luno$Current:&"+ticker[i] + " - " + price[i]);
      Serial.println(" ");
      String ts_req = "&since=";
      long ts2 = ts/1000;
      ts_req.concat(ts2 - 3600*price_change_delay);
      ts_req.concat("000");

      //requesting trade data
      client.print(String("GET ") + url_luno2 + ticker[i] + ts_req +" HTTP/1.1\r\n" +
                   "Host: " + host_luno + "\r\n" +
                   "User-Agent: ESP8266\r\n" +
                   "Connection: keep-alive\r\n" +
                   "Authorization: " + auth_luno + "\r\n\r\n");

      Serial.println(ticker[i] + " old price request sent");
      //Serial.println(url_luno2 + ticker[i] + ts_req);
  
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println(ticker[i] + " old price headers received");
          break;
        }
      } 
      
      bool pre = true;
      while(client.available()){
        if(pre){
          char c = client.read();
          //Serial.print(c);
          if(c == '['){
            pre = false;
            //Serial.println(" ");
          }
        }else{
          String s = client.readStringUntil('}');
          s.concat("}");
          //Serial.println(s);
          char c = client.read();
          if(c == ']'){
            Serial.print("Data: "); 
            //Serial.println(s);
            int j = s.indexOf("price");
            int k = s.indexOf(",", j);

            price_old[i] = s.substring(j+8, k-1).toDouble();
            if(startup) printOled("Reading Prices - Luno$Previous:&"+ticker[i] + " - " + price_old[i]);
            Serial.print("Extracted Price: "); 
            Serial.printf("%.2f @ %ld000\n", price_old[i], ts2-3600);
            break;
          }
        }    
      }
      //flushes client
      while (client.available()) {
        client.read();
        delay(5);
      }     
    }
  }
  
  if(!startup) u8g2.drawStr(pos, 24, ">>> Updating Prices..         ");
  u8g2.sendBuffer();
  
  Serial.println("\n=========Binance=========");
  
  Serial.println("Connection to Binance API - ...");
  if(startup) printOled("Reading Prices - Binance$Connecting to Binance API&...");
  if (!client.connect(host_bin, httpsPort)) {
    Serial.println("Connection to Binance API - Failed");
    if(startup) printOled("Reading Prices - Binanceo$Connecting to Binance API&Failed");
    return;
  } else {
    Serial.println("Connection to Binance API - Success");
    if(startup) printOled("Reading Prices - Binance$Connecting to Binance API&Success!");
  }

  if(!startup) u8g2.drawStr(pos, 24, ">>> Updating Prices...        ");
  u8g2.sendBuffer();

  for(int i=luno_quant; i<valid_assets; i++){
    if(ticker[i].equals("USDT")){
      price[i] = er;
      price_old[i] = er;
    }else{
      client.print(String("GET ") + url_bin + ticker[i] + " HTTP/1.1\r\n" +
                 "Host: " + host_bin + "\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Connection: keep-alive\r\n" +
                 "X-MBX-APIKEY: " + auth_bin + "\r\n\r\n");

      Serial.println("\n"+ticker[i] + " request sent");
      
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println(ticker[i] + " headers received");
          break;
        }  
      
      }
      String line = client.readStringUntil('}');
      line.concat("}");
      while (client.available()) { 
        client.read(); 
        delay(5); 
        Serial.print(".");
      }

      line.toCharArray(json, 200);
      deserializeJson(doc, json);
      double pr = doc["price"].as<double>();
      price[i] = pr * er;    
      Serial.println(ticker[i] + " - " + price[i]); 
      if(startup) printOled("Reading Prices - Binance$Current:&"+ticker[i] + " - " + price[i]);
      Serial.println(" ");

      //request for old price
      String timeStamp = "&startTime=";
      while(time_bin.getEpochTime() < 1621959833){
        time_bin.update();
      }
      timeStamp.concat(time_bin.getEpochTime() - 3600*price_change_delay);
      timeStamp.concat("000");
      timeStamp.concat("&symbol=");
      timeStamp.concat(ticker[i]);

      //requesting trade data
      client.print(String("GET ") + url_bin2 + timeStamp + " HTTP/1.1\r\n" +
                 "Host: " + host_bin + "\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Connection: keep-alive\r\n" +
                 "X-MBX-APIKEY: " + auth_bin + "\r\n\r\n");

      Serial.println(ticker[i] + " old price request sent");
      //Serial.println(url_bin2 + timeStamp);

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println(ticker[i] + " old price headers received");
          break;
        }  
      
      }
      
      line = client.readStringUntil(']');
      line.concat("]");
      //Serial.println(line);
      line.toCharArray(json, 200);
      deserializeJson(doc, json);
      pr = (doc[0][1].as<double>() + doc[0][4].as<double>())/2;
      price_old[i] = pr * er;    
      Serial.print("Extracted Price: "); 
      Serial.print(price_old[i]);
      if(startup) printOled("Reading Prices - Binance$Previous:&"+ticker[i] + " - " + price_old[i]);
      Serial.println(" @ " +  doc[0][0].as<String>());
      
      while (client.available()) {  //flushes client
        client.read(); 
        delay(5); 
      } 
    }
  }

  if(!startup) u8g2.drawStr(pos, 24, ">>> Updating Prices....       ");
  u8g2.sendBuffer();

  Serial.println("\n===========NAV===========");
  nav = 0;
  String temp_text;
  int lineCount = 0;
  if(startup) printOled("Calculating NAV");
  for(int i=0; i<valid_assets; i++){
    temp_text = "";
    nav+= amount[i] * price[i];
    Serial.print("\nAsset : " );
    Serial.print(asset[i]);
    Serial.print("\nAmount: " );
    Serial.print(amount[i]);
    Serial.print("\nPrice : " );
    Serial.print(price[i]);
    Serial.print("\n -Value: MYR" );
    Serial.print(price[i]*amount[i]);
    if(startup) printOled("Calculating NAV$"+asset[i]+" - MYR "+price[i]*amount[i]);

    if(!asset[i].equals("MYR") && !asset[i].equals("USDT")){
    temp_text.concat(asset[i]);
    temp_text.concat(" : RM ");
    temp_text.concat(price[i]);
    temp_text.concat(" (");
    double change = ((price[i] - price_old[i])/price_old[i])*100;
    if(change > 0){
      temp_text.concat("+");
      temp_text.concat(String(change));
    }else{
      temp_text.concat(String(change));
    }
    temp_text.concat("%)");
    textLines[lineCount] = temp_text;
    lineCount++;
    Serial.print("\n -Temp Text: " );
    Serial.print(temp_text);
    Serial.print("\n");
    temp_text.concat(" | ");
    }else{
      Serial.print("\n");
    }
  }

  if(!startup) u8g2.drawStr(pos, 24, ">>> Updating Prices.....      ");
  u8g2.sendBuffer();

  if(startup) printOled("Updating Strings");
  profit = ((nav-inv)/inv) * 100;
  Serial.printf("\nNAV: MYR %.2f (+%.2f%%)\n", nav, profit); 

  Serial.println("\n============================");

  double sum = 0;
  double sum_old = 0;
  for(int i=0; i<luno_quant; i++){
  sum += amount[i] * price[i];
  sum_old += amount[i] * price_old[i];
  }
  double change = ((sum - sum_old)/sum_old)*100;
  int d1 = (int)sum / 1000;
  int d2 = (int)sum % 1000;
  int d3 = (int)(sum * 100) % 100;
  if(change > 0){
    sprintf(lunoVal, "Luno    : RM %d,%03d.%02d (+%.2f%%)", d1, d2, d3, change);
  }else{
    sprintf(lunoVal, "Luno    : RM %d,%03d.%02d (%.2f%%)", d1, d2, d3, change);
  }

  if(startup) printOled("Updating Strings.");
  sum = 0;
  sum_old = 0;
  for(int i=luno_quant; i<valid_assets; i++){
    sum += amount[i] * price[i];
    sum_old += amount[i] * price_old[i];
  }
  change = ((sum - sum_old)/sum_old)*100;
  d1 = (int)sum / 1000;
  d2 = (int)sum % 1000;
  d3 = (int)(sum * 100) % 100;
  if(change > 0){
    sprintf(binVal, "Binance : RM %d,%03d.%02d (+%.2f%%)", d1, d2, d3, change);
  }else{
    sprintf(binVal, "Binance : RM %d,%03d.%02d (%.2f%%)", d1, d2, d3, change);
  }

  if(startup) printOled("Updating Strings..");
  d1 = (int)nav / 1000;
  d2 = (int)nav % 1000;
  d3 = (int)(nav * 100) % 100;
  sprintf(total, "NAV : MYR %d,%03d.%02d (+%.2f%%)", d1, d2, d3, profit);
  
  if(startup) printOled("Setup Complete!");
}

void readBalances(void){
  int valid = 0;
  Serial.println(" ");
  Serial.println(ESP.getFreeHeap(),DEC);
  Serial.println("========Forex=============");
  WiFiClient exchange;
  printOled("HTTP Setup - Forex$Pinging API&...");
  //bool ret = Ping.ping("api.exchangeratesapi.io");
  bool ret = true;
  if (ret) {
    Serial.println("SERVER PING SUCCESS");
    printOled("HTTP Setup - Forex $Pinging API&Success");
  }else{
    Serial.println("SERVER PING FAIL");
    printOled("HTTP Setup - Forex$Pinging API&Fail");
  }
  
  if (!exchange.connect(host_forex, 80)) { //works!
    Serial.println("connection failed");
    //return;
  }

  String url = "/v1/latest?access_key=API_KEY&symbols=USD,MYR,JPY";
  printOled("HTTP Setup - Forex$Sending GET Request");
  exchange.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host_forex + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: keep-alive\r\n\r\n");

  String line;
  while (exchange.connected()) {
    line = exchange.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("forex headers received");
      printOled("HTTP Setup - Forex$Headers Received");
      break;
    }
  }

  printOled("HTTP Setup - Forex$Parsing API Response");
  exchange.readStringUntil('\n');
  line = exchange.readStringUntil('\n');
  //Serial.println(line);
  while (exchange.available()) {
    exchange.read();
    delay(5);
  }
  int idx = line.indexOf("MYR");
  int com_loc = line.substring(idx,idx+25).indexOf(",") + idx; 
  double myr = line.substring(idx+5, com_loc).toDouble();
  
  idx = line.indexOf("USD");
  com_loc = line.substring(idx,idx+25).indexOf(",") + idx; 
  double usd = line.substring(idx+5, com_loc).toDouble();
  er = myr/usd;
  if(er < 0.5){
    er = 4.15;
    printOled("@HTTPS Setup - Forex$API Read Failure&Exchange Rate: " +String(er)+ " MYR/USD");
  }else{
    printOled("@HTTPS Setup - Forex$API Read Success&Exchange Rate: " +String(er)+ " MYR/USD");
  }
  Serial.print("USD/MYR: ");
  Serial.println(er);
  delay(400);
  exchange.stop();

  Serial.println(" ");
  Serial.println(ESP.getFreeHeap(),DEC);
  Serial.println("========LUNO=============");
  printOled("HTTPS Setup - Luno$Pinging API&...");
  //ret = Ping.ping("api.luno.com");
  if (ret) {
    Serial.println("SERVER PING SUCCESS");
    printOled("HTTPS Setup - Luno$Pinging API&Success");
  }else{
    Serial.println("SERVER PING FAIL");
    printOled("HTTPS Setup - Luno$Pinging API&Fail");
  }
  printOled("HTTPS Setup - Luno$Attemping Connection to API");
  if (!client.connect(host_luno, httpsPort)) {
    Serial.println("Luno Connection failed");
    printOled("HTTPS Setup - Luno$Connection Failed");
    return;
  } else {
    Serial.println("Luno Connection successful");
    printOled("HTTPS Setup - Luno$Connection Successful");
  }

  printOled("HTTPS Setup - Luno$Building Query String");
  delay(20);
  url = "/api/1/balance";

  printOled("HTTPS Setup - Luno$Sending GET Request");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host_luno + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: keep-alive\r\n" +
               "Authorization: " + auth_luno + "\r\n\r\n");


  Serial.println(url + " request sent");

  line;
  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("luno headers received");
      printOled("HTTPS Setup - Luno$Headers Received");
      break;
    }
  }

  printOled("HTTPS Setup - Luno$Parsing API Response");
  client.readStringUntil('\n');
  line = client.readStringUntil('\n');
  while (client.available()) {
    client.read();
    delay(10);
  }

  char json[500];
  line.toCharArray(json, 500);
  Serial.println("Reading Luno");
  //Serial.println(line);
  DynamicJsonDocument doc(500);
  deserializeJson(doc, json);
  int loop_size = doc["balance"].size();
  //Serial.print("Loop: ");
  //Serial.println(loop_size);
  for(int i=0; i<loop_size; i++){
    if(!doc["balance"][i]["balance"].as<double>() < 0.01){
      asset[valid] = doc["balance"][i]["asset"].as<String>();
      amount[valid] = doc["balance"][i]["balance"].as<double>();
      printOled("HTTPS Setup - Luno$" + asset[valid] +" : " + amount[valid]);
      if(!asset[valid].equals("MYR")){
        scroll_assets++;
        ticker[valid] = asset[valid];
        ticker[valid].concat("MYR");
      }else{
        ticker[valid] = asset[valid];
      }
      valid++;
    }
  }
  luno_quant = valid;

  Serial.println(" ");
  Serial.println(ESP.getFreeHeap(),DEC);
  Serial.println("========BINANCE==========");
  printOled("HTTPS Setup - Binance$Pinging API&...");
  //ret = Ping.ping("api.binance.com");
  if (ret) {
    Serial.println("SERVER PING SUCCESS");
    printOled("HTTPS Setup - Binance$Pinging API&Success");
    delay(10);
  }else{
    Serial.println("SERVER PING FAIL");
    printOled("HTTPS Setup - Binance$Pinging API&Fail");
    delay(10);
  }

  
  printOled("HTTPS Setup - Binance$Attemping Connection to API");
  if (!client.connect(host_bin, httpsPort)) {
    Serial.println("Binance Connection failed");
    printOled("HTTPS Setup - Binance$Connection Failed");
    return;
  } else {
    Serial.println("Binance Connection successful");
    printOled("HTTPS Setup - Binance$Connection Successful");
  }

  printOled("HTTPS Setup - Binance$Building Query String");
  time_bin.update();
  String timeStamp = "timestamp=";
  while(time_bin.getEpochTime() < 1621959833){
    time_bin.update();
    Serial.println(time_bin.getEpochTime());
  }
  timeStamp.concat(time_bin.getEpochTime());
  timeStamp.concat("000");
  timeStamp.concat("&recvWindow=20000");
  Serial.println(timeStamp);

  char payload[64]; 
  timeStamp.toCharArray(payload, 64);
  byte hmacResult[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
 
  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);            
 
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  printOled("HTTPS Setup - Binance$Generating SHA256 HMAC");
  String sig = "&signature=";
  for(int i= 0; i< sizeof(hmacResult); i++){
      char str[3];
      sprintf(str, "%02x", (int)hmacResult[i]);
      sig.concat(String(str));
  }
  Serial.print("Sig : ");
  Serial.println(sig);

  url = "/api/v3/account?";
  Serial.print("requesting URL: ");
  Serial.println(url);

  printOled("HTTPS Setup - Binance$Sending GET Request");
  client.print(String("GET ") + url + timeStamp + sig + " HTTP/1.1\r\n" +
               "Host: " + host_bin + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: keep-alive\r\n" +
               "X-MBX-APIKEY: " + auth_bin + "\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("\nheaders received");
      printOled("HTTPS Setup - Binance$Headers Received");
      break;
    }
    //delay(5);
  }

  boolean balStart = false;
  String rA = "";
  double rV = 0;
  String s = "";
  int dispCount = 0;
  printOled("HTTPS Setup - Binance$Parsing API Response");
  while(client.available()){
    if(!balStart){
      client.readStringUntil('[');
      balStart = true;
    }
    s = client.readStringUntil('}');
    delay(10);
    client.read(); 
    s.concat("}");

    Serial.print("Runtime: ");
    Serial.print(millis());   
    Serial.print(" - Free Heap: ");
    Serial.print(ESP.getFreeHeap(),DEC);   
    Serial.print(" - ");
    Serial.println(s);

    int k = s.indexOf(":");
    int j = s.indexOf(",");
    rA = s.substring(k+2, j-1);

    k = s.lastIndexOf(",");
    rV = s.substring(j+9, k-1).toDouble();
    
    dispCount++;
    if(dispCount == 5){
      printOled("@HTTPS Setup - Binance$Reading Asset: "+rA);
      dispCount = 0;
    }

    if(rV > 0){  
      asset[valid] = rA;
      amount[valid] = rV;
      valid++;;
    }

  }

  for(int i=luno_quant; i<valid; i++){
    printOled("HTTPS Setup - Binance$" + asset[i] +" : " + amount[i]);
    if(!asset[i].equals("USDT")){
      scroll_assets++;
      ticker[i] = asset[i];
      ticker[i].concat("USDT");
    }else{
      ticker[i] = asset[i];
    }
  }
  
  Serial.print(ESP.getFreeHeap(),DEC);  

  Serial.println("\n=========OUTPUT==========");
  Serial.print("\nValid:" );
  Serial.println(valid); 
  valid_assets = valid;
  Serial.println("");
  for(int i=0; i<valid; i++){
    Serial.print("\nAsset:" );
    Serial.print(asset[i]);
    Serial.print("\nTicker:" );
    Serial.print(ticker[i]);
    Serial.print("\nAmount:" );
    Serial.print(amount[i]);
    Serial.print("\n");
  }
  
}

void printOled(String s) {      //prints to display; $ = first line break, & = 2nd break, @ prefix = no delay
  u8g2.clearBuffer();           // clear the internal memory
  char text[200];
  int dl;
  int loc = s.indexOf("@");
  if(loc != -1){
    s = s.substring(1, s.length());
    dl = 0;
  }else{
    dl = 300;
  }
  
  int loc1 = s.indexOf("$");
  int loc2 = s.indexOf("&");
  String temp;

  if (loc1 == -1 && loc2 == -1) {
    s.toCharArray(text, 200);
    u8g2.drawStr(pos, 24, text);

  } else if (loc1 != -1 && loc2 == -1) {
    temp = s.substring(0, loc1);
    temp.toCharArray(text, 200);
    u8g2.drawStr(pos, 24, text);

    temp = s.substring(loc1 + 1, s.length());
    temp.toCharArray(text, 200);
    u8g2.drawStr(pos, 36, text);

  } else {
    temp = s.substring(0, loc1);
    temp.toCharArray(text, 200);
    u8g2.drawStr(pos, 24, text);

    temp = s.substring(loc1 + 1, loc2);
    temp.toCharArray(text, 200);
    u8g2.drawStr(pos, 36, text);

    temp = s.substring(loc2 + 1, s.length());
    temp.toCharArray(text, 200);
    u8g2.drawStr(pos, 48, text);
  }

  u8g2.sendBuffer();
  delay(dl);
}

void setup(void) {
  u8g2.begin();
  u8g2.setFont(u8g2_font_pressstart2p_8r);
  //u8g2.setFont(u8g2_font_profont12_mr);
  printOled("Wi-Fi Setup$Intitialising...");
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  Serial.print("\nConnecting");
  String text = String("Wi-Fi Setup$Conecting to ");
  text.concat(ssid);
  printOled(text);
  int count = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    text.concat(".");
    printOled(text);
    count++;
    if (count % 5 == 0) {
      text = String("Wi-Fi Setup$Conecting to ");
      text.concat(ssid);
      printOled(text);
    }
  }
  printOled("Wi-Fi Setup$WiFi Connected!");
  delay(100);
  
  time_ny.begin();
  time_bin.begin();

  u8g2.drawBox(0,0,1,1);
  delay(10000);
  u8g2.sendBuffer();

  readBalances();
  readPrice(); 
  last = millis();
  printOled("Setup Complete!");
  startup = false;
}

void updateTimeString(){
  time_ny.update();

  if (time_ny.getHours() == 0) {
    sprintf(timeNY, "New York -> %s, %02d:%02d:%02d AM  ", daysOfTheWeek[time_ny.getDay()],
            time_ny.getHours() + 12,
            time_ny.getMinutes(), time_ny.getSeconds());
  } else if (time_ny.getHours() < 12) {
    sprintf(timeNY, "New York -> %s, %02d:%02d:%02d AM  ", daysOfTheWeek[time_ny.getDay()],
            time_ny.getHours(),
            time_ny.getMinutes(), time_ny.getSeconds());
  } else if (time_ny.getHours() == 12) {
    sprintf(timeNY, "New York -> %s, %02d:%02d:%02d PM  ", daysOfTheWeek[time_ny.getDay()],
            time_ny.getHours(),
            time_ny.getMinutes(), time_ny.getSeconds());
  } else {
    sprintf(timeNY, "New York -> %s, %02d:%02d:%02d PM  ", daysOfTheWeek[time_ny.getDay()],
            time_ny.getHours() - 12,
            time_ny.getMinutes(), time_ny.getSeconds());
  }
}

int down = 0;
bool pause = false;
int pause_c = 0;
int t_height = 12;

void fillRow(void){       //method to scroll price window
  char line[35];
  for(int i=0; i<scroll_assets; i++){
    textLines[i].toCharArray(line, 35);
    if((i+2)*t_height-down > 16 && (i+2)*t_height-down < 32){
      u8g2.drawStr(pos, (i+2)*t_height-down, line);
    }
    if(i == scroll_assets-1 && down > (valid_assets-3)*t_height){
      textLines[0].toCharArray(line, 35);
      u8g2.drawStr(pos, (i+2)*t_height-down+t_height, line);
    }
  }

  if(!pause && down%t_height == 0 && down != 0){
    pause = true;
  }

  if(pause){
    pause_c++;
    if(pause_c == 8){
      pause = false;
      pause_c = 0;
      down=down+2;
    }
  }else{
    down=down+2;
  }

  
  if(down >= (scroll_assets)*t_height+2){
    down = 0;
  }
  u8g2.setDrawColor(0);
  u8g2.drawBox(0,0,256,16);
  u8g2.drawBox(0,24,256,40);
  u8g2.setDrawColor(1);
}

int update_dur = 180000;  //in ms

void loop(void) {

  if (millis() - last > update_dur) {
    Serial.println("\n>>>>>>>>>>UPDATING VALUES<<<<<<<<<<\n");
    u8g2.drawStr(pos, 24, ">>> Updating Prices           ");
    u8g2.sendBuffer();
    readPrice();              //updates prices, reads from server
    Serial.println(">>>>>>>>>>UPDATING COMPELETE<<<<<<<<<<\n");
    last = millis();
  }


  if (millis() - prev > 100) {
    updateTimeString();         //update the time string
        
    Serial.print("Runtime: ");
    Serial.print(millis());
    Serial.print("    Free Heap: ");
    Serial.print(ESP.getFreeHeap(),DEC); 
    Serial.print("    Update In: ");
    Serial.println(update_dur - (millis() - last));  
    
    u8g2.clearBuffer();					// clear the internal memory
    fillRow();                  //updates the scrolling price windown
    u8g2.drawStr(pos, 12, timeNY);
    u8g2.drawStr(pos, 36, lunoVal);
    u8g2.drawStr(pos, 48, binVal);
    u8g2.drawStr(pos, 60, total);
    u8g2.sendBuffer();					// transfer internal memory to the display
    
    prev = millis();
  }

  if (millis() - t_pos > 300000) {    //not in use - u_pos(); shifts the display by 1 pixel to reduce oled burnin
    //u_pos();
    t_pos = millis();
  }

}
