#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include <EasyUltrasonic.h>

#include <MPU9250_asukiaaa.h>

// Replace the next variables with your SSID/Password combination
const char *ssid = "b26wifi";
const char *password = "b26password";

// Add your MQTT Broker IP address, example:
const char *mqtt_server = "192.168.2.1";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// variables for mpu250
MPU9250_asukiaaa mySensor;
float aX, aY, aZ, aSqrt;
float gX, gY, gZ;

// variables for ultrasonic
EasyUltrasonic ultrasonic;
int ultrasonicDistance;

// arduino address
#define I2C_SLAVE_ADDR 0x04 // 4 in hexadecimal

int leftMotor = 0;
int rightMotor = 0;
int steeringAngle = 80;

void setup()
{
    Serial.begin(115200);
    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    // status = bme.begin();

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    Wire.begin();
    // mpu250 setup
    mySensor.setWire(&Wire);
    mySensor.beginAccel();
    mySensor.beginGyro();

    // setup ultrasonic sesor
    ultrasonic.attach(14, 12);
}

// connects to network
void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client"))
        {
            // Subscribe
            // client.subscribe("esp32/output");
            client.subscribe("esp32/forward");
            client.subscribe("esp32/back");
            client.subscribe("esp32/left");
            client.subscribe("esp32/right");
            Serial.println("conection made");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
int temp = 0;
void loop()
{
    if (!client.connected())
    {
      Serial.println("client disconceterd");
        reconnect();
    }
    client.loop();

    long now = millis();
    Serial.println("looping");
    // if (now - lastMsg > 1000)
    // {
        lastMsg = now;
        sendSensorData();
    // }



}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: \n");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // Feel free to add more if statements to control more GPIOs with MQTT

    //         If a message is received on the topic esp32 /
    //     output,
    //     you check if the message is either "on" or "off".Changes the output state according to the message
    if (String(topic) == "esp32/forward")
    {
        if (String(messageTemp) == String("1"))
        {
            goForward();
            Serial.println("going forwards");
        }
        else if (messageTemp == "0")
        {
            stop();
            Serial.println("stopping");
        }
        else
        {
            Serial.printf("ERROR invalid value passed to forward");
            Serial.println(messageTemp);
        }
    }
    else if ((String(topic) == "esp32/back"))
    {

        if (messageTemp == "1")
        {
            goBackward();
        }
        else if (messageTemp == "0")
        {
            stop();
        }
        else
        {
            Serial.printf("ERROR invalid value passed to backwards");
            Serial.println(messageTemp);
        }
    }
}
void sendSensorData()
{
    // sending range sendDataToArduino

    sendFloatToTopic("esp32/2/ultrasonic", (float)ultrasonic.getDistanceCM());

    // sending gyro data to sendDataToArduino
    mySensor.accelUpdate();
    mySensor.gyroUpdate();
    sendFloatToTopic("esp32/2/ax", mySensor.accelX());
    sendFloatToTopic("esp32/2/ay", mySensor.accelY());
    sendFloatToTopic("esp32/2/az", mySensor.accelZ());
    sendFloatToTopic("esp32/2/gx", mySensor.gyroX());
    sendFloatToTopic("esp32/2/gy", mySensor.gyroY());
    sendFloatToTopic("esp32/2/gZ", mySensor.gyroZ());

    Serial.printf("\n");
    // sending accelerometer data to sendDataToArduino
    // snnding leftWheel Speed left
    // sennding right wheel speed left
}

void sendFloatToTopic(char *topic, float value)
{
    char result[8]; // Buffer big enough for 7-character float
    dtostrf(value, 6, 2, result);

    client.publish(topic, result);

    Serial.printf(topic);
    Serial.printf("=");
    Serial.printf(result);
    Serial.printf(" ");
}

void sendDataToArduino(int leftMotor, int rightMotor, int steeringAngle)
{
    if (leftMotor > 255)
        leftMotor = 255;
    if (rightMotor > 255)
        rightMotor = 255;
    Wire.beginTransmission(I2C_SLAVE_ADDR); // transmit to device #4
    Wire.write((byte)((leftMotor & 0x0000FF00) >> 8));
    Wire.write((byte)(leftMotor & 0x000000FF));
    Wire.write((byte)((rightMotor & 0x0000FF00) >> 8));
    Wire.write((byte)(rightMotor & 0x000000FF));
    Wire.write((byte)((steeringAngle & 0x0000FF00) >> 8));
    Wire.write((byte)(steeringAngle & 0x000000FF));
    Wire.endTransmission(); // stop transmitting
    delay(100);
}

// functions for remote control from node received

void goForward()
{
    leftMotor = 255;
    rightMotor = 255;
    sendDataToArduino(leftMotor, rightMotor, steeringAngle);
}
void goBackward()
{
    leftMotor = -255;
    rightMotor = -255;
    sendDataToArduino(leftMotor, rightMotor, steeringAngle);
}
void stop()
{
    leftMotor = 0;
    rightMotor = 0;
    sendDataToArduino(leftMotor, rightMotor, steeringAngle);
}
void turnLeft()
{
    steeringAngle -= 10;
    sendDataToArduino(leftMotor, rightMotor, steeringAngle);
}
void turnRight()
{
    steeringAngle += 10;
    sendDataToArduino(leftMotor, rightMotor, steeringAngle);
}

