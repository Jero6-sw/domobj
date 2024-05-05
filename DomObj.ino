
////////////////////////////////////////////////////////////////////////
// Creation from :
// DIY Smart Blinds Controller for ESP8266 (Wemos D1 Mini)
// Supports Home Assistant MQTT auto discovery straight out of the box
// (c) Toni Korhonen 2021
// https://www.creatingsmarthome.com/?p=629

// Jero6 14/03/2024
// Compiled for NodeMCU 1.0 (ESP-12E)

// Jero6 30/03/2024
// Get genericity in the MQTT protocol : cmd discretes, get sensors (I2c moisture), get TIC
// Use WifiManager to get rid of passwords

#define MODEL "undef"


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define MODEL "ESP8266"
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#define MODEL "ESP32"
#include <ESPmDNS.h>
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include "DomObj.h"
#include "CmdServo.h"


#define JSON_BUFFER_LENGTH 2048
#define MQTT_TOPIC_MAX_LENGTH 256

#define DEBUG true // default value for debug

// Callbacks
void mqttCallback(char* topic, byte* payload, unsigned int payloadLength);

// Wifi
WiFiClient wifiClient;

// MQTT
PubSubClient client(mqtt_server, mqtt_port, mqttCallback, wifiClient);

int numberOfActuators = 0;
int numberOfSensors = 0;

CmdServo servos[sizeof(s_actuator)];

// debug
bool debug = DEBUG;
int numberOfServos = 0;
int pos = 0;

//String uniqueId;

unsigned long loopStart;

////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////
void setup() {
  // Setup serial port
  Serial.begin(115200);

  //uniqueId = WiFi.macAddress();
  //uniqueId.replace(":", "-");

  Serial.println("DomObj v" SW_VERSION);

  wifiConnect();

  // Setup OTA
  initOTA();


///////////////

  numberOfActuators = sizeof(actuator) / sizeof(actuator[0]);
  numberOfSensors = sizeof(sensor) / sizeof(sensor[0]);

//  numberOfServos = sizeof(servoPins) / sizeof(servoPins[0]);
//  int numberOfReversedServos = sizeof(reversedPins) / sizeof(servoPins[0]); // We might not have any servos as reversed, thus using servoPin size

  Serial.print("numberOfActuators = ");
  Serial.println(numberOfActuators);

  Serial.print("numberOfSensors = ");
  Serial.println(numberOfSensors);

  numberOfServos=0;
  for (int i = 0; i < numberOfActuators; i++) {
	  
	if (actuator[i].Type == tSERVO) {
       servos[numberOfServos] = CmdServo(i+1, actuator[i].Pin, servo_min_pulse, servo_max_pulse, servo_max_angle, actuator[i].reversed, debug);
       servos[numberOfServos].setDebugPrintCallback(debugPrint);
       servos[numberOfServos].setStatusChangedCallback(statusChanged);
       servos[numberOfServos].setPositionChangedCallback(positionChanged);
	   numberOfServos++;
	   }


	if (actuator[i].Type == tDIGOUT) {
      pinMode(actuator[i].Pin, OUTPUT);    // sets the digital pin as output
      digitalWrite(actuator[i].Pin, LOW);
      }

	if (actuator[i].Type == tDIGTEMP) {
      pinMode(actuator[i].Pin, OUTPUT);    // sets the digital pin as output
      digitalWrite(actuator[i].Pin, LOW);
      }

    }

  Serial.print("numberOfServos = ");
  Serial.println(numberOfServos);

////////////////

  mqttConnect();
  client.setCallback(mqttCallback);

}

void wifiConnect() {
  Serial.println("Connecting wifi..");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  wm.addParameter(&custom_mqtt_server);
  WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, 40);
  wm.addParameter(&custom_mqtt_username);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);
  wm.addParameter(&custom_mqtt_password);
  WiFiManagerParameter custom_mqtt_clientid("Client_id", "mqtt Client id", client_id, 10);
  wm.addParameter(&custom_mqtt_clientid);



    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP","passwordDom"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        delay(5000);
        ESP.restart();
    } 
//    else {
//        //if you get here you have connected to the WiFi    
//        Serial.println("connected...yeey :)");
//    }


//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED) {
//    // wait 500ms
//    Serial.print(".");
//    delay(500);
//  }

//  WiFi.mode(WIFI_STA);

  // Connected to WiFi
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttConnect() {
  if (!!!client.connected()) {
    Serial.println("Try to connect mqtt..");
    int count = 20;
    while (count-- > 0 && !!!client.connect(client_id, mqtt_username, mqtt_password)) {
      delay(500);
    }

  for (int i = 0; i < numberOfActuators; i++) {
    subscribeAndPublishConfig(i);
    }
  }
}

void initOTA() {
  ArduinoOTA.setHostname(client_id);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.begin();
}


////////////////////////////////////////////////////////////////////////
// Main Loop
////////////////////////////////////////////////////////////////////////
void loop()
{
  // check that we are connected
  if (!client.loop()) {
    mqttConnect();
  }

  unsigned long now = millis();

  if(now - loopStart >= turnTime) {
    // Over one sec from start loop passed
    loopStart = now;

////////////////
    // Main servo loop
    for (int i = 0; i < numberOfServos; i++) {
      servos[i].loop();
    }
////////////////

  }

  // Rest of the loop
#if defined(ESP8266)
  MDNS.update();
#endif
  ArduinoOTA.handle();
}



////////////////////////////////////////////////////////////////////////
// Subfunctions
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// WifiManager - get custom parameter
String getParam(WiFiManager wm, String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

////////////////////////////////////////////////////////////////////////
// CmdServo - get a servo by its id
CmdServo& servoById(int id) {
  for (int i = 0; i < numberOfServos; i++) {
    CmdServo& s = servos[i];
    if (s.getId() == id) {
      return s;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// CmdServo - get a servo by its pin number
CmdServo& servoByPin(int pin) {
  for (int i = 0; i < numberOfServos; i++) {
    CmdServo& s = servos[i];
    if (s.getPin() == pin) {
      return s;
    }
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

////////////////////////////////////////////////////////////////////////
// MQTT - publish an actuator config (SERVO, DIGITAL)
void subscribeAndPublishConfig(int ActuatorId) {
   CmdServo s;

   if (actuator[ActuatorId].Type == tSERVO) {
      s = servoByPin(actuator[ActuatorId].Pin);
      Serial.print("Publishing ha config for servo ");
      Serial.println(String(s.getId()));
      }
   else if ((actuator[ActuatorId].Type == tDIGOUT)||(actuator[ActuatorId].Type == tDIGTEMP)) {
      //actuator[ActuatorId].Pin;
      Serial.print("Publishing ha config for digital output ");
      Serial.println(actuator[ActuatorId].name);
      }
   else {
      // FAIL
      Serial.print("Failed to publish ha config (" + String(ActuatorId) + ")");
      return;
      }


   // Subscribe to the set actuator topic
   char topic[MQTT_TOPIC_MAX_LENGTH];
   sprintf(topic, mqtt_command_topic, String(ActuatorId));

   if (client.subscribe(topic)) {
      // OK
      debugPrint("Subscribed to " CLTYPE " set topic (" + String(ActuatorId) + "), uniqueId = " CLID);
   } else {
      // FAIL
      debugPrint("Failed to subscribe to " CLTYPE " set topic (" + String(ActuatorId) + ")");
   }

   // Subscribe to the set position actuator topic -- only for servos actuators --
   if (actuator[ActuatorId].Type == tSERVO) {
      char set_position_t[MQTT_TOPIC_MAX_LENGTH];
      sprintf(set_position_t, mqtt_set_position_topic, s.getId());
      if (client.subscribe(set_position_t)) {
         // OK
         debugPrint("Subscribed to blinds set position topic (" + String(s.getId()) + "), uniqueId = " CLID);
         publishConfig(ActuatorId); // Publish config right after subscribed to command topic
      } else {
         // FAIL
         debugPrint("Failed to subscribe to blinds set topic (" + String(s.getId()) + ")");
      }
   }

   //publishConfig(ActuatorId); // Publish config right after subscribed to command topic
   }

////////////////////////////////////////////////////////////////////////
// MQTT - get a topic
void mqttCallback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Got MQTT callback! ");
  Serial.print("topic = ");
  Serial.println(topic);
  
  String myString = String(topic);

  String v = getValue(topic, '/', 2); // Get the second value
  if (!v.length()) {
    return;
  }
  
  int ActuatorId = v.toInt(); // Get actuator id
  
  if (ActuatorId>numberOfActuators) { // Wrong id
    return;
  }
  

  // Check
  if (getValue(topic, '/', 0) == client_type) { //Ensure correct client topic
     if(getValue(topic, '/', 1) == client_id) { // Ensure correct device
       String thirdValue = getValue(topic, '/', 3);
       if(thirdValue == "set") { // Ensure set topic
         if (actuator[ActuatorId].Type==tSERVO)
           handleSetServo(payload, length, ActuatorId);
         else if (actuator[ActuatorId].Type==tDIGOUT)
           handleSetDigout(payload, length, ActuatorId);
         else if (actuator[ActuatorId].Type==tDIGTEMP)
           handleSetDigtemp(payload, length, ActuatorId);
       } else if(thirdValue == "position") {
          String fourthValue = getValue(topic, '/', 4);
          if(fourthValue == "set") { // Ensure set topic
            handleSetPosition(payload, length, ActuatorId);
          }
       }
     }
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - Set a servo state
void handleSetServo(byte * payload, unsigned int length, int ActuatorId) {
  CmdServo& s = servoByPin(actuator[ActuatorId].Pin);
  if (!strncmp((char *)payload, "OPEN", length)) {
    s.setOpen();
  } else if (!strncmp((char *)payload, "CLOSE", length)) {
    s.setClose();
  } else if (!strncmp((char *)payload, "STOP", length)) { 
    s.setStop();
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - Set a digital pin
void handleSetDigout(byte * payload, unsigned int length, int ActuatorId) {
  if (!strncmp((char *)payload, "ON", length)) {
    digitalWrite(actuator[ActuatorId].Pin, HIGH);
  } else if (!strncmp((char *)payload, "OFF", length)) {
    digitalWrite(actuator[ActuatorId].Pin, LOW);
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - Set a digital pin with tempo
void handleSetDigtemp(byte * payload, unsigned int length, int ActuatorId) {
  if (!strncmp((char *)payload, "ON", length)) {
    digitalWrite(actuator[ActuatorId].Pin, HIGH);
  } else if (!strncmp((char *)payload, "OFF", length)) { 
    digitalWrite(actuator[ActuatorId].Pin, LOW);
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - Set a servo position
void handleSetPosition(byte * payload, unsigned int length, int ActuatorId) {

  CmdServo& s = servoByPin(actuator[ActuatorId].Pin);
  String myString = (char*)payload;
  int pos = myString.toInt();
  if(pos >= 0 && pos <= 100) {
    s.goToPosition(pos);
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - publish Config : state, command, (position, set_position)
void publishConfig(int ActuatorId) {
  Serial.println("Publishing ha config.");

  DynamicJsonDocument root(JSON_BUFFER_LENGTH);
  
  // State topic
  char state_t[MQTT_TOPIC_MAX_LENGTH];
  sprintf(state_t, mqtt_state_topic, ActuatorId);
  root["state_topic"] = state_t;

  // Command topic
  char command_t[MQTT_TOPIC_MAX_LENGTH];
  sprintf(command_t, mqtt_command_topic, ActuatorId);
  root["command_topic"] = command_t;

   if (actuator[ActuatorId].Type == tSERVO) {
      // Position topics
      char position_t[MQTT_TOPIC_MAX_LENGTH];
      sprintf(position_t, mqtt_position_topic, ActuatorId);
      root["position_topic"] = position_t;

      char set_position_t[MQTT_TOPIC_MAX_LENGTH];
      sprintf(set_position_t, mqtt_set_position_topic, ActuatorId);
      root["set_position_topic"] = set_position_t;
   }

  // Others
  root["name"] = numberOfActuators > 1 ? friendly_name + " " + String(ActuatorId) : friendly_name;
  root["device_class"] = client_type;
  root["unique_id"] = CLTYPE "/" CLID "/" + String(ActuatorId);

  // Device
  addDevice(root);

  // Publish
  String mqttOutput;
  serializeJson(root, mqttOutput);
  
  char t[MQTT_TOPIC_MAX_LENGTH];
  sprintf(t, ha_config_topic, ActuatorId);
  client.beginPublish(t, mqttOutput.length(), true); 
  client.print(mqttOutput);
  client.endPublish();
}

////////////////////////////////////////////////////////////////////////
// ArduinoJson
void addDevice(DynamicJsonDocument& root) {
  JsonObject device = root.createNestedObject("device");
  
  JsonArray identifiers = device.createNestedArray("identifiers");
  identifiers.add(client_id);
  
  device["name"] = client_id;
  
  device["model"] = MODEL;
  device["sw_version"] = SW_VERSION;
}

////////////////////////////////////////////////////////////////////////
// subMQTT - publish debug topic
void debugPrint(String message) {
  if (debug) {
    Serial.println(message);
    // publish to debug topic
    char t[MQTT_TOPIC_MAX_LENGTH];
    sprintf(t, mqtt_debug_topic, client_id);
    client.publish(t, message.c_str());
  }
}

////////////////////////////////////////////////////////////////////////
// CmdServo - position changed -> Do nothing
void positionChanged(int servoId) {
  // Do nothing, only inform position when status is changed
}

////////////////////////////////////////////////////////////////////////
// CmdServo - status changed -> MQTT-publish status and position
void statusChanged(int servoId) {
  CmdServo& s = servoById(servoId);

  String statusMsg = "OPEN";

  switch(s.getStatus()) {
    case CmdServo::OPEN:
      statusMsg = "open";
      break;
    case CmdServo::CLOSED:
      statusMsg = "closed";
      break;
    case CmdServo::CLOSING:
      statusMsg = "closing";
      break;
    case CmdServo::OPENING:
      statusMsg = "opening";
      break;
    default:
      break;
  }

  // Publish status
  char t[MQTT_TOPIC_MAX_LENGTH];
  sprintf(t, mqtt_state_topic, servoId);
  client.publish(t, statusMsg.c_str(), retain_status);

  // Publish position
  char position_t[MQTT_TOPIC_MAX_LENGTH];
  sprintf(position_t, mqtt_position_topic, servoId);
  client.publish(position_t, String(s.currentAngleInPercent()).c_str(), retain_position);
}
