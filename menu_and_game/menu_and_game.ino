// Include necessary libraries for SPI communication, Adafruit graphics, TFT display and Gyro
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>
#include "MPU6050_light.h" // Include a lightweight library for interfacing with the MPU6050 gyroscopes

// Define the pins used for the TFT display
#define TFT_CS     10  // Chip select
#define TFT_RST    8   // Reset
#define TFT_DC     9   // Data/Command

// Initialize the TFT display object with the defined pins
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/////////////////////
// GAME MENU SETUP //
/////////////////////


// Define pins for the buttons used to navigate through the menu and make selections
const int buttonPinNext = 2;   // Button to navigate down through the menu
const int buttonPinSelect = 3; // Button to select a menu item

// Forward declare the MenuItem structure so it can be used within itself for submenus
struct MenuItem;

// Define a type for menu action functions: functions that take no arguments and return void
typedef void (*MenuAction)();

// Define the MenuItem structure, which represents each item in a menu
struct MenuItem {
    const char* name;     // Name of the menu item
    MenuAction action;    // Function to call when this item is selected
    MenuItem* submenu;    // Pointer to an array of sub-menu items
    int submenuSize;      // Number of items in the submenu
};

// Function prototypes for functions used to display menus and handle menu actions
void displayMenu();
void startGame();
void noop() {} // No operation function, used as a placeholder for menu items without an action

// Function prototypes for showing submenus and going back to the main menu
void showSpeedMenu();
void showModeMenu();
void showDifficultyMenu();
void goBack();
void setBallSpeedSlow();
void setBallSpeedMedium();
void setBallSpeedFast();

// Definition of submenu items. Each submenu ends with a "BACK" item to return to the main menu
MenuItem speedMenu[] = {
    {"SLOW", setBallSpeedSlow, NULL, 0},
    {"MEDIUM", setBallSpeedMedium, NULL, 0},
    {"FAST", setBallSpeedFast, NULL, 0},
    {"BACK", goBack, NULL, 0}
};
MenuItem modeMenu[] = {
    {"PvP", noop, NULL, 0},
    {"PvC", noop, NULL, 0},
    {"CvC", noop, NULL, 0},
    {"BACK", goBack, NULL, 0}
};
MenuItem difficultyMenu[] = {
    {"EASY", noop, NULL, 0},
    {"MEDIUM", noop, NULL, 0},
    {"HARD", noop, NULL, 0},
    {"BACK", goBack, NULL, 0}
};

// Main menu definition. It includes options that lead to submenus and a "START" item
MenuItem mainMenu[] = {
    {"SPEED", showSpeedMenu, NULL, 0},
    {"MODE", showModeMenu, NULL, 0},
    {"DIFFICULTY", showDifficultyMenu, NULL, 0},
    {"START", startGame, NULL, 0}
};

// Variables to keep track of the current menu, its size, and the currently selected item
MenuItem* currentMenu = mainMenu;  // Initially set to the main menu
int currentMenuSize = 4;           // Number of items in the current menu (main menu)
int currentSelection = 0;          // Index of the currently selected menu item

////////////////
// GAME SETUP //
////////////////

MPU6050 mpu1(Wire); // Create an MPU6050 object for the first gyroscope
MPU6050 mpu2(Wire); // Create another MPU6050 object for the second gyroscope

// Variables to hold paddle positions and scores
int lastY1 = 0; // Previous position of paddle 1
int lastY2 = 0; // Previous position of paddle 2
int y1 = 0; // Current position of paddle 1
int y2 = 0; // Current position of paddle 2
int score1 = 0; // Player 1's score
int score2 = 0; // Player 2's score
int max_score = 1; // Score needed to win the game
int paddle_length = 20; // Length of the paddle

// Ball properties
int ballX = 64, ballY = 80; // Ball position
int ballSize = 4; // Ball size
int ballVelX = 1, ballVelY = 1; // Ball velocity in X and Y direction

// Game conditions
int finished = 0;   // Variable for ending a game

enum ProgramState {MENU, GAME, PLAYER1, PLAYER2};
ProgramState currentState = MENU;

void setup() {
    Serial.begin(9600);                  // Initialize serial communication for debugging
    Wire.begin(); // Initialize I2C
    // Set unique I2C addresses for the gyroscopes
    mpu1.setAddress(0x68);
    mpu2.setAddress(0x69);
    mpu1.begin(); // Initialize the first MPU6050
    mpu2.begin(); // Initialize the second MPU6050
    mpu1.calcOffsets(); // Calibrate the first MPU6050
    mpu2.calcOffsets(); // Calibrate the second MPU6050
    
    tft.initR(INITR_REDTAB);             // Initialize the TFT display with red tab configuration
    tft.setRotation(2);                  // Set display rotation
    pinMode(buttonPinNext, INPUT_PULLUP);    // Configure the next button pin
    pinMode(buttonPinSelect, INPUT_PULLUP);  // Configure the select button pin
    displayMenu();  // Display the initial main menu
}

void loop() {
  switch (currentState) {
    case MENU:
      // Handle menu
      preGameMenu();
      break;
    case GAME: 
      // Exectue game
      displayScore();
      mainGame();
      break;
    case PLAYER1:
      // Show score and winner
      someoneWon(" Player 1");
      currentState = MENU;
      displayMenu();  // Display the initial main menu
      break;
    case PLAYER2:
      someoneWon(" Player 2");
      currentState = MENU;
      displayMenu();  // Display the initial main menu
      break;
  
  }    
}

// menu before game
void preGameMenu() {
  static unsigned long lastDebounceTime = 0; // Last time the buttons were checked
  const unsigned long debounceDelay = 200;   // Debounce delay to prevent multiple detections

  // Check if enough time has passed since the last button press to debounce the buttons
  if (millis() - lastDebounceTime > debounceDelay) {
      // If the next button is pressed, move to the next menu item
      if (digitalRead(buttonPinNext) == LOW) {
          currentSelection = (currentSelection + 1) % currentMenuSize; // Cycle through the menu items
          displayMenu(); // Update the display with the new current selection
          lastDebounceTime = millis(); // Reset the debounce timer
          while(digitalRead(buttonPinNext) == LOW); // Wait for the button to be released
      } else if (digitalRead(buttonPinSelect) == LOW) {
          // If the select button is pressed, check if the current item has an action
          if (currentMenu[currentSelection].action) {
              currentMenu[currentSelection].action(); // Perform the action associated with the selected item
          }
          lastDebounceTime = millis(); // Reset the debounce timer
          while(digitalRead(buttonPinSelect) == LOW); // Wait for the button to be released
      }
  }

}

// Function to display the current menu on the TFT screen
void displayMenu() {
    tft.fillScreen(ST7735_BLACK); // Clear the screen
    tft.setTextSize(1);           // Set text size
    for (int i = 0; i < currentMenuSize; i++) { // Loop through each menu item
        tft.setCursor(10, 20 + i * 20); // Position the text cursor
        if (i == currentSelection) {
            tft.setTextColor(ST7735_RED); // Highlight the selected item
        } else {
            tft.setTextColor(ST7735_WHITE); // Use white for unselected items
        }
        tft.println(currentMenu[i].name); // Print the name of the menu item
    }
}

//////////////////////////////////////////
// Functions associated with menu items //
//////////////////////////////////////////

void showSpeedMenu() {
    currentMenu = speedMenu; // Change to the speed submenu
    currentMenuSize = sizeof(speedMenu) / sizeof(MenuItem); // Update the size for the new menu
    currentSelection = 0; // Reset selection to the top of the submenu
    displayMenu(); // Display the submenu
}

void setBallSpeedSlow(){ballVelX = 1, ballVelY = 1;goBack();}
void setBallSpeedMedium(){ballVelX = 2, ballVelY = 2;goBack();}
void setBallSpeedFast(){ballVelX = 3, ballVelY = 3;goBack();}

void showModeMenu() {
    currentMenu = modeMenu; // Similar to showSpeedMenu
    currentMenuSize = sizeof(modeMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

void showDifficultyMenu() {
    currentMenu = difficultyMenu; // Similar to showSpeedMenu
    currentMenuSize = sizeof(difficultyMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

// Function to handle the "BACK" menu item action, returning to the main menu
void goBack() {
    currentMenu = mainMenu; // Set the current menu back to the main menu
    currentMenuSize = sizeof(mainMenu) / sizeof(MenuItem); // Update the size for the main menu
    currentSelection = 0; // Reset the selection to the top of the main menu
    displayMenu(); // Display the main menu
}

// Placeholder function for the "START" action, currently just prints to serial
void startGame() {
    Serial.println("Starting game...");
    tft.fillScreen(ST7735_BLACK);
    currentState = GAME;
}

////////////////////
// GAME FUNCTIONS //
////////////////////

void mainGame() {
  // Update gyroscope readings
  mpu1.update();
  mpu2.update();

  // Map the gyroscope angles to screen positions
  y1 = mapAngleToScreen(mpu1.getAngleX());
  y2 = mapAngleToScreen(mpu2.getAngleX());

  // Draw paddles and update their positions
  drawPaddles();
  lastY1 = y1;
  lastY2 = y2;

  // Erase the previous ball and update its position
  tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_BLACK);
  updateBallPosition();
  // Draw the ball in the new position
  tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_WHITE);

  delay(10); // Short delay to control game speed
}
// Convert gyroscope angle to a paddle position on the screen
int mapAngleToScreen(float angle) {
    return map(angle, -45, 45, 0, tft.height() - paddle_length);
}

// Draw a rectangle, used for paddles
void drawRectangle(int x, int y, uint16_t color) {
    tft.fillRect(x, y, 4, paddle_length, color);
}

// Update the positions of paddles on the screen
void drawPaddles(){
  drawRectangle(0, lastY1, ST7735_BLACK); // Erase previous paddle 1
  drawRectangle(124, lastY2, ST7735_BLACK); // Erase previous paddle 2
  drawRectangle(0, y1, ST7735_WHITE); // Draw new paddle 1
  drawRectangle(124, y2, ST7735_WHITE); // Draw new paddle 2
}

// Handle ball movement and collision logic
void updateBallPosition() {
    ballX += ballVelX; // Update ball X position
    ballY += ballVelY; // Update ball Y position

    // Check for collisions with the top and bottom of the screen
    if (ballY <= 0 || ballY >= tft.height() - ballSize) {
        ballVelY = -ballVelY; // Reverse Y direction
    }

    // Check for collisions with the left paddle
    if (ballX <= 4) {
        if (ballY + ballSize >= lastY1 && ballY <= lastY1 + paddle_length) {
            ballVelX = -ballVelX; // Bounce off the paddle
        } else if (ballX < 0) {
            score2++; // Increment score for player 2
            if (score2 >= max_score) { // Check if player 2 won
              currentState = PLAYER1;
              return;
            }
            newPoint(); // Start a new point
            displayScore(); // Update the score display
        }
    }
    // Check for collisions with the right paddle
    else if (ballX >= tft.width() - 8) {
        if (ballY + ballSize >= lastY2 && ballY <= lastY2 + paddle_length) {
            ballVelX = -ballVelX; // Bounce off the paddle
        } else if (ballX > tft.width() - ballSize) {
            score1++; // Increment score for player 1
            if (score1 >= max_score) { // Check if player 1 won
              currentState = PLAYER2;
              return;
            }
            newPoint(); // Start a new point
            displayScore(); // Update the score display
        }
    }
}

// Reset the ball to the center of the screen
void resetBall() {
    ballX = tft.width() / 2;
    ballY = tft.height() / 2;
}

// Start a new point after scoring
void newPoint() {
  resetBall(); // Reset the ball position
  tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_WHITE); // Draw the ball at the new position
  int timer = millis(); // Start a timer
  while (millis() - timer < 1000) { // Wait for 1 second
    mpu1.update(); // Update MPU6050 readings
    mpu2.update();
    y1 = mapAngleToScreen(mpu1.getAngleX()); // Update paddle positions
    y2 = mapAngleToScreen(mpu2.getAngleX());
    drawPaddles(); // Draw the paddles in their new positions
    lastY1 = y1;
    lastY2 = y2;
  }
}

// Handle game logic when someone wins
void someoneWon(String player){
  showWinnerMessage(player); // Show the winning message
  score1 = 0; // Reset scores
  score2 = 0;
  delay(2000); // Wait for 2 seconds
  tft.fillScreen(ST7735_BLACK); // Clear the screen
}

// Display the winner message
void showWinnerMessage(String player) {
  tft.fillScreen(ST7735_BLACK); // Clear the screen
  tft.setTextColor(ST7735_WHITE); // Set text color
  tft.setCursor(5, tft.height() / 4); // Set cursor position
  tft.setTextSize(1.5); // Set text size
  tft.print("    The Winner is"); // Print the message
  tft.setCursor(5, tft.height() / 2);
  tft.setTextSize(2);
  tft.print(player); // Print the winner's name
}

// Display the current score
void displayScore() {
    tft.fillRect(0, 0, tft.width(), 10, ST7735_BLACK); // Clear the score display area
    tft.setTextColor(ST7735_WHITE); // Set text color
    tft.setTextSize(1); // Set text size
    tft.setCursor(5, 0); // Position the cursor
    tft.print("P1: "); // Print player 1's score label
    tft.print(score1); // Print player 1's score
    tft.setCursor(tft.width() / 2, 0); // Position the cursor for player 2's score
    tft.print("P2: "); // Print player 2's score label
    tft.print(score2); // Print player 2's score
}



