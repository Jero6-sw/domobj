#include "CmdServo.h"

CmdServo::CmdServo(int id, int servoPin, int minValue, int maxValue, int maxDegree, boolean reversed, boolean debug) {
  init(id, servoPin, minValue, maxValue, maxDegree, reversed, debug);
}

CmdServo::CmdServo() {
      
}

CmdServo::~CmdServo() {
  servo.detach();
}

void CmdServo::init(int id, int servoPin, int minPulseValue, int maxPulseValue, int maxDegree, boolean reversed, boolean debug) {
  this->id = id;
  this->servoPin = servoPin;
  this->servoMinPulse = minPulseValue;
  this->servoMaxPulse = maxPulseValue;
  this->servoMaxDegree = maxDegree;
  this->isDebug = debug;
  this->isReversed = reversed;
  attached = false;
}

void CmdServo::attach() {
  servo.attach(servoPin, servoMinPulse, servoMaxPulse);
  attached = true;
}

void CmdServo::detach() {
  servo.detach();
  attached = false;
}

void CmdServo::goToAngle(int angle) {
  debugPrint("Go to angle: " + String(angle));
  
  // Ensure limits
  if(angle > servoMaxDegree || angle < 0) {
    return;
  }
  
  if (isMoving()) {
    setStop(); // Stop
  }
  
  target = angle;
}


void CmdServo::setStop() {
  // Cancel current move
  debugPrint("Stop moving!");
  target = currentPosition;

  statusChangedCallback(id);
}

void CmdServo::setOpen() {
  goToAngle(servoMaxDegree);  
}

void CmdServo::setClose() {
  goToAngle(0);
}

void CmdServo::goToPosition(int position) {
  if(position >= 0 && position <= 100) {
    float angle = (float) ((float) position / (float) 100) * (float) 270;
    int res = (int)round(angle);
    goToAngle(res);
  }
}

void CmdServo::loop() {
  // Do the main loop
  if (currentPosition != target && attached == false) {
    attach();
  }

  // Before move, we take the prev position
  previousPosition = currentPosition;
  CmdStatus currentStatus = getStatus();

  if(currentPosition < target){
    servo.writeMicroseconds(angleToServo(currentPosition++));
    if (previousStatus != currentStatus) {
      statusChangedCallback(id);
    }
    positionChangedCallback(id);
  }
  else if(currentPosition > target){
    servo.writeMicroseconds(angleToServo(currentPosition--));
    if (previousStatus != currentStatus) {
      statusChangedCallback(id);
    }
    positionChangedCallback(id);
  }

  // Notify that we have reached the target
  if (currentPosition == target && previousPosition != currentPosition) {
    detach();
    statusChangedCallback(id);
    positionChangedCallback(id);
  }

  previousStatus = currentStatus;
}

boolean CmdServo::isOpening() {
  return target > currentPosition;
}

boolean CmdServo::isClosing() {
  return target < currentPosition;
}

int CmdServo::currentAngleInPercent() {
  return (int)round(((float) currentPosition / (float) servoMaxDegree) * 100);
}
    
int CmdServo::getId() {
  return id;
}
int CmdServo::getPin() {
  return servoPin;
}
int CmdServo::getAngle() {
  return currentPosition;
}

boolean CmdServo::isClosed() {
  return currentPosition == 0;
}

boolean CmdServo::isMoving() {
  return currentPosition != target;
}

CmdServo::CmdStatus CmdServo::getStatus() {
  if (currentPosition == 0) {
    return CmdServo::CLOSED;
  } else if(target < currentPosition) {
    return CmdServo::CLOSING;
  } else if(target > currentPosition) {
    return CmdServo::OPENING;
  }

  return CmdServo::OPEN;
}


void CmdServo::setDebug(bool debug) {
  isDebug = debug;
}

void CmdServo::debugPrint(String text) {
  if (isDebug) {
    String value = "(" + String(id) + ") " + text;
    debugPrintCallback(value);
  }
}

void CmdServo::setDebugPrintCallback(DEBUG_PRINT_CALLBACK_SIGNATURE) {
  if (isDebug) {
    this->debugPrintCallback = debugPrintCallback;
  }
}

void CmdServo::setStatusChangedCallback(STATUS_CHANGED_CALLBACK_SIGNATURE) {
  this->statusChangedCallback = statusChangedCallback;
}

void CmdServo::setPositionChangedCallback(POSITION_CHANGED_CALLBACK_SIGNATURE) {
  this->positionChangedCallback = positionChangedCallback;
}

// Privates
int CmdServo::angleToServo(int angle){
  // Convert from min - max to 0 - 270

  // formula: (degree * (max - min / servoMax)) + offset 
  float result = (angle * (float)((float)(servoMaxPulse - servoMinPulse) / (float)servoMaxDegree)) + servoMinPulse;

  int res = (int)round(result);

  if(isReversed) { // Servo reversing support
    res = (servoMaxPulse + servoMinPulse) - res;
  }

  // Ensure result is valid
  if (res < servoMinPulse || res > servoMaxPulse) {
    return servoMinPulse;
  }

  return res;
}
