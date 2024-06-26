// Libraries for SPI, Adafruit graphics, TFT display, and MPU6050
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>
#include "MPU6050_light.h"
#include <math.h>  // For the round function


// Constants for TFT Display and game setup
constexpr uint8_t TFT_CS = 10, TFT_RST = 8, TFT_DC = 9;
constexpr uint8_t BUTTON_PIN_NEXT = 3, BUTTON_PIN_SELECT = 4;
constexpr int BALL_SIZE = 4, PADDLE_LENGTH = 20, PADDLE_WIDTH = 4;
constexpr int MAX_SCORE = 10;
constexpr int PADDLE_LEFT_X = 0;       // X-coordinate for the left paddle
constexpr int PADDLE_RIGHT_X = 124;    // X-coordinate for the right paddle

// Game state variables
int ballVelXSet = 1 ,ballVelYSet = 1;  // speed variable for the game state, doesnt change during one game
int ballVelX = 1, ballVelY = 1; // speed variables which can change during game
int ballX = 64, ballY = 80;

int lastY1 = 0, lastY2 = 0, y1 = 0, y2 = 0, score1 = 0, score2 = 0;
bool finished = false;
enum ProgramState { MENU, GAME, PLAYER1, PLAYER2 };
ProgramState currentState = MENU;

// Add Difficulty Settings
enum DifficultyLevel { EASY, HARD };
DifficultyLevel currentDifficulty = EASY;

// TFT Display object initialization
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// MPU6050 gyroscope objects
MPU6050 mpu1(Wire), mpu2(Wire);

// Menu structures
struct MenuItem {
    const char* name;
    void (*action)();    // Pointer to the function executed on selection
    MenuItem* submenu;   // Pointer to submenu array
    int submenuSize;     // Number of items in the submenu
};

// Enum for game mode
enum GameMode { PvP, PvC };
GameMode currentGameMode = PvP;

// NPC Variables
int npcReactionDelay = 20; // Milliseconds delay
unsigned long lastNpcMoveTime = 0;
int npcOffset = 0; // Offset to simulate intentional errors

// Function prototypes
void setupMPU6050(MPU6050& mpu, uint8_t address);
void displayMenu();
void startGame();
void showSpeedMenu();
void showModeMenu();
void showNpcDifficultyMenu();
void showDifficultyMenu();
void goBack();
void setBallSpeedSlow();
void setBallSpeedMedium();
void setBallSpeedFast();
void setEasyMode();
void setHardMode();
void setPvPMode();
void setPvCMode();
void setEasyNpc();
void setHardNpc();
void applyPaddleAcceleration();
void noop() {}
void preGameMenu();
void mainGame();
void drawPaddles();
void updateBallPosition();
void resetBall();
void newPoint();
void someoneWon(const char* player);
void showWinnerMessage(const char* player);
void displayScore();
int mapAngleToScreen(float angle);
enum CollisionType { NONE, WALL_TOP, WALL_BOTTOM, PADDLE_LEFT, PADDLE_RIGHT, OUT_OF_BOUNDS_LEFT, OUT_OF_BOUNDS_RIGHT };
CollisionType detectCollision();
void handleScoring(ProgramState winner);

// Menu item arrays
MenuItem speedMenu[] = {
    {"SLOW", setBallSpeedSlow, nullptr, 0},
    {"MEDIUM", setBallSpeedMedium, nullptr, 0},
    {"FAST", setBallSpeedFast, nullptr, 0},
    {"BACK", goBack, nullptr, 0}
};
// Mode Menu actions
MenuItem modeMenu[] = {
    {"PvP", setPvPMode, nullptr, 0},
    {"PvC", setPvCMode, nullptr, 0},
    {"BACK", goBack, nullptr, 0}
};
// NPC Difficulty Menu
MenuItem npcDifficultyMenu[] = {
    {"EASY", setEasyNpc, nullptr, 0},
    {"HARD", setHardNpc, nullptr, 0},
    {"BACK", goBack, nullptr, 0}
};
// Update the general difficulty menu to call the new functions
MenuItem difficultyMenu[] = {
    {"EASY", setEasyNpc, nullptr, 0},
    {"HARD", setHardNpc, nullptr, 0},
    {"BACK", goBack, nullptr, 0}
};

MenuItem mainMenu[] = {
    {"SPEED", showSpeedMenu, nullptr, 0},
    {"MODE", showModeMenu, nullptr, 0},
    {"DIFFICULTY", showNpcDifficultyMenu, nullptr, 0},
    {"START", startGame, nullptr, 0}
};

// Menu navigation variables
MenuItem* currentMenu = mainMenu;
int currentMenuSize = sizeof(mainMenu) / sizeof(MenuItem);
int currentSelection = 0;

// Function to initialize an MPU6050 gyroscope
void setupMPU6050(MPU6050& mpu, uint8_t address) {
    mpu.setAddress(address);
    mpu.begin();
    mpu.calcOffsets();
}

// Arduino setup function for hardware initialization
void setup() {
    Serial.begin(9600);
    Wire.begin();
    setupMPU6050(mpu1, 0x68);
    setupMPU6050(mpu2, 0x69);
    tft.initR(INITR_REDTAB);
    tft.setRotation(2);
    pinMode(BUTTON_PIN_NEXT, INPUT_PULLUP);
    pinMode(BUTTON_PIN_SELECT, INPUT_PULLUP);
    displayMenu();
}

// Arduino loop function that cycles through game states
void loop() {
    switch (currentState) {
        case MENU:
            preGameMenu();
            break;
        case GAME:
            displayScore();
            mainGame();
            break;
        case PLAYER1:
            someoneWon(" Player 1");
            currentState = MENU;
            displayMenu();
            break;
        case PLAYER2:
            someoneWon(" Player 2");
            currentState = MENU;
            displayMenu();
            break;
    }
}

// Handle pre-game menu navigation and button debouncing
void preGameMenu() {
    static unsigned long lastDebounceTime = 0;
    constexpr unsigned long debounceDelay = 200;

    if (millis() - lastDebounceTime > debounceDelay) {
        if (digitalRead(BUTTON_PIN_NEXT) == LOW) {
            currentSelection = (currentSelection + 1) % currentMenuSize;
            displayMenu();
            lastDebounceTime = millis();
            while (digitalRead(BUTTON_PIN_NEXT) == LOW);
        } else if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
            if (currentMenu[currentSelection].action) {
                currentMenu[currentSelection].action();
            }
            lastDebounceTime = millis();
            while (digitalRead(BUTTON_PIN_SELECT) == LOW);
        }
    }
}

// Display the current menu and highlight the selected item
void displayMenu() {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextSize(1);
    for (int i = 0; i < currentMenuSize; i++) {
        tft.setCursor(10, 20 + i * 20);
        if (i == currentSelection) {
            tft.setTextColor(ST7735_RED);
        } else {
            tft.setTextColor(ST7735_WHITE);
        }
        tft.println(currentMenu[i].name);
    }
}

// Display the speed submenu
void showSpeedMenu() {
    currentMenu = speedMenu;
    currentMenuSize = sizeof(speedMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

// Display the mode submenu
void showModeMenu() {
    currentMenu = modeMenu;
    currentMenuSize = sizeof(modeMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

// Display the NPC difficulty submenu
void showNpcDifficultyMenu() {
    currentMenu = npcDifficultyMenu;
    currentMenuSize = sizeof(npcDifficultyMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

// Set ball speed functions that return to the main menu
void setBallSpeedSlow() { ballVelXSet = 1; ballVelYSet = 1; goBack(); }
void setBallSpeedMedium() { ballVelXSet = 2; ballVelYSet = 2; goBack(); }
void setBallSpeedFast() { ballVelXSet = 3; ballVelYSet = 3; goBack(); }

// Set the game mode to Player vs. Player
void setPvPMode() {
    currentGameMode = PvP;
    goBack();
}

// Set the game mode to Player vs. NPC and show the difficulty selection
void setPvCMode() {
    currentGameMode = PvC;
    showNpcDifficultyMenu();
}

// Set NPC difficulty levels
void setEasyNpc() {
    npcReactionDelay = 0; // Slow reaction time
    npcOffset = 5; // Larger offset
    goBack();
}

void setHardNpc() {
    npcReactionDelay = 0; // Faster reaction time
    npcOffset = 1; // Smaller offset
    goBack();
}

// Function to update the NPC paddle
void updateNpcPaddle(int &npcPaddleY, int ballY, int paddleSpeed) {
    if (millis() - lastNpcMoveTime > npcReactionDelay) {
        int randomChange = random(-npcOffset, npcOffset);
        int targetY = ballY + randomChange ; // Add randomness

        if (npcPaddleY < targetY) {
            npcPaddleY += paddleSpeed; // Move down
        } else if (npcPaddleY > targetY) {
            npcPaddleY -= paddleSpeed; // Move up
        }

        lastNpcMoveTime = millis();
    }

    // Ensure paddle stays within screen bounds
    npcPaddleY = constrain(npcPaddleY, 0, tft.height() - PADDLE_LENGTH);
}

// Return to the main menu
void goBack() {
    currentMenu = mainMenu;
    currentMenuSize = sizeof(mainMenu) / sizeof(MenuItem);
    currentSelection = 0;
    displayMenu();
}

// Start the game and switch to game state
void startGame() {
    Serial.println("Starting game...");
    tft.fillScreen(ST7735_BLACK);
    currentState = GAME;
    ballVelY = ballVelYSet;
    ballVelX = ballVelXSet;
    newPoint();
}

// Main game loop for ball and paddle movements
void mainGame() {
  
    mpu1.update(); // Always update the first player's gyroscope
    y1 = mapAngleToScreen(mpu1.getAngleX());

    if (currentGameMode == PvP) {
        mpu2.update(); // Second player is a human
        y2 = mapAngleToScreen(mpu2.getAngleX());
    } else if (currentGameMode == PvC) {
        updateNpcPaddle(y2, ballY, abs(ballVelY)); // NPC controls the second paddle
    }

    drawPaddles();
    lastY2 = y2;
    lastY1 = y1;
    tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST7735_BLACK);
    updateBallPosition();
    tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST7735_WHITE);

    delay(10);
}

// Map gyroscope angle to screen position
int mapAngleToScreen(float angle) {
    return map(angle, -45, 45, 0, tft.height() - PADDLE_LENGTH);
}

// Draw paddles on the screen
void drawPaddles() {
    tft.fillRect(PADDLE_LEFT_X, lastY1, PADDLE_WIDTH, PADDLE_LENGTH, ST7735_BLACK);
    tft.fillRect(PADDLE_RIGHT_X, lastY2, PADDLE_WIDTH, PADDLE_LENGTH, ST7735_BLACK);
    tft.fillRect(PADDLE_LEFT_X, y1, PADDLE_WIDTH, PADDLE_LENGTH, ST7735_WHITE);
    tft.fillRect(PADDLE_RIGHT_X, y2, PADDLE_WIDTH, PADDLE_LENGTH, ST7735_WHITE);
}

// Adjust ball reflection based on paddle position
void reflectBallFromPaddle(int paddleTop, int paddleLength, int &ballVelX, int &ballVelY) {
  
    int paddleCenter = paddleTop + paddleLength / 2;
    int hitPosition = ballY - paddleCenter;
    float hitRatio = static_cast<float>(hitPosition) / (paddleLength / 2);
    ballVelX = -ballVelX;
    int newVerticalSpeed = ballVelY + static_cast<int>(round(hitRatio * 1));
    ballVelY = newVerticalSpeed;//hitRatio < 0 ? -newVerticalSpeed : newVerticalSpeed;
}

// Accelerate paddle based on MPU6050 angles
void applyPaddleAcceleration() {
    float accelFactor = 10;
    y1 += static_cast<int>(accelFactor * (mpu1.getAngleX() - lastY1));
    y2 += static_cast<int>(accelFactor * (mpu2.getAngleX() - lastY2));
    y1 = constrain(y1, 0, tft.height() - PADDLE_LENGTH);
    y2 = constrain(y2, 0, tft.height() - PADDLE_LENGTH);
}

// Detect which type of collision occurred
CollisionType detectCollision() {
    if (ballY <= 0) return WALL_TOP;
    if (ballY >= tft.height() - BALL_SIZE) return WALL_BOTTOM;
    if (ballX <= PADDLE_WIDTH) {
        if (ballY + BALL_SIZE >= lastY1 && ballY <= lastY1 + PADDLE_LENGTH) {
            return PADDLE_LEFT;
        }
        return OUT_OF_BOUNDS_LEFT;
    }
    if (ballX >= tft.width() - PADDLE_WIDTH - BALL_SIZE) {
        if (ballY + BALL_SIZE >= lastY2 && ballY <= lastY2 + PADDLE_LENGTH) {
            return PADDLE_RIGHT;
        }
        return OUT_OF_BOUNDS_RIGHT;
    }
    return NONE;
}

void updateBallPosition() {
    ballX += ballVelX;
    ballY += ballVelY;

    switch (detectCollision()) {
        case WALL_TOP:
        case WALL_BOTTOM:
            ballVelY = -ballVelY;
            break;
        case PADDLE_LEFT:
            reflectBallFromPaddle(lastY1, PADDLE_LENGTH, ballVelX, ballVelY);
            break;
        case PADDLE_RIGHT:
            reflectBallFromPaddle(lastY2, PADDLE_LENGTH, ballVelX, ballVelY);
            break;
        case OUT_OF_BOUNDS_LEFT:
            score2++;
            handleScoring(PLAYER2);
            break;
        case OUT_OF_BOUNDS_RIGHT:
            score1++;
            handleScoring(PLAYER1);
            break;
        case NONE:
            break;
    }
}

// Handle scoring and initiate a new point
void handleScoring(ProgramState winner) {
    if (score1 >= MAX_SCORE || score2 >= MAX_SCORE) {
        currentState = winner;
    } else {
        newPoint();
        displayScore();
    }
}

// Reset ball to the center of the screen
void resetBall() {
    ballX = tft.width() / 2;
    ballY = tft.height() / 2;
    ballVelX = ballVelXSet;
    ballVelY = ballVelYSet;
    y1 = ballY;
    y2 = ballY;
}

// Start a new point and reset the ball position
void newPoint() {
    resetBall();
    tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST7735_WHITE);
    int timer = millis();
    while (millis() - timer < 1000) {
        mpu1.update();
        mpu2.update();
        y1 = mapAngleToScreen(mpu1.getAngleX());
        y2 = mapAngleToScreen(mpu2.getAngleX());
        drawPaddles();
        lastY1 = y1;
        lastY2 = y2;
    }
}

// Display a winning message and reset the scores
void someoneWon(const char* player) {
    showWinnerMessage(player);
    score1 = 0;
    score2 = 0;
    delay(2000);
    tft.fillScreen(ST7735_BLACK);
}

// Show a congratulatory message to the winning player
void showWinnerMessage(const char* player) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(5, tft.height() / 4);
    tft.setTextSize(1.5);
    tft.print("    The Winner is");
    tft.setCursor(5, tft.height() / 2);
    tft.setTextSize(2);
    tft.print(player);
}

// Display the current score of both players
void displayScore() {
    tft.fillRect(0, 0, tft.width(), 10, ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 0);
    tft.print("P1: ");
    tft.print(score1);
    tft.setCursor(tft.width() / 2, 0);
    tft.print("P2: ");
    tft.print(score2);
}
