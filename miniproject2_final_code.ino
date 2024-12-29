#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#define ssid "OPPOA54"
#define password "12345678"
const char* testServer = "www.google.com"; // Known web server to test internet access
#define API_KEY "AIzaSyAD_3hUEq6G2OCbH6cyb8FI9TIjAptmnhc"
#define DATABASE_URL "https://mini-c75f8-default-rtdb.firebaseio.com/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
#define soil 34
const int dry=2835;
const int wet=1115;
const int relay = 26;
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);  
DallasTemperature sensors(&oneWire);
float fahrenheit; // Declare fahrenheit variable here
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup(){
  pinMode(soil, INPUT);
  lcd.begin();
  lcd.setBacklight(HIGH);
  pinMode(relay, OUTPUT);
  connectToWiFi();
  Serial.begin(115200);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
   sensors.begin();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Print "connected" to serial monitor after successfully connecting to WiFi
  Serial.println("Connected");

  // Check internet access by attempting to connect to a known web server
  if (WiFi.isConnected()) {
    WiFiClient client;
    if (client.connect(testServer, 80)) {
      Serial.println("Internet access available");
      client.stop();
    } else {
      Serial.println("No internet access");
    }
  } else {
    Serial.println("WiFi connection lost");
  }
}

void loop(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    int humidity,WeatherTemperature;
    String Weather;
    // Generate random integer value between 0 and 100
    int randomInt = random(0, 101);
    int out = analogRead(soil);
   // int percentage = ( 100 - ( (out/4095.00) * 100 ) );
   int percentage=map(out,wet,dry,100,0);
    percentage = constrain(percentage, 0, 100); 
    sensors.requestTemperatures(); 
    float celsius = sensors.getTempCByIndex(0);
    
  // Check if temperature is valid (DS18B20 can sometimes return -127 as an error code)
  if (celsius != -127.00) {
    // Convert Celsius to Fahrenheit
     fahrenheit = celsius * 1.8 + 32.0;
    
    // Print temperature in Fahrenheit
    Serial.print("Temperature: ");
    Serial.print(fahrenheit);
    Serial.println(" F");
  } else {
    // If temperature reading is invalid, print an error message
    Serial.println("Error: Unable to read temperature!");
  }
  
  // Wait for a moment before taking the next reading
  delay(1000);
    Serial.print("Analog Reading: ");
    Serial.println(out);
    Serial.print("Mapped Percentage: ");
    Serial.println(percentage);
  
    // Write the percentage value to the database path SOIL/Moisture
  if (Firebase.RTDB.setInt(&fbdo, "SOIL/Moisture", percentage)){
    Serial.println("Moisture Percentage Sent to Firebase: " + String(percentage) + "%");
  } else {
    Serial.println("Failed to send moisture percentage to Firebase");
  }
  
  if (Firebase.RTDB.setFloat(&fbdo, "SOIL/Temperature", fahrenheit)) {
    Serial.print("Temperature (Fahrenheit) Sent to Firebase: ");
    Serial.println(fahrenheit);
  } else {
    Serial.println("Failed to send temperature (Fahrenheit) to Firebase");
  }

   if (Firebase.RTDB.getInt(&fbdo, "/weather/humidity")) {
      Serial.println("Data fetched from Firebase:");
      if(fbdo.dataType() == "int"){
         humidity = fbdo.intData();
        Serial.print("Humidity:");
        Serial.println(humidity);
      }
    }
    if (Firebase.RTDB.getInt(&fbdo, "/weather/temperature")) {
     // Serial.println("Data fetched from Firebase:");
      if(fbdo.dataType() == "int"){
         WeatherTemperature = fbdo.intData();
        Serial.print("WeatherTemperature:");
        Serial.println(WeatherTemperature);
      }
    }
    if (Firebase.RTDB.getString(&fbdo, "/weather/weather")) {
     // Serial.println("Data fetched from Firebase:");
      if(fbdo.dataType() == "string"){
         Weather = fbdo.stringData();
        Serial.print("Weather:");
        Serial.println(Weather);
      }
    }
     else {
      Serial.println("Failed to fetch data from Firebase");
    }
   if(percentage<30 ){
    lcd.setCursor(13, 1);
    lcd.print("On");
    digitalWrite(relay, HIGH);

  }else if(percentage<50 && Weather=="Rainy" ){
    lcd.setCursor(13, 1);
    lcd.print("off");
    digitalWrite(relay, LOW);
  }
  else{
    lcd.setCursor(13, 1);
    lcd.print("off");
    digitalWrite(relay, LOW);
  }
  // Clear the LCD display
    lcd.clear();

    // Set cursor to (0, 0) and print Soil Moisture
    lcd.setCursor(0, 0);
    lcd.print("SM:"); // Display text
    lcd.print(percentage); // Display moisture percentage
    lcd.print("% ");

    // Print Weather Temperature
    lcd.print("WT:");
    lcd.print(WeatherTemperature);
    lcd.print("'c ");

    // Set cursor to (0, 1) and print Humidity
    lcd.setCursor(0, 1);
    lcd.print("H:"); // Display text
    lcd.print(humidity); // Display humidity percentage
    lcd.print("% ");

    // Print Weather
    lcd.print(Weather); 
  
  delay(500); // Adjust delay as needed
}
}
