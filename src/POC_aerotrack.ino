/*
Projet : Aérotrack
*/


#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <HTTPClient.h>
#include "base64.h"
#include <ESP32Servo.h>  // Remplacement de la bibliothèque Servo par ESP32Servo
#include "NodeMQTT.h"

// NodeMQTT.h
/*
String message = "Hello";
float temperature = 23.45;

// Convertir le String en const char*
const char* msgStr = message.c_str();

// Convertir le float en char*
char tempStr[8];
dtostrf(temperature, 6, 2, tempStr);

// Publier les messages
client.publish("myTopic", msgStr);
client.publish("temperatureTopic", tempStr);

*/

// Paramètres WiFi et MQTT
const char* ssid = "ssid";
const char* password = "00000000";
const char* mqtt_server = "172.20.10.2";
const char* mqtt_user = "mqtt_user";
const char* mqtt_password = "user";
const int mqtt_port = 1883;

// WeatherStack API key
const String apiKey = "your_api_key";

// Twilio credentials
const String twilioAccountSID = " your_account_sid";
const String twilioAuthToken = "your_auth_token";
const String twilioFromNumber = " your_twilio_number"; 
const String recipientNumber = "your.. ";  // The recipient's phone number

// Crée une instance de NodeMQTT
NodeMQTT mqttClient(ssid, password, mqtt_server, mqtt_port, mqtt_user, mqtt_password);

//Topic
const char* temperature_topic = "aerotrack/sensor/temperature";
const char* humidity_topic = "aerotrack/sensor/humidity";
const char* smoke_topic = "aerotrack/sensor/smoke";

const char* town_humidity_topic = "aerotrack/sensor/humidityville";
const char* town_temperature_topic = "aerotrack/sensor/temperatureville";

const char* window_control_topic = "aerotrack/window/control";

String city = "";
Adafruit_BME680 bme;

const int ldrPin = 32; // Pin analogique pour le LDR
const int R_fixe = 10; // Résistance fixe en kilo ohms (10 kΩ)
const float V_in = 3.3; // Tension d'alimentation

const int servoPin = 35; // Choisis un pin approprié pour connecter le servo
Servo myServo;  // Utilisation de la bibliothèque ESP32Servo

// Seuils
const float humidityThreshold = 10.0; // Différence d'humidité pour envoyer une notification
const float temperatureThreshold = 16.0; // Température en dessous de laquelle il faut allumer le chauffage

void setup_wifi() {
  delay(10);
  Serial.print("Connexion à ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connecté");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());
}


void setup() {
  Serial.begin(115200);
  
  //setup_wifi();
  mqttClient.setupWifi();
  mqttClient.setupMQTT();

  // Initialisation du capteur BME680
  if (!bme.begin()) {
    Serial.println("Erreur de connexion au capteur BME680 !");
    while (1);
  }

  // Configuration du BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320°C pour 150 ms

  // Initialisation du servo
  myServo.attach(servoPin);  // Utilisation de la fonction attach() de ESP32Servo
  myServo.write(0); // Position initiale du servo à 0°

  getCityFromIP(); // Obtenir la ville via GeoPlugin
}

void loop() {

    // Gère la connexion MQTT
  mqttClient.handleMQTT();

  if (city.length() > 0) {
    getWeatherAndCompare();
  } else {
    Serial.println("Ville non obtenue, attente...");
    delay(1000); // Attendre un peu avant de réessayer
  }

  // Lecture des capteurs et publication via MQTT
  if (!bme.performReading()) {
    Serial.println("Erreur de lecture du capteur BME680");
    return;
  }
  delay(3000); // Attendre 5 secondes avant la prochaine lecture
}


// Récupérer la ville à partir de GeoPlugin
void getCityFromIP() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://www.geoplugin.net/json.gp");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      city = extractValue(payload, "geoplugin_city");
      if (city.length() > 0) {
        Serial.print("Ville obtenue : ");
        Serial.println(city);
      }
    } else {
      Serial.print("Erreur HTTP : ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi déconnecté");
  }
}

void checkLDR() {
  int adcValue = analogRead(ldrPin); 

  Serial.print("ADC VALUE :");
  Serial.println(adcValue);

  if (adcValue > 4000) {
   // sendSMS("FUMEE detectée, vérifiez la situation !");
    
    // Faire bouger le servo de 0° à 90°
    myServo.write(90);
    while(adcValue > 4000){
      delay(1000); // Attendre 1 seconde
    }
    myServo.write(0);
  } else {
    // Remettre le servo à 0° si aucune fumée n'est détectée
    myServo.write(0);
  }
}

// Extraire une valeur JSON d'une chaîne de caractères
String extractValue(String payload, String key) {
  int startIndex = payload.indexOf(key + "\":") + key.length() + 2;
  int endIndex = payload.indexOf(",", startIndex);
  if (endIndex == -1) {
    endIndex = payload.indexOf("}", startIndex);
  }
  return payload.substring(startIndex, endIndex);
}

// Comparer les données météo et envoyer une notification si nécessaire
void getWeatherAndCompare() {
  if (WiFi.status() == WL_CONNECTED && city.length() > 0) {
    HTTPClient http;
    String url = "http://api.weatherstack.com/current?access_key=" + apiKey + "&query=" + city;
    Serial.print("Demande des données météo pour : ");
    Serial.println(city);
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      String temperatureWS = extractValue(payload, "temperature");
      String humidityWS = extractValue(payload, "humidity");

      float weatherTemperature = temperatureWS.toFloat();
      float weatherHumidity = humidityWS.toFloat();

      Serial.print("Température WeatherStack : ");
      Serial.println(weatherTemperature);
      Serial.print("Humidité WeatherStack : ");
      Serial.println(weatherHumidity);

         // Convertir les données du capteur en char* pour MQTT
      char tempWStr[8];
      dtostrf(weatherTemperature, 6, 2, tempWStr);  
  
      char humWStr[8];
      dtostrf(weatherHumidity, 6, 2, humWStr);      

      // Publier les données de température et d'humidité sur les topics MQTT
        mqttClient.publishMessage(town_humidity_topic, humWStr);
        mqttClient.publishMessage(town_temperature_topic, tempWStr);

      float bmeTemperature = bme.temperature;
      float bmeHumidity = bme.humidity;

      Serial.print("Température BME680 : ");
      Serial.println(bmeTemperature);
      Serial.print("Humidité BME680 : ");
      Serial.println(bmeHumidity);

    // Convertir les données du capteur en char* pour MQTT
      char tempStr[8];
      dtostrf(bme.temperature, 6, 2, tempStr);  
  
      char humStr[8];
      dtostrf(bme.humidity, 6, 2, humStr);      

      // Publier les données de température et d'humidité sur les topics MQTT
        mqttClient.publishMessage(humidity_topic, humStr);
        mqttClient.publishMessage(temperature_topic, tempStr);

      // Comparer les données et envoyer des SMS si nécessaire
      if ((bmeHumidity - weatherHumidity) > humidityThreshold) {
       // sendSMS("Aérez la pièce");
      }

      if (bmeTemperature < temperatureThreshold) {
        //sendSMS("Allumez le chauffage");
      }

    } else {
      Serial.print("Erreur HTTP : ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi déconnecté ou ville introuvable");
  }
}

// Envoyer un SMS via Twilio
void sendSMS(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String twilioUrl = "https://api.twilio.com/2010-04-01/Accounts/" + twilioAccountSID + "/Messages.json";
    String postData = "To=" + recipientNumber + "&From=" + twilioFromNumber + "&Body=" + message;

    String auth = twilioAccountSID + ":" + twilioAuthToken;
    String authBase64 = base64::encode(auth);

    http.begin(twilioUrl);
    http.addHeader("Authorization", "Basic " + authBase64);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("SMS envoyé avec succès :");
      Serial.println(response);
    } else {
      Serial.print("Erreur envoi SMS : ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi déconnecté");
  }
}