
//#define __VMC1LR__
#define __ARROSLR__

#define SW_VERSION "0.1"

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
  } actuator[] = {{"servo1", tSERVO, D7, false},
			            {"servo2", tSERVO, D6, false},
			            {"servo3", tSERVO, D5, false},
			            {"servo4", tSERVO, D4, false},
			            {"servo5", tSERVO, D3, false},
			            {"servo6", tSERVO, D2, false},
			            {"servo7", tSERVO, D1, false},
			            {"poweron", tDIGOUT, D8, false}};


const struct s_sensor {
  char Name[13];
  e_sensor Type;
  int Pin;
} sensor[1] = {{"powerstatus", tDIGIN, D9}};

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
			         {"LED", tDIGOUT, 1, false}};


const struct s_sensor {
  char name[13];
  e_sensor Type;
  int Pin;
} sensor[1] = {{"soil", tANIN, 36}};

#endif // __ARROSLR__


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
const char* mqtt_state_topic        = CLTYPE "/" CLID "/%s/state";
const char* mqtt_command_topic      = CLTYPE "/" CLID "/%s/set";
const char* mqtt_position_topic     = CLTYPE "/" CLID "/%s/position";
const char* mqtt_set_position_topic = CLTYPE "/" CLID "/%s/position/set";
const char* mqtt_debug_topic        = CLTYPE "/" CLID "/%s/debug"; // debug topic
const char* ha_config_topic           = "homeassistant/cover/" CLTYPE "/" CLID "/%s/config";
