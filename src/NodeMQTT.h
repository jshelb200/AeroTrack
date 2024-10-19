// Module : Interface entre Nodered et ESP32
// Créé le 10/10/2024

#ifndef NODEMQTT_H
#define NODEMQTT_H

#include <PubSubClient.h>
#include <WiFi.h>

class NodeMQTT {
  private:
    WiFiClient espClient;
    PubSubClient client;
    const char* ssid;
    const char* password;
    const char* mqttServer;
    const char* mqttUser;
    const char* mqttPassword;
    int mqttPort;

    // Méthode privée pour se reconnecter au MQTT en cas de déconnexion
    void reconnectMQTT() {
      while (!client.connected()) {
        Serial.print("Connexion au broker MQTT...");
        if (client.connect("ESPClient", mqttUser, mqttPassword)) {
          Serial.println("connecté");
        } else {
          Serial.print("Échec, rc=");
          Serial.println(client.state());
          delay(5000);
        }
      }
    }

  public:
    NodeMQTT(const char* wifiSSID, const char* wifiPassword, const char* server, int port, const char* user, const char* pass) 
      : ssid(wifiSSID), password(wifiPassword), mqttServer(server), mqttPort(port), mqttUser(user), mqttPassword(pass), client(espClient) {}

    void setupWifi() {
      Serial.print("Connexion à ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("\nWiFi connecté, IP : ");
      Serial.println(WiFi.localIP());
    }

    void setupMQTT() {
      client.setServer(mqttServer, mqttPort);
    }

    void publishMessage(const char* topic, const char* message) {
      if (!client.connected()) {
        reconnectMQTT();
      }
      client.publish(topic, message);
    }

    void handleMQTT() {
      client.loop();
    }
};

#endif
