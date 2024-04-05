#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>

// Pin configuration for the TFT display
#define TFT_CS     10
#define TFT_RST    8 
#define TFT_DC     9

// Create TFT object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Button pin assignments
const int buttonPinDown = 3;  // Button to move down in the menu
const int buttonPinSelect = 4;// Button to select the menu option

enum MenuOption { SPEED, MODE, DIFFICULTY, START };
MenuOption currentOption = SPEED;

void setup() {
  Serial.begin(9600);
  tft.initR(INITR_REDTAB);   // Initialize the display
  tft.setRotation(2);          // Set the rotation
  tft.fillScreen(ST7735_BLACK);// Clear the screen

  // Initialize button pins
  pinMode(buttonPinDown, INPUT_PULLUP);
  pinMode(buttonPinSelect, INPUT_PULLUP);

  displayMenu(); // Display the initial menu
}

void loop() {
  
  mainMenu();
  
}

void mainMenu(){
  static unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 200;
  if (millis() - lastDebounceTime > debounceDelay) {
    
    if (digitalRead(buttonPinDown) == LOW) {
      Serial.println("DOWN");
      lastDebounceTime = millis();
      currentOption = static_cast<MenuOption>((currentOption + 1) % 4); // Move down
      displayMenu();
    }
    else if (digitalRead(buttonPinSelect) == LOW) {
      Serial.println("ENTER");
      lastDebounceTime = millis();
      // Here, implement what happens when an option is selected.
      // For now, it just prints to the Serial Monitor.
      Serial.print("Option selected: ");
      switch (currentOption) {
        case SPEED: Serial.println("SPEED"); break;
        case MODE: Serial.println("MODE"); break;
        case DIFFICULTY: Serial.println("DIFFICULTY"); break;
        case START: Serial.println("START"); break;
      }
    }
  }
}

void displayMenu() {
  tft.fillScreen(ST7735_BLACK); // Clear the screen
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  
  int startX = 5;
  int startY = 20; 
  tft.setCursor(startX, startY);
  // Display all menu options
  for (int i = SPEED; i <= START; i++) {
    if (i == currentOption) {
      tft.setTextColor(ST7735_RED, ST7735_BLACK); // Highlight the selected option
    } else {
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    }
    
    switch (i) {
      case SPEED: tft.println("SPEED"); break;
      case MODE: tft.println("MODE"); break;
      case DIFFICULTY: tft.println("DIFFICULTY"); break;
      case START: tft.println("START"); break;
    }
    // Adjust the cursor for the next item. Note: adjust the '20' below if more vertical space is needed
    tft.setCursor(startX, startY + (i + 1) * 20); // This moves the cursor down for each menu item
  }
}

