/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#define ENGINESOLENOID_ACTIVE (HIGH)
#define ENGINESOLENOID_INACTIVE (LOW)
#define OILPRESSURE_OK (HIGH)
#define ENGINETEMP_OK (HIGH)
#define WATERPRESSURE_OK (HIGH)
#define ALTENATOR_OK (HIGH)
#define USERBUTTON_PRESSED (LOW)

#define ENGINE_WARMUP_TIME_S (60)   // Time before checking sensors to let engine warm up

// WiFi settings
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// MQTT Broker details
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";
const char *MQTT_STATE_EP = "esp32/humidity";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// LED Pins
const int ledPin = 14;
const int stateLedPin = 2;

// Engine IO
const int engineSolenoid_pin = 27;
const int oilPressure_pin = 4;
const int engineTemp_pin = 15;
const int waterPressure_pin = 32;
const int altenator_pin = 25;
const int userButton_pin = 26;

bool led_state = false;
bool engine_active = false;
volatile bool engine_fault;
hw_timer_t * timer = NULL;

/*
 * Critical section
 * Check engine sensors and perform STOP if sensor in error state
*/
void IRAM_ATTR onTime() 
{
   digitalWrite(stateLedPin, led_state ? HIGH : LOW);
   led_state = !led_state;

  // Check input and control engine solenoid if fault
  if (digitalRead(oilPressure_pin) != OILPRESSURE_OK)
    goto engine_fault;
  if (digitalRead(engineTemp_pin) != ENGINETEMP_OK)
    goto engine_fault;
  if (digitalRead(waterPressure_pin) != WATERPRESSURE_OK)
    goto engine_fault;
  if (digitalRead(altenator_pin) != ALTENATOR_OK)
    goto engine_fault;

  return; // OK

engine_fault:
  engine_fault = true;
  digitalWrite(engineSolenoid_pin, ENGINESOLENOID_INACTIVE);
  // Schedule MQTT message
}

void setup() 
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(stateLedPin, OUTPUT);
  pinMode(engineSolenoid_pin, OUTPUT);
  
  pinMode(oilPressure_pin, INPUT);
  pinMode(engineTemp_pin, INPUT);
  pinMode(waterPressure_pin, INPUT);
  pinMode(altenator_pin, INPUT);
  pinMode(userButton_pin, INPUT);

  // Configure the Prescaler at 80, the ESP32 is 80Mhz
  // 80000000 / 80 = 1000000 tics / second
  timer = timerBegin(0, 80, true);                
  timerAttachInterrupt(timer, &onTime, true);    
  
  // Timer to expire every 1 second
  timerAlarmWrite(timer, 1000000, true);           
  timerAlarmEnable(timer);
  
//  delay(10);
//  setup_wifi();
//  client.setServer(mqtt_server, 1883);
//  client.setCallback(callback);

  engine_fault = false;

  Serial.println("Setup done.");
}

void setup_wifi() 
{
  int timeout;
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  timeout = 120;
  while (WiFi.status() != WL_CONNECTED && --timeout > 0) {
    delay(500);
    Serial.print(".");
  }

  if (timeout == 0)
  {
    // WiFi failed. Reboot?
    Serial.println("WiFi Failed");
    delay(1000);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) 
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() 
{
  int timeout;
  timeout = 10;
  // Loop until we're reconnected
  while (!client.connected() && --timeout > 0) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  if (timeout == 0)
  {
    Serial.println("MQTT connect fail");
    delay(3000);
  }
}

void sendEngineAlert()
{
  Serial.println("ENGINE FAULT!");
}

void loop() 
{
  digitalWrite(stateLedPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(500);

  digitalWrite(ledPin, LOW);
  delay(500);

  if (engine_fault)
  {
    sendEngineAlert();

    // Clear flag??
    engine_fault = false;
  }
  
//  if (WiFi.status() != WL_CONNECTED) {
//    WiFi.disconnect();
//    delay(2000);
//    return setup_wifi();
//  }
//  if (!client.connected()) {
//    reconnect();
//  }
//  else
//  {
//    client.loop();
//  
//    long now = millis();
//    if (now - lastMsg > 30000) {
//      lastMsg = now;
//      
//      // Convert the value to a char array
//      String humString = String(now);
//      Serial.print("Humidity: ");
//      Serial.println(humString);
//      client.publish(MQTT_STATE_EP, humString.c_str());
//    }
//  }
}
