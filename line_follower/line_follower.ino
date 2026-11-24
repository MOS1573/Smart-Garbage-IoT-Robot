// Line follower with obstacle avoidance (Arduino Uno).
//
// Two IR sensors follow a black line on a light floor. An HC-SR04 on a servo
// looks ahead; when something blocks the line, the robot scans left and right
// and drives around the obstacle on the side with more room.
//
// The motors are switched by a 2-channel relay module, so they always run at
// full speed: there are no enable / PWM pins.

// Motor relays
#define in1 9   // left motor, backward
#define in2 8   // left motor, forward
#define in3 7   // right motor, forward
#define in4 6   // right motor, backward

// IR line sensors: 0 = white floor, 1 = black line
#define L_S A0
#define R_S A1

// HC-SR04 ultrasonic sensor
#define echo    A2
#define trigger A3

// Servo that turns the ultrasonic sensor
#define servo A5

const int SET_DISTANCE_CM = 15;  // closer than this counts as an obstacle

int distance_L, distance_F, distance_R;

void setup() {
  Serial.begin(9600);

  pinMode(R_S, INPUT);
  pinMode(L_S, INPUT);
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  pinMode(servo, OUTPUT);

  // Sweep the sensor once at start-up and leave it facing forward (70 deg).
  for (int angle = 70; angle <= 140; angle += 5) servoPulse(servo, angle);
  for (int angle = 140; angle >= 0; angle -= 5)  servoPulse(servo, angle);
  for (int angle = 0; angle <= 70; angle += 5)   servoPulse(servo, angle);
  delay(500);
}

void loop() {
  distance_F = ultrasonicRead();
  Serial.print("D F="); Serial.println(distance_F);

  const int right = digitalRead(R_S);
  const int left = digitalRead(L_S);

  if (right == 0 && left == 0) {          // both on white: line is between them
    if (distance_F > SET_DISTANCE_CM) forward();
    else checkSide();
  } else if (right == 1 && left == 0) {   // right sensor on the line: turn right
    turnRight();
  } else if (right == 0 && left == 1) {   // left sensor on the line: turn left
    turnLeft();
  }
  // Both on black (a crossing or the end of the track): keep the last command.

  delay(10);
}

// Software servo pulse: 500 us at 0 deg, about 11 us per degree.
void servoPulse(int pin, int angle) {
  const int pulse = (angle * 11) + 500;
  digitalWrite(pin, HIGH);
  delayMicroseconds(pulse);
  digitalWrite(pin, LOW);
  delay(50);  // servo refresh period
}

// Distance in cm: speed of sound is ~29 us/cm, and the echo travels both ways.
long ultrasonicRead() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  const long time = pulseIn(echo, HIGH, 30000);  // time out after ~5 m
  if (time == 0) return 999;                      // nothing in range
  return time / 29 / 2;
}

// Stop, look right and left, then avoid the obstacle.
void checkSide() {
  Stop();
  delay(100);

  for (int angle = 70; angle <= 140; angle += 5) servoPulse(servo, angle);
  delay(300);
  distance_R = ultrasonicRead();
  Serial.print("D R="); Serial.println(distance_R);
  delay(100);

  for (int angle = 140; angle >= 0; angle -= 5) servoPulse(servo, angle);
  delay(500);
  distance_L = ultrasonicRead();
  Serial.print("D L="); Serial.println(distance_L);
  delay(100);

  for (int angle = 0; angle <= 70; angle += 5) servoPulse(servo, angle);
  delay(300);
  compareDistance();
}

// Drive around the obstacle on the freer side and turn back towards the line.
void compareDistance() {
  if (distance_L > distance_R) {
    turnLeft();  delay(500);
    forward();   delay(600);
    turnRight(); delay(500);
    forward();   delay(600);
    turnRight(); delay(400);
  } else {
    turnRight(); delay(500);
    forward();   delay(600);
    turnLeft();  delay(500);
    forward();   delay(600);
    turnLeft();  delay(400);
  }
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------
void forward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void backward() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void turnRight() {  // left wheel forward, right wheel backward
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void turnLeft() {   // left wheel backward, right wheel forward
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void Stop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
