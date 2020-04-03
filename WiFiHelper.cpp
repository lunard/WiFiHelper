
#include "WiFiHelper.h"

WiFiHelper::WiFiHelper(String ssid, String password)
{
    _ssid = ssid;
    _password = password;
    client = new WiFiClient();
    ntpUdpClient = new WiFiUDP();
    mqttClient = new PubSubClient(*client);
    timeClient = new NTPClient(*ntpUdpClient);
}

IPAddress WiFiHelper::connect()
{
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(_ssid);

    WiFi.begin(_ssid.c_str(), _password.c_str());
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        if (retryCount > 20)
        {
            Serial.println("");
            Serial.println("Retry init WiFi");
            WiFi.begin(_ssid.c_str(), _password.c_str());
            retryCount = 0;
        }
        delay(500);
        Serial.print(".");
        retryCount++;
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    timeClient->begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient->setTimeOffset(2 * 3600);

    return WiFi.localIP();
};

bool WiFiHelper::connected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiHelper::mqtt_connect(String mqttServer, int mqttPort, String mqttUser, String mqttPassword)
{
    mqttClient->setServer(mqttServer.c_str(), mqttPort);
    mqttClient->setCallback(MQTTCallback);

    while (!mqttClient->connected())
    {
        Serial.print("Attempting MQTT connection...");
        // // Create a random client ID
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient->connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str()))
        {
            Serial.println("connected");
            //Once connected, publish an announcement...
            //client.publish("/icircuit/presence/ESP32/", "hello world");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient->state());
            delay(5000);
        }
    }
}

bool WiFiHelper::mqtt_connected()
{
    return mqttClient->connected();
}

bool WiFiHelper::mqtt_publish(String topic, String payload)
{
    if (mqttClient->connected())
    {
        mqttClient->publish(topic.c_str(), payload.c_str());
        return true;
    }
    else
    {
        Serial.println("publishOnMQTT: not connected");
        return false;
    }
}

bool WiFiHelper::mqtt_subscribe(String topic)
{
    if (mqttClient->connected())
    {
        mqttClient->subscribe(topic.c_str());
        return true;
    }
    else
    {
        Serial.println("subscribeOnMQTT: not connected");
        return false;
    }
}

void WiFiHelper::MQTTCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.println("-------new message from broker-----");
    Serial.print("channel:");
    Serial.println(topic);
    Serial.print("data:");
    Serial.write(payload, length);
    Serial.println();
}

bool WiFiHelper::downloadFile(fs::FS &fs, String uri, String fileName)
{
    bool result = false;

    if (connected())
    {
        bool isRedirected = false;
        HTTPClient http;
        HTTPClient httpRedirected;
        http.begin(uri);

        const char *headers[] = {"Location"};
        http.collectHeaders(headers, 1);

        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_FOUND && http.hasHeader("Location"))
            {
                String redirectedUri = http.header("Location");
                Serial.println("Redirect to " + redirectedUri);
                httpRedirected.begin(redirectedUri);
                httpCode = httpRedirected.GET();
                http.end();
                isRedirected = true;
            }

            if (httpCode == HTTP_CODE_OK)
            {
                if (fs.exists(fileName))
                    fs.remove(fileName);

                File f = fs.open(fileName, FILE_WRITE);
                if (f)
                {
                    Serial.println("Download " + uri + " to file " + fileName);

                    if (isRedirected)
                        httpRedirected.writeToStream(&f);
                    else
                        http.writeToStream(&f);

                    f.flush();
                    Serial.println("Wrote " + String(f.size()) + " bytes");
                    result = true;
                }
                else
                {
                    Serial.println("Failed to open file " + fileName + " for writing");
                }

                f.close();
            }
        }

        Serial.println("httpCode:" + String(httpCode));

        if (isRedirected)
            httpRedirected.end();
        else
            http.end();
    }
    else
    {
        Serial.println("downloadFile: not connected to a WiFi");
    }
    return result;
}

// https://github.com/taranais/NTPClient/blob/master/NTPClient.cpp
String WiFiHelper::getFormattedTime()
{
    while (!timeClient->update())
    {
        timeClient->forceUpdate();
    }

    return timeClient->getFormattedTime();
}


unsigned long WiFiHelper::getEpochTime()
{
    while (!timeClient->update())
    {
        timeClient->forceUpdate();
    }

    return timeClient->getEpochTime();
}
