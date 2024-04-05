// Include necessary libraries for SPI communication, Adafruit graphics, and the TFT display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>

// Define the pins used for the TFT display
#define TFT_CS     10  // Chip select
#define TFT_RST    8   // Reset
#define TFT_DC     9   // Data/Command

// Initialize the TFT display object with the defined pins
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

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

// Definition of submenu items. Each submenu ends with a "BACK" item to return to the main menu
MenuItem speedMenu[] = {
    {"SLOW", noop, NULL, 0},
    {"MEDIUM", noop, NULL, 0},
    {"FAST", noop, NULL, 0},
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

void setup() {
    Serial.begin(9600);                  // Initialize serial communication for debugging
    tft.initR(INITR_REDTAB);             // Initialize the TFT display with red tab configuration
    tft.setRotation(2);                  // Set display rotation
    pinMode(buttonPinNext, INPUT_PULLUP);    // Configure the next button pin
    pinMode(buttonPinSelect, INPUT_PULLUP);  // Configure the select button pin
    displayMenu();  // Display the initial main menu
}

void loop() {
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

// Functions associated with menu items
void showSpeedMenu() {
    currentMenu = speedMenu; // Change to the speed submenu
    currentMenuSize = sizeof(speedMenu) / sizeof(MenuItem); // Update the size for the new menu
    currentSelection = 0; // Reset selection to the top of the submenu
    displayMenu(); // Display the submenu
}

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
}
