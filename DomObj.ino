
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

// Jero6 02/08/2024
// DEBUG temp manual pumpon

// Jero6 25/11/2021
// Improve robustness in poor wifi connexions, OTA is OK

///
/// \file DomObj.ino
/// \brief Domotic Object in MQTT protocol.
/// \author Jero6
/// \version 1.0
/// \date 25 november 2026
///
/// MQTT object for ESP32/ESP8266 targets, with generic peripherals.
///
///

 
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
#include "time.h"


#include "DomObj.h"
#include "CmdServo.h"


#define JSON_BUFFER_LENGTH 2048
#define MQTT_TOPIC_MAX_LENGTH 256

#define DEBUG true // default value for debug

// Callbacks
void mqttCallback(char* topic, byte* payload, unsigned int payloadLength);
void statusChanged(int servoId);
void positionChanged(int servoId);

// Substitution functions for ESP8266
#if defined(ESP8266)
bool getLocalTime(struct tm * info, uint32_t ms = 5000);
#endif

// Wifi
WiFiClient wifiClient;
unsigned long lastWifiCheck = 0;
unsigned long wifiDownSince = 0;
unsigned long lastMqttRetry = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 10000;  // 10 s

// MQTT
PubSubClient client(mqtt_server, mqtt_port, mqttCallback, wifiClient);
void subscribeAndPublishConfig(int ActuatorId);
void publishConfig(int ActuatorId);

int numberOfActuators = 0;
int numberOfSensors = 0;

CmdServo servos[sizeof(s_actuator)];

// debug
bool debug = DEBUG;
int numberOfServos = 0;
int pos = 0;
void debugPrint(String message);

//String uniqueId;

unsigned long loopStart;

// DEBUG JGA 2/8/24
//#include "time.h"
byte Pump1ON = 0;
byte Pump2ON = 0;
// Assign output variables to GPIO pins
const int output5 = 5;
const int output18 = 18;
// Hour Minute Second variables
unsigned long currentTime = millis();
unsigned long previousMillis = 0;        // will store last time Data was updated
unsigned long interv_tick_second = 1000;           // interval at which to save datas (milliseconds)
struct tm timeinfo;
byte InitTime=0;
byte LocalSecond=0;
byte LocalMinute=0;
byte LocalHour=0;
int DeltaSecond;
//struct tm timeinfo;
// Update things on minute change
byte PreviousLocalMinute=0;
byte isMinuteChanged=0;
byte LocalHourSensorPublished=255;
byte LocalMinuteSaved=0;
int TmpTimeON=30; // Temps PumpON en secondes
String statusMsg;



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

  delay(1000);

  // Setup OTA
  initOTA();

  delay(1000);

  ///////////////

  numberOfActuators = sizeof(actuator) / sizeof(actuator[0]);
  numberOfSensors = sizeof(sensor) / sizeof(sensor[0]);

  //  numberOfServos = sizeof(servoPins) / sizeof(servoPins[0]);
  //  int numberOfReversedServos = sizeof(reversedPins) / sizeof(servoPins[0]); // We might not have any servos as reversed, thus using servoPin size

  Serial.print("numberOfActuators = ");
  Serial.println(numberOfActuators);


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
      if (actuator[i].reversed == false) {
         digitalWrite(actuator[i].Pin, LOW);
         }
      else {
         digitalWrite(actuator[i].Pin, HIGH);
         }
      }

	if (actuator[i].Type == tDIGTEMP) {
      pinMode(actuator[i].Pin, OUTPUT);    // sets the digital pin as output
      if (actuator[i].reversed == false) {
         digitalWrite(actuator[i].Pin, LOW);
         }
      else {
         digitalWrite(actuator[i].Pin, HIGH);
         }
      }

  Serial.print(i);
  Serial.print("-");

    } // numberOfActuators

  Serial.print("numberOfServos = ");
  Serial.println(numberOfServos);


  for (int i = 0; i < numberOfSensors; i++) {
  	if (sensor[i].Type == tANIN) {
      //pinMode(sensor[i].Pin, INPUT);    // sets the digital pin as input
      }
	  if (sensor[i].Type == tDIGIN) {
      pinMode(sensor[i].Pin, INPUT);    // sets the digital pin as input
      }
  Serial.print(i);
  Serial.print("-");
  }

  Serial.print("numberOfSensors = ");
  Serial.println(numberOfSensors);

  ////////////////

  mqttConnect();
  client.setCallback(mqttCallback);
  client.setKeepAlive(30);      // plus tolérant aux réseaux lents
  client.setSocketTimeout(2);

  }

////////////////////////////////////////////////////////////////////////
void wifiConnect() {
  Serial.println("Connecting wifi..");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFi.persistent(true);        // identifiants bien conservés (NVM/flash)
  WiFi.setAutoReconnect(true);  // reconnexion automatique de la couche WiFi


  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();
  wm.setConfigPortalTimeout(180);       // ne bloque jamais indéfiniment au boot
  wm.setConnectRetries(5);              // plusieurs essais avant le portail AP
  wm.setWiFiAutoReconnect(true);
  
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
    Serial.println("Failed to connect, restarting");
    ESP.restart();
    } 

  // Connected to WiFi
  WiFi.setSleep(false);   // IMPORTANT : évite les déconnexions par économie d'énergie

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  InitTime=1; // Ask for Internet time

  // Après une connexion réussie, récupérer les paramètres du portail
  if (strlen(custom_mqtt_server.getValue())) {
    strncpy(mqtt_server,   custom_mqtt_server.getValue(), 40);
    strncpy(mqtt_username, custom_mqtt_username.getValue(), 40);
    strncpy(mqtt_password, custom_mqtt_password.getValue(), 40);
    mqtt_server[40] = '\0';
    mqtt_username[40] = '\0';
    mqtt_password[40] = '\0';
    }

  }

////////////////////////////////////////////////////////////////////////
void mqttConnect() {
  Serial.println("Try to connect mqtt..");

  if (!client.connect(client_id, mqtt_username, mqtt_password)) {
    Serial.println("MQTT connect failed, will retry");
    return;   // pas de boucle bloquante
	}

  if (client.connected()) {
	// resubscribe + config uniquement si connecté
    for (int i = 0; i < numberOfActuators; i++) {
      subscribeAndPublishConfig(i);
      }
	Serial.println("MQTT connected & resubscribed");
    debugPrint("DomObj mqtt connected, v" SW_VERSION);
    }
  }

////////////////////////////////////////////////////////////////////////
void initOTA() {
  Serial.print("InitOTA..");
  ArduinoOTA.setHostname(client_id);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.begin();
  Serial.println("Done!");
}


////////////////////////////////////////////////////////////////////////
// Main Loop
////////////////////////////////////////////////////////////////////////
void loop() {
  unsigned long now = millis();

  ArduinoOTA.handle();
#if defined(ESP8266)
  MDNS.update();
#endif


  // ---------- Gestion WiFi, toutes les 5 secondes ----------
  if (now - lastWifiCheck >= 5000) {
    lastWifiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDownSince == 0) wifiDownSince = now;
      // désactivé le disconnect, peut faire tomber OTA
	  //Serial.println("WiFi down, trying to reconnect...");
      //WiFi.disconnect();
      //WiFi.reconnect();
      // Si le WiFi est tombé depuis 10 minutes, on repart proprement
      if (now - wifiDownSince >= 600000UL) {
        Serial.println("WiFi down too long, restarting...");
        wifiDownSince = 0;
        ESP.restart();
      }
    } else {
      wifiDownSince = 0;
    }
  }

  ////////////////
  // Main servo loop
  if(now - loopStart >= turnTime) {
    // Over one sec from start loop passed
    loopStart = now;

    for (int i = 0; i < numberOfServos; i++) {
      servos[i].loop();
      }
    }
  ////////////////


  // -------- MQTT (seulement si le WiFi est OK), non bloquant --------
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connected()) {
      client.loop();
    }
    else if (now - lastMqttRetry >= MQTT_RECONNECT_INTERVAL) {
      lastMqttRetry = now;
      mqttConnect();   // reconnexion espacée, adoucie
    }
  }




  // Rest of the loop
  // check that we are connected
  if (WiFi.status() == WL_CONNECTED) {
    if ( (InitTime==1) && (getLocalTime(&timeinfo)) ) { //If internet : update the Time
      delay(500);
      LocalSecond = timeinfo.tm_sec;
      LocalMinute = timeinfo.tm_min;
      LocalHour = timeinfo.tm_hour;
      InitTime=0;

      Serial.println("Update time OK");
      }
    }


   currentTime = millis();

   // Update local time
   if (currentTime - previousMillis >= interv_tick_second) {
      DeltaSecond = (currentTime - previousMillis) / interv_tick_second;
      // save the last time the second has been updated
      previousMillis = currentTime;

      LocalSecond = LocalSecond + DeltaSecond;
      if (LocalSecond>59) {
         LocalSecond=LocalSecond-60;
         LocalMinute++;
         if (LocalMinute==60) {
            LocalMinute=0;
            LocalHour++;
            if (LocalHour==24)
               LocalHour=0;
            }
         }
      } // end interv_tick_second

   // Update things on minute change
   if (LocalMinute!=PreviousLocalMinute) {
      PreviousLocalMinute=LocalMinute;
      isMinuteChanged=1;
      }
   else {
      isMinuteChanged=0;
      }

   // Publication MQTT des capteurs analogiques toutes les heures
   if ((LocalMinute==02)&&(isMinuteChanged==1)&&(LocalHourSensorPublished!=LocalHour)) {
//      Serial.print(&timeinfo, "%H:%M:%S : ");

     for (int i = 0; i < numberOfSensors; i++) {
       publishSensor(i);
       }

      LocalHourSensorPublished=LocalHour;
      } // end every hour


  // Acquisition toutes les minutes pour obtenir une valeur moyenne des capteurs analogiques
  if ((LocalMinuteSaved!=LocalMinute)) {
    LocalMinuteSaved=LocalMinute;
    for (int i = 0; i < numberOfSensors; i++) {
      if (sensor[i].Type == tANIN) {
        if (sensor[i].NbVal==0) {
          sensor[i].MeanVal=analogRead(sensor[i].Pin);
          }
        else {
          sensor[i].MeanVal = sensor[i].MeanVal + analogRead(sensor[i].Pin);
          }
        sensor[i].NbVal++;
        }
      }
    }

  } // End loop



////////////////////////////////////////////////////////////////////////
// Subfunctions
////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/// \fn getLocalTime
/// \brief substitution function for ESP8266 target
/// \param info is a time structure
/// \param ms is the timeout in millisecond (default 5000)
/// \return boolean for correct operation
#if defined(ESP8266)
bool getLocalTime(struct tm * info, uint32_t ms)
{
    uint32_t start = millis();
    time_t now;
    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
        delay(10);
    }
    return false;
}
#endif
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
/// \fn servoById
/// \brief get a servo by its id
/// \param id is the id of the servo
/// \return pointer to the CmdServo struct
CmdServo& servoById(int id) {
  for (int i = 0; i < numberOfServos; i++) {
    CmdServo& s = servos[i];
    if (s.getId() == id) {
      return s;
    }
  }
}

////////////////////////////////////////////////////////////////////////
/// \fn servoByPin
/// \brief get a servo by its pin number
/// \param pin is the pin number of the CPU connected to the servo
/// \return pointer to the CmdServo struct
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
      Serial.println(actuator[ActuatorId].Name);
      }
   else {
      // FAIL
      Serial.print("Failed to publish ha config (" + String(ActuatorId) + ")");
      return;
      }


   // Subscribe to the set actuator topic
   char topic[MQTT_TOPIC_MAX_LENGTH];
   String stractuatorId = String(ActuatorId);
   sprintf(topic, mqtt_command_topic, ActuatorId); //stractuatorId.c_str());

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
      String strId = String(s.getId());
      sprintf(set_position_t, mqtt_set_position_topic, ActuatorId); //strId.c_str());
      if (client.subscribe(set_position_t)) {
         // OK
         debugPrint("Subscribed to " CLTYPE " set position topic (" + String(s.getId()) + "), uniqueId = " CLID);
         publishConfig(ActuatorId); // Publish config right after subscribed to command topic
      } else {
         // FAIL
         debugPrint("Failed to subscribe to " CLTYPE " set topic (" + String(s.getId()) + ")");
      }
   }

   publishConfig(ActuatorId); // Publish config right after subscribed to command topic
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
      if (actuator[ActuatorId].reversed == false) {
         digitalWrite(actuator[ActuatorId].Pin, HIGH);
         }
      else {
         digitalWrite(actuator[ActuatorId].Pin, LOW);
         }
  } else if (!strncmp((char *)payload, "OFF", length)) {
      if (actuator[ActuatorId].reversed == false) {
         digitalWrite(actuator[ActuatorId].Pin, LOW);
         }
      else {
         digitalWrite(actuator[ActuatorId].Pin, HIGH);
         }
  }
}

////////////////////////////////////////////////////////////////////////
// subMQTT - Set a digital pin with tempo
void handleSetDigtemp(byte * payload, unsigned int length, int ActuatorId) {
  if (!strncmp((char *)payload, "ON", length)) {
      if (actuator[ActuatorId].reversed == false) {
         digitalWrite(actuator[ActuatorId].Pin, HIGH);
         }
      else {
         digitalWrite(actuator[ActuatorId].Pin, LOW);
         }
  } else if (!strncmp((char *)payload, "OFF", length)) { 
      if (actuator[ActuatorId].reversed == false) {
         digitalWrite(actuator[ActuatorId].Pin, LOW);
         }
      else {
         digitalWrite(actuator[ActuatorId].Pin, HIGH);
         }
  }
// DEBUG JGA 2/8/24
debugPrint("SetDigTemp");
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



////////////////////////////////////////////////////////////////////////
// MQTT-publish sensor
void publishSensor(int sensorId) {
  int sensorValue;
  char cstatusMsg[20];
  String statusMsg;

	if (sensor[sensorId].Type == tANIN) {
    sensorValue = (sensor[sensorId].MeanVal)/(sensor[sensorId].NbVal);
    sensor[sensorId].NbVal=0;
    sprintf(cstatusMsg, "%d", sensorValue);
    }

  statusMsg=cstatusMsg;

	if (sensor[sensorId].Type == tDIGIN) {
    if (digitalRead(sensor[sensorId].Pin))
      statusMsg = "open";
    else
      statusMsg = "closed";
    }


  // Publish status
//  String statusMsg = "0";
  Serial.println(statusMsg);
  char t[MQTT_TOPIC_MAX_LENGTH];
  String strsensorId = String(sensorId);
  sprintf(t, mqtt_sensor_topic, sensorId); //strsensorId.c_str());
  client.publish(t, statusMsg.c_str());

}
