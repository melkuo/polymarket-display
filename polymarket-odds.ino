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
// Array of market condition IDs to display
String marketConditionIds[] = {
  "0xae6dab898b0531faf72bf9f09517538249131850cfe48b309f7f91124411c380",
  "0x51f7452fddea310ba15955cfa8fa21419335b16f8310074e43f48c4856d7e0b8",
  "0xfd6714df13e06c8e551cd2e907b8d7ee3982ade9a55476536b557e88ce3d8c78",
  "0x510e4e26f501eb687ced8a5f8357002119d6504d081a6edae1ab9ddf0dea5ec5",
  "0x2899aa05134d3a493f870653f2fe5e286d0a694b46847b330029c74975904cac"
};
int numMarkets = 5;  // Update this if you change the array size
int delayBetweenMarkets = 15000;  // 15 seconds between markets
unsigned long apiRefreshInterval = 15 * 60 * 1000; // 15 minutes in milliseconds
unsigned long lastApiUpdateTime = 0;

// Structure to store market data
struct MarketData {
  String question;
  String outcome1;
  String outcome2;
  int percent1;
  int percent2;
  bool dataValid;
};

// Array to store cached market data
MarketData marketCache[5]; // Update this if you change the array size

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
  
  // Initialize market cache
  for (int i = 0; i < numMarkets; i++) {
    marketCache[i].dataValid = false;
  }
  
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
  
  // Initial market data fetch
  fetchAllMarketData();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    display1.print("Wifi disconnected");
    delay(1000);
    return;
  }
  
  // Check if it's time to refresh API data (every 15 minutes)
  unsigned long currentTime = millis();
  if (currentTime - lastApiUpdateTime >= apiRefreshInterval || lastApiUpdateTime == 0) {
    display1.print("Updating market data...");
    fetchAllMarketData();
    lastApiUpdateTime = currentTime;
  }
  
  // Cycle through markets (display a different one every 15 seconds)
  for (int i = 0; i < numMarkets; i++) {
    if (marketCache[i].dataValid) {
      // Display market question
      display1.print(marketCache[i].question.c_str());
      
      // Display outcomes and prices
      char displayText2[50];
      sprintf(displayText2, "%s: %d%% %s: %d%%", 
              marketCache[i].outcome1.c_str(), marketCache[i].percent1, 
              marketCache[i].outcome2.c_str(), marketCache[i].percent2);
      display2.print(displayText2);
    } else {
      display1.print("No data for market");
      display2.print("Waiting for update");
    }
    
    // Wait before showing the next market
    delay(delayBetweenMarkets);
  }
}

void fetchAllMarketData() {
  for (int i = 0; i < numMarkets; i++) {
    fetchMarketData(i);
  }
}

void fetchMarketData(int marketIndex) {
  String conditionId = marketConditionIds[marketIndex];
  String requestUrl = "https://clob.polymarket.com/markets/" + conditionId;
  
  // Make the API request to Polymarket
  String response = httpsGetRequest(requestUrl);
  
  // Parse the JSON response
  const size_t capacity = 8192;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    marketCache[marketIndex].dataValid = false;
    return;
  }
  
  // Extract and cache the market data
  marketCache[marketIndex].question = doc["question"].as<String>();
  
  // Extract outcomes and prices
  JsonArray tokens = doc["tokens"];
  if (tokens.size() >= 2) {
    marketCache[marketIndex].outcome1 = tokens[0]["outcome"].as<String>();
    float price1 = tokens[0]["price"].as<float>();
    marketCache[marketIndex].outcome2 = tokens[1]["outcome"].as<String>();
    float price2 = tokens[1]["price"].as<float>();
    
    // Format prices as percentages
    marketCache[marketIndex].percent1 = (int)(price1 * 100);
    marketCache[marketIndex].percent2 = (int)(price2 * 100);
    
    marketCache[marketIndex].dataValid = true;
  } else {
    marketCache[marketIndex].dataValid = false;
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