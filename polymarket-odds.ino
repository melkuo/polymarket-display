#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiClientSecureBearSSL.h>

// Setup variables
const char* ssid = "";
const char* password = "";
String marketConditionIds[] = {
  "0xae6dab898b0531faf72bf9f09517538249131850cfe48b309f7f91124411c380",
  "0x51f7452fddea310ba15955cfa8fa21419335b16f8310074e43f48c4856d7e0b8"
};
int numMarkets = 2;
int delayBetweenMarkets = 20000;
String apiEndpoint = "https://clob.polymarket.com/markets/";

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12
#define DATA_PIN_1 D7 // GPIO13
#define CLK_PIN_1 D5  // GPIO14
#define CS_PIN_1 D8   // GPIO15
#define DATA_PIN_2 D2 // GPIO4
#define CLK_PIN_2 D1  // GPIO5
#define CS_PIN_2 D3   // GPIO0

MD_Parola display2 = MD_Parola(HARDWARE_TYPE, DATA_PIN_1, CLK_PIN_1, CS_PIN_1, MAX_DEVICES);
MD_Parola display1 = MD_Parola(HARDWARE_TYPE, DATA_PIN_2, CLK_PIN_2, CS_PIN_2, MAX_DEVICES);

void setup() {
  Serial.begin(9600);
  display1.begin();
  display1.setIntensity(0);
  display1.displayClear();
  display2.begin();
  display2.setIntensity(0);
  display2.displayClear();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    display1.print("Connecting to wifi");
    delay(500);
    display1.print("Connecting to wifi.");
    delay(500);
    display1.print("Connecting to wifi..");
    delay(500);
    display1.print("Connecting to wifi...");
    delay(500);
  }
  display1.displayClear();
  display2.displayClear();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    display1.print("Wifi disconnected");
    delay(1000);
    return;
  }
  
  for (int i = 0; i < numMarkets; i++) {
    String conditionId = marketConditionIds[i];
    String requestUrl = "https://clob.polymarket.com/markets/" + conditionId;
    
    // Make the API request to Polymarket
    String response = httpsGetRequest(requestUrl);
    
    // Parse the JSON response
    // Use a larger capacity for the JSON document as the response is quite large
    const size_t capacity = 8192;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      display1.print("JSON parse error");
      display2.print(error.c_str());
      delay(5000);
      continue;
    }
    
    // Extract question (for display1)
    String question = doc["question"].as<String>();
    display1.print(question.c_str());
    
    // Extract outcomes and prices (for display2)
    // Since there are typically only two outcomes (Yes/No), we can display them together
    JsonArray tokens = doc["tokens"];
    if (tokens.size() >= 2) {
      String outcome1 = tokens[0]["outcome"].as<String>();
      float price1 = tokens[0]["price"].as<float>();
      String outcome2 = tokens[1]["outcome"].as<String>();
      float price2 = tokens[1]["price"].as<float>();
      
      // Format prices as percentages (multiply by 100 and round)
      int percent1 = (int)(price1 * 100);
      int percent2 = (int)(price2 * 100);
      
      // Prepare the text for display2
      char displayText2[50];
      sprintf(displayText2, "%s: %d%% %s: %d%%", outcome1.c_str(), percent1, outcome2.c_str(), percent2);
      display2.print(displayText2);
    } else {
      display2.print("No outcome data");
    }
    
    // Delay before showing the next market
    delay(delayBetweenMarkets);
  }
}

String httpsGetRequest(String serverName) {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  https.begin(*client, serverName);
  int httpCode = https.GET();
  String response = "{}";
  if (httpCode > 0) {
    response = https.getString();
    if (httpCode != 200) {
      display1.displayClear();
      display1.print(httpCode);
    }
  } else {
    display1.print("Error: no response code");
  }
  https.end();
  return response;
}
