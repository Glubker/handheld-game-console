#include "MoreSettings.h"
#include <cstring>
#include "../CustomColors.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

// Static or global variables for keyboard and UI state
std::chrono::steady_clock::time_point keyDownStartTime;
bool isKeyHeld = false;
bool isInRapidMode = false;
int repeatDelay = 500;
int repeatRate = 75;
int lastMoveDirection = 0;
bool showPopup = false;
std::string popupMessage = "";
std::chrono::steady_clock::time_point popupStartTime;
int popupDuration = 2000;
int highlightedKey = 11;
bool showNetworkManager = false;
bool showKeyboard = false;
char *currentField = nullptr;
char maskedPassword[256] = "";
char ssid[256] = "";
char password[256] = "";
static bool capsLock = false;
bool keyPressed = false;
int fieldType = 0;

const char *lowercaseKeys = "1234567890@\nqwertyuiop'\nasdfghjkl-_\nzxcvbnm,.!?";
const char *uppercaseKeys = "1234567890@\nQWERTYUIOP'\nASDFGHJKL-_\nZXCVBNM,.!?";

std::chrono::steady_clock::time_point lastKeyboardCloseTime = std::chrono::steady_clock::now();

// Handle text input and show keyboard
void HandleTextInput(char *buffer, size_t bufferSize, int type, Font font) {
    if (IsKeyDown(KEY_ENTER)) {
        currentField = buffer;
        fieldType = type;
    }

    if (showKeyboard) {
        DrawOnScreenKeyboard(font);
    }

    if (!keyPressed) {
        size_t len = strlen(buffer);
        if (len < bufferSize - 1) {
            // Add character to buffer logic
        }
    }
}

// Connect to Wi-Fi using the provided SSID and password
void ConnectToWifi(const char* ssid, const char* password) {
    std::ostringstream command;
    command << "nmcli dev wifi connect '" << ssid << "' password '" << password << "'";
    int result = system(command.str().c_str());

    if (result == 0) {
        popupMessage = "Connected to Wi-Fi network: " + std::string(ssid);
    } else {
        popupMessage = "Failed to connect to Wi-Fi network: " + std::string(ssid);
    }

    showPopup = true;
    popupStartTime = std::chrono::steady_clock::now();
}

void DrawOnScreenKeyboard(Font font) {
    const char *keys = capsLock ? uppercaseKeys : lowercaseKeys;

    int keyWidth = 65; // Larger keys for better visibility
    int keyHeight = 65;

    static const int columns = 11;
    static const int regularRows = 4; // Regular key rows
    static const int specialKeys = 4; // Number of special keys
    static const int totalKeys = regularRows * columns + specialKeys; // Total keys including special keys

    float startX = (GetScreenWidth() - (columns * (keyWidth + 5.0f))) / 2;
    float startY = (GetScreenHeight() - (5 * (keyHeight + 5.0f))) / 2 + 25;

    // Draw semi-transparent background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    // Text Preview
    if (fieldType == 0) {
        DrawTextEx(font, currentField, {25.0f, 25}, 30, 2, CUSTOM_WHITE);
    } else {
        size_t len = strlen(currentField);
        memset(maskedPassword, '*', len); // Fill maskedPassword with asterisks
        maskedPassword[len] = '\0'; // Null-terminate the maskedPassword
        DrawTextEx(font, maskedPassword, {25.0f, 25}, 30, 2, CUSTOM_WHITE);
    }

    // Draw keys
    for (int i = 0, row = 0, col = 0; keys[i] != '\0'; i++) {
        if (keys[i] == '\n') {
            row++;
            col = 0;
            continue;
        }
        Rectangle keyRect = { startX + col * (keyWidth + 5.0f), startY + row * (keyHeight + 5.0f), static_cast<float>(keyWidth), static_cast<float>(keyHeight) };

        bool isHighlighted = (highlightedKey == (row * columns + col));

        DrawRoundedRectWithBorder(keyRect, 0.3f, isHighlighted ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
        DrawTextEx(font, TextFormat("%c", keys[i]), {(float)static_cast<int>(keyRect.x) + 15, (float)static_cast<int>(keyRect.y) + 10}, 30, 2, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), keyRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !keyPressed) {
            size_t len = strlen(currentField);
            if (len < sizeof(ssid) - 1) { // Assuming currentField points to ssid or password
                currentField[len] = keys[i];
                currentField[len + 1] = '\0';
            }
            keyPressed = true;
        }

        col++;
    }

    // Special keys
    float specialKeyWidth = keyWidth * 2;
    float specialKeyHeight = keyHeight;

    float spaceKeyWidth = keyWidth * 5;

    Rectangle shiftKeyRect = { startX, startY + 4 * (keyHeight + 5), specialKeyWidth + 5, specialKeyHeight };
    Rectangle backspaceKeyRect = { startX + 7 * (keyWidth + 5), startY + 4 * (keyHeight + 5), specialKeyWidth + 5, specialKeyHeight };
    Rectangle spaceKeyRect = { startX + 2 * (keyWidth + 5), startY + 4 * (keyHeight + 5), spaceKeyWidth + 20, specialKeyHeight };
    Rectangle enterKeyRect = { startX + 9 * (keyWidth + 5), startY + 4 * (keyHeight + 5), specialKeyWidth + 5, specialKeyHeight };

    bool isShiftHighlighted = (highlightedKey == totalKeys - 4);
    bool isSpaceHighlighted = (highlightedKey == totalKeys - 3);
    bool isBackspaceHighlighted = (highlightedKey == totalKeys - 2);
    bool isEnterHighlighted = (highlightedKey == totalKeys - 1);

    // Shift Key
    DrawRoundedRectWithBorder(shiftKeyRect, 0.3f, isShiftHighlighted ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND, capsLock ? CUSTOM_BLUE : CUSTOM_BORDER_COLOR, 2.0f);
    DrawTextEx(font, capsLock ? "CAPS" : "Caps", {(float)static_cast<int>(shiftKeyRect.x) + 15, (float)static_cast<int>(shiftKeyRect.y) + 10}, 20, 2, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), shiftKeyRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !keyPressed) {
        capsLock = !capsLock;
        keyPressed = true;
    }

    // Space Key
    DrawRoundedRectWithBorder(spaceKeyRect, 0.3f, isSpaceHighlighted ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    DrawTextEx(font, "Space", {(float)static_cast<int>(spaceKeyRect.x) + 15, (float)static_cast<int>(spaceKeyRect.y) + 10}, 20, 2, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), spaceKeyRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !keyPressed) {
        size_t len = strlen(currentField);
        if (len < sizeof(ssid) - 1) { // Assuming currentField points to ssid or password
            currentField[len] = ' ';
            currentField[len + 1] = '\0';
        }
        keyPressed = true;
    }

    // Backspace Key
    DrawRoundedRectWithBorder(backspaceKeyRect, 0.3f, isBackspaceHighlighted ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    DrawTextEx(font, "Delete", {(float)static_cast<int>(backspaceKeyRect.x) + 15, (float)static_cast<int>(backspaceKeyRect.y) + 10}, 20, 2, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), backspaceKeyRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !keyPressed) {
        size_t len = strlen(currentField);
        if (len > 0) {
            currentField[len - 1] = '\0';
        }
        keyPressed = true;
    }

    // Enter Key
    DrawRoundedRectWithBorder(enterKeyRect, 0.3f, isEnterHighlighted ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    DrawTextEx(font, "Submit", {(float)static_cast<int>(enterKeyRect.x) + 15, (float)static_cast<int>(enterKeyRect.y) + 10}, 20, 2, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), enterKeyRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !keyPressed) {
        showKeyboard = false;
        keyPressed = true;
        lastKeyboardCloseTime = std::chrono::steady_clock::now();
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && keyPressed) {
        keyPressed = false;
    }
}

// Draw a popup message
void DrawPopup(Font font) {
    // Draw popup box
    Rectangle popupRect = { GetScreenWidth() / 5.0f, GetScreenHeight() / 3.0f, GetScreenWidth() / 1.5f, GetScreenHeight() / 6.0f };
    DrawRoundedRectWithBorder(popupRect, 0.3f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);

    // Draw popup text
    Vector2 textSize = MeasureTextEx(font, popupMessage.c_str(), 24, 2);
    DrawTextEx(font, popupMessage.c_str(), { popupRect.x + (popupRect.width - textSize.x) / 2.0f, popupRect.y + (popupRect.height - textSize.y) / 2.0f }, 24, 2, CUSTOM_WHITE);
}


void DrawMoreSettingsUI(int &selectedIndex, Font font, Texture2D backIcon, char* username) {
    float padding = 40.0f;
    float sectionHeight = 60.0f;
    float backButtonSize = 40.0f;
    float backButtonPadding = 25.0f;

    // Back Button
    Rectangle backButtonRect = { padding, 36.0f, backButtonSize, backButtonSize };
    DrawRoundedRectWithBorder(backButtonRect, 0.5f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);

    float iconSize = 15.0f;
    float iconXPosition = padding + (backButtonSize - iconSize) / 2.0f;
    float iconYPosition = 40.0f + (backButtonSize - iconSize) / 2.0f - 4.0f;
    DrawTextureEx(backIcon, { iconXPosition, iconYPosition }, 0.0f, iconSize / backIcon.width, CUSTOM_WHITE);

    float titleXPosition = padding + backButtonSize + backButtonPadding;
    DrawTextEx(font, "More Settings", { titleXPosition, 40.0f }, 32, 2, CUSTOM_WHITE);

    // Wi-Fi Section Label
    DrawTextEx(font, "Wi-Fi Settings", { padding, 100.0f }, 24, 2, CUSTOM_WHITE);

    Rectangle ssidSectionRect = { padding, 130.0f, GetScreenWidth() - 2 * padding, sectionHeight };
    Rectangle passwordSectionRect = { padding, 130.0f + sectionHeight + 20.0f, GetScreenWidth() - 2 * padding, sectionHeight };
    Rectangle connectSectionRect = { padding, 130.0f + 2 * (sectionHeight + 20.0f), GetScreenWidth() - 2 * padding, sectionHeight };

    // Username Section Label (Moved further down)
    float usernameSectionStartY = 130.0f + 3 * (sectionHeight + 20.0f) + 40.0f;
    DrawTextEx(font, "Username Settings", { padding, usernameSectionStartY - 30.0f }, 24, 2, CUSTOM_WHITE);

    Rectangle usernameSectionRect = { padding, usernameSectionStartY, GetScreenWidth() - 2 * padding, sectionHeight };

    // Highlight selected row
    if (selectedIndex == 0) {
        DrawRoundedRectWithBorder(ssidSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
    } else {
        DrawRoundedRectWithBorder(ssidSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    }

    if (selectedIndex == 1) {
        DrawRoundedRectWithBorder(passwordSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
    } else {
        DrawRoundedRectWithBorder(passwordSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    }

    if (selectedIndex == 2) {
        DrawRoundedRectWithBorder(connectSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
    } else {
        DrawRoundedRectWithBorder(connectSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    }

    if (selectedIndex == 3) {
        DrawRoundedRectWithBorder(usernameSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
    } else {
        DrawRoundedRectWithBorder(usernameSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
    }

    // Draw Wi-Fi fields
    DrawTextEx(font, "SSID", { ssidSectionRect.x + 25.0f, ssidSectionRect.y + ssidSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
    DrawTextEx(font, ssid, { ssidSectionRect.x + 200.0f, ssidSectionRect.y + ssidSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
    DrawTextEx(font, "Password", { passwordSectionRect.x + 25.0f, passwordSectionRect.y + passwordSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
    size_t len = strlen(password);
    if (len > 0) {
        memset(maskedPassword, '*', len);
        maskedPassword[len] = '\0';
        DrawTextEx(font, maskedPassword, { passwordSectionRect.x + 200.0f, passwordSectionRect.y + passwordSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
    }
    DrawTextEx(font, "Connect", { connectSectionRect.x + 25.0f, connectSectionRect.y + connectSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);

    // Draw username field
    DrawTextEx(font, "Username", { usernameSectionRect.x + 25.0f, usernameSectionRect.y + usernameSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
    DrawTextEx(font, username, { usernameSectionRect.x + 200.0f, usernameSectionRect.y + usernameSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);

    auto now = std::chrono::steady_clock::now();
    auto elapsedSinceClose = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastKeyboardCloseTime).count();

    // Only allow interactions if more than 500 milliseconds have passed since the keyboard was closed
    if (elapsedSinceClose > 500) {
        // Handle back button press
        if (!showKeyboard) {
            if (CheckCollisionPointRec(GetMousePosition(), backButtonRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                showNetworkManager = false;
                keyPressed = true;
            }

            // Handle field input for touch
            if (CheckCollisionPointRec(GetMousePosition(), ssidSectionRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                selectedIndex = 0;
                currentField = ssid;
                fieldType = 0;
                showKeyboard = true;
            }
            if (CheckCollisionPointRec(GetMousePosition(), passwordSectionRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                selectedIndex = 1;
                currentField = password;
                fieldType = 1;
                showKeyboard = true;
            }
            if (CheckCollisionPointRec(GetMousePosition(), connectSectionRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                selectedIndex = 2;
                ConnectToWifi(ssid, password); // Connect to Wi-Fi
            }
            if (CheckCollisionPointRec(GetMousePosition(), usernameSectionRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                selectedIndex = 3;
                currentField = username;
                fieldType = 0;
                showKeyboard = true;
            }
        }
    }

    // Handle field input for keyboard
    if (selectedIndex == 0) {
        HandleTextInput(ssid, sizeof(ssid), 0, font);
    } else if (selectedIndex == 1) {
        HandleTextInput(password, sizeof(password), 1, font);
    } else if (selectedIndex == 2 && IsKeyPressed(KEY_ENTER)) {
        showKeyboard = false;
        ConnectToWifi(ssid, password);
    } else if (selectedIndex == 3) {
        HandleTextInput(username, sizeof(username), 0, font);
    }

    // Draw the popup if needed
    if (showPopup) {
        DrawPopup(font);

        // Check if popup should be hidden
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - popupStartTime).count();
        if (elapsed >= popupDuration) {
            showPopup = false;
        }
    }
}



void HandleNetworkManagerInput(int &selectedIndex, char* username) {
    static const int columns = 11;
    static const int regularRows = 4;
    static const int specialKeys = 4;
    static const int totalKeys = regularRows * columns + specialKeys;
    static const int rows = regularRows + 1;

    auto now = std::chrono::steady_clock::now();

    // If the keyboard is shown, disable background input handling
    if (showKeyboard) {
        // Handle only keyboard navigation when the keyboard is visible
        bool keyNavigated = false;

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_UP)) {
            if (!isKeyHeld) {
                keyDownStartTime = now;
                isKeyHeld = true;
                isInRapidMode = false;
                lastMoveDirection = IsKeyDown(KEY_RIGHT) ? 1 : IsKeyDown(KEY_LEFT) ? -1 : IsKeyDown(KEY_DOWN) ? columns : -columns;
                keyNavigated = true;
            } else {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - keyDownStartTime).count();
                if (!isInRapidMode && elapsed >= repeatDelay) {
                    isInRapidMode = true;
                }
                if (isInRapidMode && elapsed >= repeatRate) {
                    keyNavigated = true;
                    keyDownStartTime = now;
                }
            }

            if (keyNavigated) {
                if (lastMoveDirection == 1) {
                    highlightedKey = (highlightedKey + 1) % totalKeys;
                } else if (lastMoveDirection == -1) {
                    highlightedKey = (highlightedKey - 1 + totalKeys) % totalKeys;
                } else if (lastMoveDirection == columns) {
                    if (highlightedKey < (rows - 1) * columns) {
                        int nextRow = (highlightedKey / columns + 1) % rows;
                        highlightedKey = nextRow * columns + highlightedKey % columns;
                        if (highlightedKey >= totalKeys) highlightedKey = totalKeys - 1;
                    }
                } else if (lastMoveDirection == -columns) {
                    if (highlightedKey >= columns) {
                        int prevRow = (highlightedKey / columns - 1 + rows) % rows;
                        highlightedKey = prevRow * columns + highlightedKey % columns;
                        if (highlightedKey < 0) highlightedKey = 0;
                    }
                }
            }
        } else {
            isKeyHeld = false;
            isInRapidMode = false;
            lastMoveDirection = 0;
        }

        // Select key
        if (IsKeyPressed(KEY_ENTER)) {
            int row = highlightedKey / columns;
            int col = highlightedKey % columns;
            const char *keys = capsLock ? uppercaseKeys : lowercaseKeys;
            int keyIndex = 0;
            for (int i = 0; i < row; ++i) {
                keyIndex += columns + 1;
            }
            keyIndex += col;

            if (keyIndex < strlen(keys) && keys[keyIndex] != '\n') {
                size_t len = strlen(currentField);
                if (len < sizeof(ssid) - 1) {
                    currentField[len] = keys[keyIndex];
                    currentField[len + 1] = '\0';
                }
            } else if (highlightedKey == totalKeys - 4) {
                capsLock = !capsLock;
            } else if (highlightedKey == totalKeys - 3) {
                size_t len = strlen(currentField);
                if (len < sizeof(ssid) - 1) {
                    currentField[len] = ' ';
                    currentField[len + 1] = '\0';
                }
            } else if (highlightedKey == totalKeys - 2) {
                size_t len = strlen(currentField);
                if (len > 0) {
                    currentField[len - 1] = '\0';
                }
            } else if (highlightedKey == totalKeys - 1) {
                showKeyboard = false;
            }
        }

        return; // Exit after handling keyboard interactions
    }

    // Regular input handling when keyboard is not shown
    if (IsKeyPressed(KEY_DOWN)) {
        selectedIndex = (selectedIndex + 1) % 4;
        showKeyboard = false;
    }
    if (IsKeyPressed(KEY_UP)) {
        selectedIndex = (selectedIndex - 1 + 4) % 4;
        showKeyboard = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        showNetworkManager = false;
        showKeyboard = false;
    }
    if (IsKeyPressed(KEY_ENTER)) {
        showKeyboard = true;
        capsLock = false;
        currentField = (selectedIndex == 0) ? ssid : (selectedIndex == 1) ? password : username;
    }
}
