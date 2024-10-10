#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <HTTPClient.h>
#include "base64.h"

// Wifi c
const char* ssid = "iPhone de Rafik";
const char* password = "1234567890";

// WeatherStack API key
const String apiKey = "7d77b83eebfa9c00162898cf7050454c";

const String twilioAccountSID = "ACd97f14ad4bbee2144d558134971bb352";
const String twilioAuthToken = "20f671b922d85353ed71542a5e59871b";
const String twilioFromNumber = "+13344234679"; 
const String recipientNumber = "+33667204668";  // The recipient's phone number

String city = "";
Adafruit_BME680 bme;

// Thresholds
float humidityThreshold = 10.0; // Difference in humidity for sending a notification
float temperatureThreshold = 16.0; // Below this, ask to turn on the heater

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");

  // Initialize the BME680 sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  // Set up BME680 sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms

  getCityFromIP(); // Get city name from GeoPlugin
}

void getCityFromIP() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://www.geoplugin.net/json.gp");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      city = extractValue(payload, "geoplugin_city");
      if (city.length() > 0) {
        Serial.print("City from GeoPlugin: ");
        Serial.println(city);
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void getWeatherAndCompare() {
  if (WiFi.status() == WL_CONNECTED && city.length() > 0) {
    HTTPClient http;
    String url = "http://api.weatherstack.com/current?access_key=" + apiKey + "&query=" + city;
    Serial.print("Requesting weather data for: ");
    Serial.println(city);
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      String temperatureWS = extractValue(payload, "temperature");
      String humidityWS = extractValue(payload, "humidity");

      float weatherTemperature = temperatureWS.toFloat();
      float weatherHumidity = humidityWS.toFloat();

      Serial.print("WeatherStack Temperature: ");
      Serial.println(weatherTemperature);
      Serial.print("WeatherStack Humidity: ");
      Serial.println(weatherHumidity);

      // Get BME680 sensor data
      if (!bme.performReading()) {
        Serial.println("Failed to perform reading on BME680 sensor");
        return;
      }

      float bmeTemperature = bme.temperature;
      float bmeHumidity = bme.humidity;

      Serial.print("BME680 Temperature: ");
      Serial.println(bmeTemperature);
      Serial.print("BME680 Humidity: ");
      Serial.println(bmeHumidity);

      // Compare BME680 and WeatherStack values
      if ((bmeHumidity - weatherHumidity) > humidityThreshold) {
        sendSMS("Aerez la chambre");
      }

      if (bmeTemperature < temperatureThreshold) {
        sendSMS("Allumez le chauffage");
      }

    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected or City not found");
  }
}

void sendSMS(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Twilio API URL
    String twilioUrl = "https://api.twilio.com/2010-04-01/Accounts/" + twilioAccountSID + "/Messages.json";

    // SMS content
    String postData = "To=" + recipientNumber + "&From=" + twilioFromNumber + "&Body=" + message;

    // Base64 encode for Twilio Authentication
    String auth = twilioAccountSID + ":" + twilioAuthToken;
    String authBase64 = base64::encode(auth);

    // Send POST request
    http.begin(twilioUrl);
    http.addHeader("Authorization", "Basic " + authBase64);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("SMS sent successfully:");
      Serial.println(response);
    } else {
      Serial.print("Error sending SMS: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

String extractValue(String payload, String key) {
  int startIndex = payload.indexOf(key + "\":") + key.length() + 2;
  int endIndex = payload.indexOf(",", startIndex);
  if (endIndex == -1) {
    endIndex = payload.indexOf("}", startIndex);
  }
  return payload.substring(startIndex, endIndex);
}

void loop() {
  if (city.length() > 0) {
    getWeatherAndCompare();
    delay(1800000); // Wait for 30 minutes
  } else {
    Serial.println("City not yet obtained, waiting...");
    delay(1000); // Wait a short time before checking again
  }
}