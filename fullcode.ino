#include <LiquidCrystal.h>

// LCD pin setup
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pulse Sensor configuration
double alpha = 0.9;          // Filter responsiveness (exponential smoothing factor)
double lastValue = 0;        // Previous value for filtering
unsigned long previousMillis = 0; // Time tracking for heart rate output interval
const unsigned long interval = 1000; // Interval for updating heart rate display (1 second)
int currentHeartRate = 0;    // Current heart rate
int lastHeartRate = 0;       // Previous heart rate

// Rhythm game configuration
const int irSensorPin = 7;   // Infrared sensor pin for user input
const int buzzerPin = 6;     // Buzzer pin for feedback sound
const int LCD_COLS = 16;     // Number of columns on the LCD
const int LCD_ROWS = 2;      // Number of rows on the LCD
int noteCol = -1;            // Column position of the active note (-1 means inactive)
bool noteActive = false;     // Indicates whether a note is active on the screen
bool noteProcessed = false;  // Indicates whether the score for the note has been processed
int score = 0;               // Player's score
const int TARGET_COL = 12;   // Column position where input detection occurs
int noteSpeed = 600;         // Speed of note movement (milliseconds per update)
const int SPEED_INCREMENT = 3; // Decrease in note speed as the score increases
const int MIN_SPEED = 200;   // Minimum speed for note movement
bool sensorPreviouslyTriggered = false; // Tracks the previous state of the IR sensor

unsigned long lastUpdateTime = 0;       // Last time the note position was updated
unsigned long lastNoteSpawnTime = 0;    // Last time a new note was spawned
const int NOTE_SPAWN_INTERVAL = 800;   // Interval between note spawns (milliseconds)

void setup() {
  pinMode(irSensorPin, INPUT); // Configure the IR sensor pin as input
  pinMode(A0, INPUT);          // Configure the heart rate sensor pin as input
  pinMode(buzzerPin, OUTPUT);  // Configure the buzzer pin as output
  lcd.begin(LCD_COLS, LCD_ROWS); // Initialize the LCD display
  lcd.clear();
  Serial.begin(9600); // Start serial communication for debugging
}

void loop() {
  unsigned long currentMillis = millis();

  // Heart rate calculation
  float pulse;
  int sum = 0;
  for (int i = 0; i < 10; i++) { // Take 10 samples for averaging
    sum += analogRead(A0);
    delay(1); // Sampling delay
  }
  pulse = sum / 10.0;

  // Apply exponential smoothing filter
  double beat = (alpha * lastValue) + ((1 - alpha) * pulse);
  lastValue = beat;

  // Output and store the heart rate
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    currentHeartRate = map(beat, 500, 1023, 60, 120); // Map filtered value to heart rate (adjust as needed)

    // Adjust note speed based on heart rate changes
    if (currentHeartRate > lastHeartRate + 5) { // If heart rate increases significantly
      noteSpeed += 50; // Slow down the notes
      if (noteSpeed > 1000) noteSpeed = 1000; // Cap the maximum speed
    } else if (currentHeartRate < lastHeartRate - 5) { // If heart rate decreases significantly
      noteSpeed -= 50; // Speed up the notes
      if (noteSpeed < MIN_SPEED) noteSpeed = MIN_SPEED; // Enforce minimum speed limit
    }

    lastHeartRate = currentHeartRate; // Update the last heart rate
    Serial.print("Heart Rate: ");
    Serial.println(currentHeartRate);
  }

  // Spawn new notes at regular intervals
  if (!noteActive && (currentMillis - lastNoteSpawnTime >= NOTE_SPAWN_INTERVAL)) {
    lastNoteSpawnTime = currentMillis;
    noteCol = random(0, LCD_COLS / 2); // Randomly select a starting column
    noteActive = true;                 // Activate the note
    noteProcessed = false;             // Reset score processing
  }

  // Update the position of active notes
  if (noteActive && currentMillis - lastUpdateTime >= noteSpeed) {
    lastUpdateTime = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Score: ");
    lcd.print(score);

    lcd.setCursor(TARGET_COL, 1); // Display input detection marker
    lcd.print("|");

    if (noteCol < LCD_COLS) {
      lcd.setCursor(noteCol, 1); // Display the moving note
      lcd.print("O");
      noteCol++;
    } else {
      noteCol = -1; // Deactivate note once it moves off-screen
      noteActive = false;
    }
  }

  // Detect user input via the IR sensor
  bool sensorTriggered = (digitalRead(irSensorPin) == LOW);
  if (noteActive && !noteProcessed && !sensorPreviouslyTriggered) {
    if ((noteCol == TARGET_COL || noteCol > TARGET_COL) && sensorTriggered) {
      score++; // Increment the score
      noteProcessed = true;

      // Play buzzer sound for feedback
      tone(buzzerPin, 1000, 200); // Generate a 1000Hz tone for 200ms

      // Increase note speed
      if (noteSpeed > MIN_SPEED) {
        noteSpeed -= SPEED_INCREMENT;
      }

      // Deactivate the note
      noteCol = -1;
      noteActive = false;
    }
  }

  // Update the previous state of the IR sensor
  sensorPreviouslyTriggered = sensorTriggered;
}
