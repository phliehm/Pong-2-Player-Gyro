#include "SPI.h" // Include the SPI library for serial communication, needed for the TFT display
#include "Adafruit_GFX.h" // Include the Adafruit Graphics library for drawing graphics
#include "Adafruit_ST7735.h" // Include the library specifically for the ST7735 TFT display
#include "Wire.h" // Include the Wire library for I2C communication, used by the MPU6050 gyroscopes
#include "MPU6050_light.h" // Include a lightweight library for interfacing with the MPU6050 gyroscopes

// Define pins for the TFT display
#define TFT_PIN_CS 10 // Chip select pin
#define TFT_PIN_DC 9 // Data/command pin
#define TFT_PIN_RST 8 // Reset pin
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST); // Create a display object

MPU6050 mpu1(Wire); // Create an MPU6050 object for the first gyroscope
MPU6050 mpu2(Wire); // Create another MPU6050 object for the second gyroscope

// Variables to hold paddle positions and scores
int lastY1 = 0; // Previous position of paddle 1
int lastY2 = 0; // Previous position of paddle 2
int y1 = 0; // Current position of paddle 1
int y2 = 0; // Current position of paddle 2
int score1 = 0; // Player 1's score
int score2 = 0; // Player 2's score
int max_score = 5; // Score needed to win the game
int paddle_length = 20; // Length of the paddle

// Ball properties
int ballX = 64, ballY = 80; // Ball position
int ballSize = 4; // Ball size
int ballVelX = 1, ballVelY = 1; // Ball velocity in X and Y direction

void setup() {
    Serial.begin(9600); // Start serial communication
    Wire.begin(); // Initialize I2C

    // Initialize the TFT display
    tft.initR(INITR_REDTAB);
    tft.setRotation(2);
    tft.fillScreen(ST7735_BLACK);
    
    // Set unique I2C addresses for the gyroscopes
    mpu1.setAddress(0x68);
    mpu2.setAddress(0x69);
    mpu1.begin(); // Initialize the first MPU6050
    mpu2.begin(); // Initialize the second MPU6050
    mpu1.calcOffsets(); // Calibrate the first MPU6050
    mpu2.calcOffsets(); // Calibrate the second MPU6050

    displayScore(); // Display the initial score
}

void loop() {
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
              someoneWon(" Player 2");
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
              someoneWon(" Player 1");
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
