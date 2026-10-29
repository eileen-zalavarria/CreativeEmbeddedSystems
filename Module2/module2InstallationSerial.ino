/*
  LILYGO Joystick, Potentiometer, Button
  sends one line per reading to Processing: joyX,joyY,potentiometer,button
*/
#define BUTTON_PIN 2
#define POTENTIOMETER_PIN 36

int xyzPins[] = {39, 32, 33};   // x, y, z (switch) pins

void setup() {
  Serial.begin(9600);
  pinMode(xyzPins[2], INPUT_PULLUP);  // joystick 
  pinMode(BUTTON_PIN, INPUT_PULLUP); // button
}

void loop() {
  int xVal = analogRead(xyzPins[0]);
  int yVal = analogRead(xyzPins[1]);
  int potVal = analogRead(POTENTIOMETER_PIN);
  int pressed = (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;   // is 1 when pressed

  Serial.printf("%d,%d,%d,%d\n", xVal, yVal, potVal, pressed);
  delay(20);
}