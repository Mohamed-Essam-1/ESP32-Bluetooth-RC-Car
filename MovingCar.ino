#include <BluetoothSerial.h>
#include <ESP32Servo.h>
// ------------------------------------------------------------------
// Pin Definitions
// ------------------------------------------------------------------
// NOTE: ENA was originally on GPIO34, which is an INPUT-ONLY pin on the
// ESP32 (no output driver at all) -> analogWrite() would never work there.
// Moved it to GPIO32 which supports PWM output.
#define trig 5   // Ultrasonic sensor trigger
#define echo 18  // Ultrasonic sensor echo (digital input)
#define ENA 32   // PWM Right Motor Speed  (was 34 - INPUT ONLY, fixed)
#define in1 14
#define in2 27
#define ENB 33  // PWM Left Motor Speed
#define in3 26
#define in4 25
#define led1 19  // front light
#define led2 21  // back light
#define buzzer 22
#define SERVO_PIN 13

Servo servo;
BluetoothSerial bt;
char reading = 'S';
int currentSpeed = 255;

//------------------------------------------------------------------------------------------------------
// set the speed
//------------------------------------------------------------------------------------------------------------
void updatespeed() {
  if (reading == '0') currentSpeed = 0;
  else if (reading == '1') currentSpeed = 42;
  else if (reading == '2') currentSpeed = 85;
  else if (reading == '3') currentSpeed = 127;
  else if (reading == '4') currentSpeed = 170;
  else if (reading == '5') currentSpeed = 212;
  else if (reading == '6') currentSpeed = 255;

  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed);
}

//-----------------------------------------------------------------------------------
// move the car forward
//------------------------------------------------------------------------------------
void moveforward() {
  digitalWrite(in1, 1);
  digitalWrite(in2, 0);
  digitalWrite(in3, 1);
  digitalWrite(in4, 0);
}

//-----------------------------------------------------------------------------------
// move the car backward
//-----------------------------------------------------------------------------------
void movebackward() {
  digitalWrite(in1, 0);
  digitalWrite(in2, 1);
  digitalWrite(in3, 0);
  digitalWrite(in4, 1);
}

//------------------------------------------------------------------------------------
// stop the car
//--------------------------------------------------------------------------------------
void stopCar() {
  digitalWrite(in1, 0);
  digitalWrite(in2, 0);
  digitalWrite(in3, 0);
  digitalWrite(in4, 0);
}

//----------------------------------------------------------------------------------
// move the car forward right
//---------------------------------------------------------------------------------
void moveforwardright() {
  analogWrite(ENA, currentSpeed / 3);  // Slow right wheel
  analogWrite(ENB, currentSpeed);      // Full speed left wheel
  digitalWrite(in1, 1);
  digitalWrite(in2, 0);
  digitalWrite(in3, 1);
  digitalWrite(in4, 0);
}

//----------------------------------------------------------------------------------
// move the car forward left
//---------------------------------------------------------------------------------
void moveforwardleft() {
  analogWrite(ENA, currentSpeed);      // Full speed right wheel
  analogWrite(ENB, currentSpeed / 3);  // Slow left wheel
  digitalWrite(in1, 1);
  digitalWrite(in2, 0);
  digitalWrite(in3, 1);
  digitalWrite(in4, 0);
}

//-------------------------------------------------------------------------------------------
// move the car back left
//-----------------------------------------------------------------------------------------
void movebackleft() {
  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed / 3);
  digitalWrite(in1, 0);
  digitalWrite(in2, 1);
  digitalWrite(in3, 0);
  digitalWrite(in4, 1);
}

//-------------------------------------------------------------------------------------------
// move the car back right
//-----------------------------------------------------------------------------------------
void movebackright() {
  analogWrite(ENA, currentSpeed / 3);
  analogWrite(ENB, currentSpeed);
  digitalWrite(in1, 0);
  digitalWrite(in2, 1);
  digitalWrite(in3, 0);
  digitalWrite(in4, 1);
}

//----------------------------------------------------------------------------------------
// open or close the front light
//---------------------------------------------------------------------------------------------
void openorclosefrontlight() {
  if (reading == 'W') {
    digitalWrite(led1, 1);
  }
  if (reading == 'w') {
    digitalWrite(led1, 0);
  }
}
//----------------------------------------------------------------------------------------
// open or close the back light
//---------------------------------------------------------------------------------------------
void openorclosebacklight() {
  if (reading == 'U') {
    digitalWrite(led2, 1);
  }
  if (reading == 'u') {
    digitalWrite(led2, 0);
  }
}
//--------------------------------------------------------------------------------
// make the move when press
//---------------------------------------------------------------------------------------
void processMovement() {
  if (reading == 'F') moveforward();
  else if (reading == 'B') movebackward();  // FIXED: was "movebackwaard" (typo, wouldn't compile)
  else if (reading == 'S') stopCar();
  else if (reading == 'I') moveforwardright();
  else if (reading == 'G') moveforwardleft();
  else if (reading == 'H') movebackleft();
  else if (reading == 'J') movebackright();
}

//---------------------------------------------------------------------------------
// stop the car if there is an object in front of you
//---------------------------------------------------------------------------------
void crashsystem() {
  digitalWrite(trig, 1);
  delayMicroseconds(10);
  digitalWrite(trig, 0);

  // FIXED: added a timeout (30000us ~= 5m range) so pulseIn() can't block
  // the whole loop for up to 1 second when no echo returns (e.g. nothing
  // in range). Without this, Bluetooth control would freeze/lag badly.
  float time = pulseIn(echo, HIGH, 30000);

  // FIXED: speed of sound constant corrected from 0.035 to 0.034 cm/us
  float distanceincm = time * 0.034 / 2;

  bool isMovingForward = (reading == 'F' || reading == 'I' || reading == 'G');

  // time == 0 means pulseIn timed out (no object in range) -> ignore
  if (time > 0 && distanceincm > 0 && distanceincm < 20 && isMovingForward) {
    stopCar();
    reading = 'S';  // ADDED: also reset command so it doesn't keep re-triggering forward next loop
  }
}

//------------------------------------------------------------------------------
// horn
//-------------------------------------------------------------------------------
void horn() {
  if (reading == 'V') {
    tone(buzzer, 440);
    delay(150);
    tone(buzzer, 587);
    delay(150);
    noTone(buzzer);
  } else {
    noTone(buzzer);
  }
}
//------------------------------------------------------------------------------
// Servo
//-------------------------------------------------------------------------------
void servoControl(char data) {

  int angle;

  if (data >= '0' && data <= '9') {

    int value = data - '0';

    angle = map(value, 0, 9, 0, 180);

    servo.write(angle);

    Serial.print("Slider: ");
    Serial.print(value);
    Serial.print(" -> Servo: ");
    Serial.println(angle);
  }

  else if (data == 'q') {

    servo.write(180);

    Serial.println("Slider: 100 -> Servo: 180");
  }
}
//----------------------------------------------------------------------------------
// setup
//---------------------------------------------------------------------------------
void setup() {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(ENA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  servo.attach(SERVO_PIN);
  servo.write(90);
  bt.begin("carbt");
  digitalWrite(trig, 0);
}

//-----------------------------------------------------------------------------------
// loop
//------------------------------------------------------------------------------------
void loop() {

  void servoControl(char data);

  if (bt.available()) {

    char data = bt.read();

    reading = data;

    if ((data >= '0' && data <= '9') || data == 'q') {
      servoControl(data);
    }
  }

  updatespeed();
  processMovement();
  crashsystem();
  openorclosefrontlight();
  openorclosebacklight();
  horn();
}