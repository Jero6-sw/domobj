///
/// \file DomObj.h
/// \brief Domotic Object in MQTT protocol.
/// \author Jero6
/// \version 0.6
/// \date 18 october 2025
///
/// MQTT object for ESP32/ESP8266 targets, with generic peripherals.
///
///

//#define __VMC1LR__
//#define __ARROSLR__
#define __LITGOD__

#define SW_VERSION "0.7"

enum e_actuator { tSERVO, tDIGOUT, tDIGTEMP};
enum e_sensor { tDIGIN, tANIN, tTIC};

////////////////////////////////////////////////////////////////////////
// Compilation for VMC - ESP8266
#ifdef __VMC1LR__

#define CLTYPE "domobj"
#define CLID "vmc1"

const struct s_actuator {
  char Name[13];
  e_actuator Type;
  int Pin;
  boolean reversed;
  } actuator[] = {{"servo1", tSERVO, 4, false},
			         {"servo2", tSERVO, 0, false},
			         {"servo3", tSERVO, 2, false},
			         {"servo4", tSERVO, 15, false},
			         {"servo5", tSERVO, 12, false},
                  {"servo6", tSERVO, 14, false},
                  {"poweron12", tDIGOUT, 5, true},
			         {"poweron34", tDIGOUT, 13, true},
			         {"poweron56", tDIGOUT, 16, true}};


struct s_sensor {
  char Name[13];
  e_sensor Type;
  long MeanVal;
  byte NbVal;
  int Pin;
} sensor[] = {{"powerrb", tANIN, 0, 0, 6}}; // pin A0/6/TOUT for ESP8266

#endif // __VMC1LR__

////////////////////////////////////////////////////////////////////////
// Compilation for watering module - ESP32
#ifdef __ARROSLR__

#define CLTYPE "domobj"
#define CLID "arroslr"

const struct s_actuator {
  char name[13];
  e_actuator Type;
  int Pin;
  boolean reversed;
  } actuator[] = {{"pump1", tDIGTEMP, 5, false},
			         {"pump2", tDIGTEMP, 18, false},
			         {"LED", tDIGOUT, 19, false}
               };


struct s_sensor {
  char name[13];
  e_sensor Type;
  long MeanVal;
  byte NbVal;
  int Pin;
} sensor[] = {{"soil", tANIN, 0, 0, 36}};

#endif // __ARROSLR__

////////////////////////////////////////////////////////////////////////
// Compilation for Litiere Auto Godet - ESP8266
#ifdef __LITGOD__

#define CLTYPE "domobj"
#define CLID "LitGod"

const struct s_actuator {
  char Name[13];
  e_actuator Type;
  int Pin;
  boolean reversed;
  } actuator[] = {{"poweron12", tDIGOUT, 5, true},
			         {"poweron34", tDIGOUT, 13, true},
			         {"poweron56", tDIGOUT, 16, true}};


struct s_sensor {
  char Name[13];
  e_sensor Type;
  long MeanVal;
  byte NbVal;
  int Pin;
} sensor[] = {{"powerrb", tANIN, 0, 0, 6}}; // pin A0/6/TOUT for ESP8266

#endif // __VMC1LR__

////////////////////////////////////////////////////////////////////////
// wifi settings
//const char* ssid     = ""; // Your WiFi SSID
//const char* password = ""; // Your WiFi password

////////////////////////////////////////////////////////////////////////
// OTA Settings
const char* ota_password = "BlindsOTA";

////////////////////////////////////////////////////////////////////////
// Servo pins to use (each servo to be placed on it's own IO pin)
// To set servos turn as reversed, set reversed pins array to wich servos should turn opposite direction. example: { D7, D6 }
// Reversed servos should be subset of servoPins e.g. reversedPin must be also in servoPins array!
//const unsigned int servoPins[] = { D7, D6, D5, D4, D3, D2, D1 };
//const unsigned int reversedPins[] = { }; // Array of reversed servo pins. Must be subset of servoPins.
const unsigned int turnTime = 50; // how many milliseconds after one point of turn (must be greater than amount of turn time of all the servos)


////////////////////////////////////////////////////////////////////////
// mqtt server settings
char* mqtt_server   = "192.168.8.50"; // Your MQTT server address
const int mqtt_port       = 1883; // Your MQTT server port
char* mqtt_username = "guest"; // Your MQTT user
char* mqtt_password = "guest"; // Your MQTT password

////////////////////////////////////////////////////////////////////////
// mqtt client settings
const char* client_type = CLTYPE; // Must be unique on the MQTT network
const char* client_id = CLID; // Must be unique on the MQTT network
const boolean retain_status = true; // Retain status messages (keeps blinds status available after HA reset)
const boolean retain_position = true; // Retain position messages

////////////////////////////////////////////////////////////////////////
// Home assistant configuration
// Friendly name of the device. If using multiple servos, a number will be appended at the end of the name e.g. "Blinds 1", "Blinds 2"
const String friendly_name = CLTYPE "s";

// -- Below should not be changed if using same servos as mentioned in the  blog post
// Servo settings
const int servo_min_pulse = 500;
const int servo_max_pulse = 2500;
const int servo_max_angle = 270;

////////////////////////////////////////////////////////////////////////
// Mqtt topics (for advanced use, no need to modify)
const char* mqtt_state_topic        = CLTYPE "/" CLID "/%d/state";
const char* mqtt_sensor_topic       = CLTYPE "/" CLID "/%d/sensor";
const char* mqtt_command_topic      = CLTYPE "/" CLID "/%d/set";
const char* mqtt_position_topic     = CLTYPE "/" CLID "/%d/position";
const char* mqtt_set_position_topic = CLTYPE "/" CLID "/%d/position/set";
const char* mqtt_debug_topic        = CLTYPE "/" CLID "/%s/debug"; // debug topic
const char* ha_config_topic           = "homeassistant/cover/" CLTYPE "/" CLID "/%d/config";
