#ifndef WIFIHELPER_H_
#define WIFIHELPER_H_

#include <WiFi.h>
#include <PubSubClient.h>

class WiFiHelper
{
public:
    WiFiHelper(String ssid, String password);
    IPAddress connect(void);
    bool connected();
    bool mqtt_connect(String mqttServer, int mqttPort, String mqttUser, String mqttPassword);
    bool mqtt_connected();
    bool mqtt_publish(String topic, String payload);
    bool mqtt_subscribe(String topic);
private:
    String _ssid;
    String _password;
    WiFiClient *client;
    PubSubClient *mqttClient;
    static void MQTTCallback(char *topic, byte *payload, unsigned int length);
};


#endif /* WIFIHELPER_H_ */
