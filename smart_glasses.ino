#include <Wire.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <MPU6050.h>

const int trigPin = 9;
const int echoPin = 10;
const int dfPlayerRxPin = 6;
const int dfPlayerTxPin = 7;
const int dfPlayerBusyPin = 4;
const int buzzerPin = 8;
const int mpuIntPin = 2;

const float OBSTACLE_THRESHOLD_CM = 80.0;
const float MIN_DISTANCE_CM = 4.0;
const int MIN_FREQUENCY = 300;
const int MAX_FREQUENCY = 2500;
const int HEAD_TURN_THRESHOLD = 50;

const int TRACK_STARTUP = 1;
const int TRACK_OBSTACLE = 2;
const int TRACK_HEADTURN = 3;

// SoftwareSerial constructor order is (Arduino RX, Arduino TX).
// This maps Arduino D7 <- DFPlayer TX and Arduino D6 -> DFPlayer RX.
SoftwareSerial mySoftwareSerial(dfPlayerTxPin, dfPlayerRxPin);
DFRobotDFPlayerMini myDFPlayer;
MPU6050 mpu;

volatile bool mpuInterrupt = false;
bool headAlerted = false;
long gx_offset = 0, gy_offset = 0, gz_offset = 0;

enum SystemState { IDLE, MP3_PLAYING, BUZZER_ACTIVE };
SystemState currentState = IDLE;
bool waitingToPlayObstacle = false;

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(dfPlayerBusyPin, INPUT);
  pinMode(mpuIntPin, INPUT);
  noTone(buzzerPin);

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Mini not detected."));
    while (true) {}
  }

  myDFPlayer.volume(30);
  myDFPlayer.stop();
  myDFPlayer.play(TRACK_STARTUP);
  delay(2000);
  myDFPlayer.stop();

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println(F("MPU6050 connection failed!"));
    while (true) {}
  }

  calibrateMPU();
  attachInterrupt(digitalPinToInterrupt(mpuIntPin), onMotionDetected, RISING);
  mpu.setIntMotionEnabled(true);
  mpu.setMotionDetectionThreshold(5);
  mpu.setMotionDetectionDuration(5);

  Serial.println(F("System ready."));
  currentState = IDLE;
}

void loop() {
  float distanceCm = getDistance();
  bool obstaclePresent = (distanceCm > 0 && distanceCm < OBSTACLE_THRESHOLD_CM);
  bool playerIsBusy = (digitalRead(dfPlayerBusyPin) == LOW);
  bool headTilted = isHeadTilted();

  switch (currentState) {
    case IDLE:
      noTone(buzzerPin);
      if (headTilted && !headAlerted) {
        Serial.println(F("** HEAD TURN DETECTED **"));
        if (!playerIsBusy) myDFPlayer.play(TRACK_HEADTURN);
        headAlerted = true;
        if (obstaclePresent) waitingToPlayObstacle = true;
      }

      if (!headTilted) headAlerted = false;

      if (obstaclePresent && !headTilted && !playerIsBusy) {
        Serial.print(F("Obstacle at "));
        Serial.print(distanceCm);
        Serial.println(F(" cm -> Playing warning..."));
        myDFPlayer.play(TRACK_OBSTACLE);
        delay(100);
        currentState = MP3_PLAYING;
      }

      if (waitingToPlayObstacle && !playerIsBusy && !headTilted) {
        Serial.println(F("Playing delayed obstacle warning..."));
        myDFPlayer.play(TRACK_OBSTACLE);
        delay(100);
        currentState = MP3_PLAYING;
        waitingToPlayObstacle = false;
      }
      break;

    case MP3_PLAYING:
      noTone(buzzerPin);
      if (!obstaclePresent) {
        currentState = IDLE;
      } else if (!playerIsBusy) {
        currentState = BUZZER_ACTIVE;
      }
      break;

    case BUZZER_ACTIVE:
      if (!obstaclePresent) {
        noTone(buzzerPin);
        currentState = IDLE;
      } else {
        float effectiveDistance = constrain(distanceCm, MIN_DISTANCE_CM, OBSTACLE_THRESHOLD_CM);
        int frequency = map(effectiveDistance, MIN_DISTANCE_CM, OBSTACLE_THRESHOLD_CM,
                            MAX_FREQUENCY, MIN_FREQUENCY);
        tone(buzzerPin, frequency);
      }
      break;
  }

  delay(50);
}

void onMotionDetected() {
  mpuInterrupt = true;
}

float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.0343 / 2.0;
}

bool isHeadTilted() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  gy -= gy_offset;
  gz -= gz_offset;

  float gForceY = gy / 131.0;
  float gForceZ = gz / 131.0;
  return (abs(gForceY) > HEAD_TURN_THRESHOLD || abs(gForceZ) > HEAD_TURN_THRESHOLD);
}

void calibrateMPU() {
  Serial.println(F("Calibrating MPU6050..."));
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for (int i = 0; i < 1000; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;
    delay(3);
  }

  gx_offset = gx_sum / 1000;
  gy_offset = gy_sum / 1000;
  gz_offset = gz_sum / 1000;

  Serial.println(F("MPU6050 Gyroscope Offsets:"));
  Serial.print(F("gx offset: ")); Serial.println(gx_offset);
  Serial.print(F("gy offset: ")); Serial.println(gy_offset);
  Serial.print(F("gz offset: ")); Serial.println(gz_offset);
}
