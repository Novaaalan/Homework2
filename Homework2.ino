/*
  INTERACTIVE PLUSH TOY
  Mood System + Photoresistor Sleeping Mode

  RGB LED:
    R  -> D9
    G  -> D10
    B  -> D11
    V  -> 5V

  Button module:
    IN  -> D2
    VCC -> 5V
    GND -> GND

  Photoresistor module:
    A0  -> Arduino A0
    VCC -> 5V
    GND -> GND
    D0  -> Not connected

  MOOD:
    0 points  = Blue (very sad)
    10 points = Green (neutral)
    20 points = Red (very happy)

  SLEEP:
    Cover photoresistor = Pink
    Uncover photoresistor = Wake up, reset to 10 (green)

  IMPORTANT:
    This version assumes the photoresistor reading
    INCREASES when covered.
*/

// ----- PIN SETTINGS -----

const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

const int BUTTON_PIN = 2;
const int PHOTO_PIN = A0;

// RGB module with V connected to 5V
const bool COMMON_ANODE = true;

// Change to LOW if your button outputs LOW when pressed.
const int PRESSED_LEVEL = HIGH;

// ----- MOOD SETTINGS -----

int mood = 10;

const int MIN_MOOD = 0;
const int MAX_MOOD = 20;

const unsigned long SAD_INTERVAL = 1500UL;
const unsigned long DEBOUNCE_MS = 40UL;

// ----- PHOTORESISTOR SETTINGS -----

// Adjust these after checking Serial Monitor.
const int DARK_THRESHOLD = 300;
const int LIGHT_THRESHOLD = 350;

bool sleeping = false;

// ----- TIMERS AND BUTTON STATES -----

unsigned long lastPetTime = 0;
unsigned long lastDebounceTime = 0;
unsigned long lastLightUpdate = 0;

int lastReading = LOW;
int stableButtonState = LOW;


// ----- RGB LED CONTROL -----

void setRGB(int red, int green, int blue) {
  red = constrain(red, 0, 255);
  green = constrain(green, 0, 255);
  blue = constrain(blue, 0, 255);

  if (COMMON_ANODE) {
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  }

  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}


// ----- MOOD COLOR -----

void updateLED() {

  // Sleeping overrides all mood colors.
  if (sleeping) {
    // Pink = red + blue
    setRGB(255, 40, 130);
    return;
  }

  int red = 0;
  int green = 0;
  int blue = 0;

  // 0 points = blue
  // 10 points = green
  // 20 points = red

  if (mood <= 10) {
    green = map(mood, 0, 10, 0, 255);
    blue = map(mood, 0, 10, 255, 0);
  }
  else {
    red = map(mood, 10, 20, 0, 255);
    green = map(mood, 10, 20, 255, 0);
  }

  setRGB(red, green, blue);
}


// ----- SETUP -----

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(9600);

  mood = 10;
  lastPetTime = millis();

  updateLED();

  Serial.println("Toy started!");
  Serial.println("Mood = 10 (GREEN)");
}


// ----- MAIN PROGRAM -----

void loop() {
  unsigned long now = millis();

  // ----- PHOTORESISTOR -----

  int lightLevel = analogRead(PHOTO_PIN);

  // REVERSED DETECTION:
  // Higher reading = covered/dark = sleep
  // Lower reading = uncovered/light = wake

  if (!sleeping && lightLevel >= DARK_THRESHOLD) {
    sleeping = true;

    Serial.println("Sleeping! Color = PINK");

    updateLED();
  }
  else if (sleeping && lightLevel <= LIGHT_THRESHOLD) {
    sleeping = false;

    // Wake up with a neutral mood.
    mood = 10;
    lastPetTime = now;

    Serial.println("Awake! Mood = 10, GREEN");

    updateLED();
  }


  // ----- BUTTON -----

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = now;
    lastReading = reading;
  }

  if (now - lastDebounceTime >= DEBOUNCE_MS) {

    if (reading != stableButtonState) {
      stableButtonState = reading;

      // Count a single press, not a held button.
      if (stableButtonState == PRESSED_LEVEL) {

        if (!sleeping) {
          mood = min(mood + 1, MAX_MOOD);
          lastPetTime = now;

          Serial.print("Petted! Mood = ");
          Serial.println(mood);

          updateLED();
        }
      }
    }
  }


  // ----- MOOD DECAY -----

  // Mood countdown pauses while sleeping.
  if (!sleeping && now - lastPetTime >= SAD_INTERVAL) {

    unsigned long drops =
      (now - lastPetTime) / SAD_INTERVAL;

    mood = max(mood - (int)drops, MIN_MOOD);

    lastPetTime += drops * SAD_INTERVAL;

    Serial.print("Getting sad. Mood = ");
    Serial.println(mood);

    updateLED();
  }


  // ----- DEBUGGING -----

  if (now - lastLightUpdate >= 250UL) {
    lastLightUpdate = now;

    Serial.print("Light A0 = ");
    Serial.print(lightLevel);

    Serial.print(" | Sleeping = ");
    Serial.print(sleeping ? "YES" : "NO");

    Serial.print(" | Mood = ");
    Serial.println(mood);
  }
}