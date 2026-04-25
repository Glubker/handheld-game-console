#include "raylib.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include "src/gameLoader.h"
#include "src/systemStats.h"

#include "src/ui/MenuButton.h" // Include MenuButton.h
#include "src/ui/Background.h"  // Include Background.h
#include "src/ui/Row.h" // Include Row.h for DrawRoundedRectWithBorder
#include "src/CustomColors.h" // Include Button.h

#include <string>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

#include "src/json.hpp"
#include "src/ui/MoreSettings.h"

using json = nlohmann::json;

#if defined(__APPLE__)
    const std::string RESOURCE_PATH = "resources/";
#else
    const std::string RESOURCE_PATH = "/home/pi/game_console/resources/";
#endif

constexpr int screenWidth = 800;
constexpr int screenHeight = 480;
constexpr int menuWidth = 200;
constexpr int menuHeight = 50;
constexpr int gameDisplayCount = 3;

int frameCounter = 0;
bool useModernTheme = true;

enum Alignment { TOP_LEFT, TOP_CENTER, TOP_RIGHT, CENTER_LEFT, CENTER_CENTER, CENTER_RIGHT };
enum Page { MAIN_MENU, GAMES_MENU, SETTINGS_MENU, DEVELOPER_MENU, GAME_PLAY, PAUSE_MENU, SUSPENDED };
bool gameSuspended = false;  // Flag to indicate if the game is suspended

Page currentPage = MAIN_MENU;
int selectedMenu = 0;
const int totalMenus = 3;
int selectedGame = 0;
int totalGames = 3;

//int volumeLevel = 5;
int brightnessLevel = 10; // Initialize brightness level to a default value (5 as an example)

float transitionAlpha = 0.0f;
bool isTransitioning = false;
float transitionTimer = 0.0f;
const float transitionDuration = 0.5f;
Page nextPage;
bool entering = true; // Add this line to define the 'entering' variable
bool isPauseMenuTransitioning = false;
float pauseMenuTransitionAlpha = 0.0f;

int selectedThemeIndex = useModernTheme ? 0 : 1;  // 0 for Modern, 1 for Retro


char username[256] = "Glubker";

const char *menuItems[] = { "Games", "Settings", "Developer" };
// const char *gameItems[] = { "Platformer", "Pong", "Rocket Bird", "Retro Runner", "Snake", "Asteroids" };
// const char *gameCovers[] = { "PlatformerCover.png", "PongCover.png", "RocketBirdCover.png", "RetroRunnerCover.png", "SnakeCover.png", "AsteroidsCover.png" };
// const char *gameDescriptions[] = {
//     "Navigate through levels, overcome obstacles, and defeat enemies to\nreach the goal.",
//     "Classic table tennis game. Hit the ball past your opponent to score\npoints.",
//     "Fly through levels as a bird with a rocket. Avoid obstacles and\ncollect items.",
//     "Run endlessly while avoiding obstacles. How far can you go without\ncrashing?",
//     "Control a snake to eat food and grow. Avoid running into walls or\nyourself.",
//     "Classic space shooter. Destroy asteroids and UFOs while avoiding\ncollisions."
// };

Rectangle menuRectangles[] = {
    { screenWidth / 2.0f - menuWidth / 2.0f, screenHeight / 2.0f - menuHeight * 2.0f, (float)menuWidth, (float)menuHeight },
    { screenWidth / 2.0f - menuWidth / 2.0f, screenHeight / 2.0f - menuHeight / 2.0f, (float)menuWidth, (float)menuHeight },
    { screenWidth / 2.0f - menuWidth / 2.0f, screenHeight / 2.0f + menuHeight, (float)menuWidth, (float)menuHeight }
};

Rectangle backButton = { 40, screenHeight - 65, 70, 30 };

typedef void (*GameFunction)();

GameFunction currentGame = nullptr;

// Initialization section in main.cpp
Font modernFont;
Texture2D backIcon;
Texture2D audioIcon;
Texture2D brightnessIcon;
Texture2D gamesIcon;
Texture2D settingsIcon;
Texture2D developerIcon;
// Texture2D batteryIcon;
Texture2D wifiIcon;

void LoadResources() {
    modernFont = LoadFontEx((RESOURCE_PATH + "satoshi.otf").c_str(), 32, 0, 0);
    backIcon = LoadTexture((RESOURCE_PATH + "BackArrow.png").c_str());
    audioIcon = LoadTexture((RESOURCE_PATH + "AudioIcon.png").c_str());
    brightnessIcon = LoadTexture((RESOURCE_PATH + "BrightnessIcon.png").c_str());
    gamesIcon = LoadTexture((RESOURCE_PATH + "GamesIcon.png").c_str());
    settingsIcon = LoadTexture((RESOURCE_PATH + "SettingsIcon.png").c_str());
    developerIcon = LoadTexture((RESOURCE_PATH + "DeveloperIcon.png").c_str());
    // batteryIcon = LoadTexture((RESOURCE_PATH + "BatteryIcon.png").c_str());
    wifiIcon = LoadTexture((RESOURCE_PATH + "WifiIcon.png").c_str());
}


void HandlePauseMenuInput();

void PingPong(); // Forward declaration of the game function
void Platformer(); // Forward declaration of the game function
void RocketBird(); // Forward declaration of the game function
void RetroRunner(); // Forward declaration of the game function
void Snake(); // Forward declaration of the game function
void Asteroids(); // Forward declaration of the game function




/* --------------------------------------------------------------------------- */
/*                              Global Functions                               */
/* --------------------------------------------------------------------------- */

std::vector<std::string> availableGames;
std::vector<std::string> availableGameDescriptions;

void LoadGames() {
    availableGames = LoadAvailableGames();
    availableGameDescriptions = LoadAvailableGameDescriptions();
    totalGames = availableGames.size();
}


bool IsConnectedToWiFi() {
    // Command to check the Wi-Fi connection status
    int result = system("nmcli -t -f WIFI g | grep -q '^enabled$' && nmcli -t -f ACTIVE,SSID dev wifi | grep -q '^yes'");

    return (result == 0); // Return true if connected, false otherwise
}

inline int Clamp(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}


// Save settings to a JSON file
void SaveSettings() {
    json settings;
    //settings["volume"] = volumeLevel;
    settings["brightness"] = brightnessLevel;
    settings["theme"] = useModernTheme;
    settings["username"] = std::string(username); // Convert char array to string

    std::ofstream file(RESOURCE_PATH + "settings.json");
    if (file.is_open()) {
        file << settings.dump(4);  // Save with pretty print (4 spaces)
        file.close();
    }
}

// Load settings from a JSON file
void LoadSettings() {
    std::ifstream file(RESOURCE_PATH + "settings.json");
    if (file.is_open()) {
        json settings;
        file >> settings;
        file.close();

        //volumeLevel = settings.value("volume", 5);  // Default to 5 if not found
        std::string loadedUsername = settings.value("username", username);
        strncpy(username, loadedUsername.c_str(), sizeof(username) - 1); // Copy to char array
        username[sizeof(username) - 1] = '\0'; // Ensure null-termination
        brightnessLevel = settings.value("brightness", 10);  // Default to 10 if not found
        useModernTheme = settings.value("theme", true);  // Default to true if not found
        useModernTheme = settings.value("theme", false);
        selectedThemeIndex = useModernTheme ? 0 : 1;  // Update theme index
    }
}





Vector2 GetPosition(int width, int height, Alignment alignment, Vector2 offset) {
    switch (alignment) {
        case TOP_LEFT: return (Vector2){ 0.0f + offset.x, 0.0f + offset.y };
        case TOP_CENTER: return (Vector2){ screenWidth / 2.0f - width / 2.0f + offset.x, 0.0f + offset.y };
        case TOP_RIGHT: return (Vector2){ screenWidth - width + offset.x, 0.0f + offset.y };
        case CENTER_LEFT: return (Vector2){ 0.0f + offset.x, screenHeight / 2.0f - height / 2.0f + offset.y };
        case CENTER_CENTER: return (Vector2){ screenWidth / 2.0f - width / 2.0f + offset.x, screenHeight / 2.0f - height / 2.0f + offset.y };
        case CENTER_RIGHT: return (Vector2){ screenWidth - width + offset.x, screenHeight / 2.0f - height / 2.0f + offset.y };
        default: return (Vector2){ 0.0f, 0.0f };
    }
}

// Updated DrawAlignedText function to use custom font
void DrawAlignedText(const char *text, int fontSize, Alignment alignment, Color color, Vector2 offset, Font font) {
    Vector2 position;
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 2);

    switch (alignment) {
        case TOP_LEFT: position = (Vector2){ offset.x, offset.y }; break;
        case TOP_CENTER: position = (Vector2){ screenWidth / 2.0f - textSize.x / 2.0f + offset.x, offset.y }; break;
        case TOP_RIGHT: position = (Vector2){ screenWidth - textSize.x + offset.x, offset.y }; break;
        case CENTER_LEFT: position = (Vector2){ offset.x, screenHeight / 2.0f - textSize.y / 2.0f + offset.y }; break;
        case CENTER_CENTER: position = (Vector2){ screenWidth / 2.0f - textSize.x / 2.0f + offset.x, screenHeight / 2.0f - textSize.y / 2.0f + offset.y }; break;
        case CENTER_RIGHT: position = (Vector2){ screenWidth - textSize.x + offset.x, screenHeight / 2.0f - textSize.y / 2.0f + offset.y }; break;
        default: position = (Vector2){ 0.0f, 0.0f }; break;
    }

    if (useModernTheme){
        DrawTextEx(font, text, position, fontSize, 2, color);
    } else {
        DrawText(text, position.x, position.y, fontSize, color);
    }
}


void DrawScrollbar(int totalItems, int visibleItems, int currentIndex, Rectangle rect) {
    int scrollbarHeight = (rect.height / totalItems) * visibleItems;
    int scrollbarY = rect.y + ((rect.height - scrollbarHeight) * currentIndex) / (totalItems - visibleItems);

    DrawRectangle(rect.x, scrollbarY, rect.width, scrollbarHeight, WHITE);
    DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, DARKGRAY);
}





/* --------------------------------------------------------------------------- */
/*                                Main Menu                                   */
/* --------------------------------------------------------------------------- */
void DrawMainMenu() {
    if (useModernTheme) {
        float buttonWidth = 180.0f;
        float buttonHeight = 100.0f;
        float spacing = 20.0f;
        float sectionHeight = 50.0f;  // Height of the Shutdown button section
        float padding = 105.0f;
        float startX = (screenWidth - (totalMenus * buttonWidth + (totalMenus - 1) * spacing)) / 2.0f;
        float startY = screenHeight / 2.0f - buttonHeight / 2.0f;

        const std::string menuIcons[] = { "GamesIcon.png", "SettingsIcon.png", "DeveloperIcon.png" };

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        int hour = tm.tm_hour;

        std::string greeting;
        if (hour >= 0 && hour < 12) {
            greeting = "Good Morning, " + std::string(username);
        } else if (hour >= 12 && hour < 18) {
            greeting = "Good Afternoon, " + std::string(username);
        } else if (hour >= 18 && hour < 24) {
            greeting = "Good Evening, " + std::string(username);
        }

        DrawTextEx(modernFont, greeting.c_str(), {startX - 4, 125}, 32, 2, WHITE);

        for (int i = 0; i < totalMenus; i++) {
            Rectangle buttonRect = { startX + i * (buttonWidth + spacing), startY, buttonWidth, buttonHeight };
            DrawMainMenuButton(menuItems[i], buttonRect, selectedMenu == i, menuIcons[i], modernFont,
                menuIcons[i] == "GamesIcon.png" ? gamesIcon :
                menuIcons[i] == "SettingsIcon.png" ? settingsIcon : developerIcon);
        }

        // Shutdown Button
        Rectangle shutdownButtonRect = { padding, startY + buttonHeight + spacing + 20, screenWidth - 2 * padding, sectionHeight };

        // Highlight the shutdown button if it's selected
        Color shutdownButtonColor = selectedMenu == totalMenus ? CUSTOM_SELECTED_BACKGROUND_COLOR : CUSTOM_SECONDARY_BACKGROUND;
        Color shutdownTextColor = selectedMenu == totalMenus ? CUSTOM_WHITE : CUSTOM_TEXT_COLOR;

        DrawRoundedRectWithBorder(shutdownButtonRect, 0.4f, shutdownButtonColor, CUSTOM_BORDER_COLOR, 2.0f);

        if (CheckCollisionPointRec(GetMousePosition(), shutdownButtonRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            system("sudo shutdown -h now");
        }

        Vector2 textSize = MeasureTextEx(modernFont, "Shutdown", 20, 2);
        Vector2 textPosition = { shutdownButtonRect.x + shutdownButtonRect.width / 2.0f - textSize.x / 2.0f, shutdownButtonRect.y + shutdownButtonRect.height / 2.0f - textSize.y / 2.0f };
        DrawTextEx(modernFont, "Shutdown", {textPosition.x, textPosition.y}, 20, 2, shutdownTextColor);

        if (selectedMenu == totalMenus && IsKeyPressed(KEY_ENTER)) {
            system("sudo shutdown -h now");
        }

    } else {
        float sectionHeight = menuHeight; // Make the height of the shutdown button consistent with the other buttons

        for (int i = 0; i < totalMenus; i++) {
            DrawMenuButton(menuItems[i], menuRectangles[i], selectedMenu == i, useModernTheme, modernFont);
        }

        // Add a fourth rectangle for the shutdown button
        Rectangle shutdownButtonRect = { menuRectangles[0].x, menuRectangles[2].y + menuRectangles[2].height + 20, menuRectangles[2].width, sectionHeight };
        DrawMenuButton("Shutdown", shutdownButtonRect, selectedMenu == totalMenus, useModernTheme, modernFont);

        if (CheckCollisionPointRec(GetMousePosition(), shutdownButtonRect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            system("sudo shutdown -h now");
        }

        // Handle selection and execution of the shutdown command
        if (selectedMenu == totalMenus && IsKeyPressed(KEY_ENTER)) {
            system("sudo shutdown -h now");
        }
    }
}












/* --------------------------------------------------------------------------- */
/*                                Games Menu                                   */
/* --------------------------------------------------------------------------- */

void SmoothScroll(float &currentPosition, float targetPosition, float speed) {
    currentPosition += (targetPosition - currentPosition) * speed;
}

static float currentScrollPosition = 0.0f;

// Games Menu
void DrawGamesMenu() {
    float padding = 40.0f;
    float backButtonSize = 40.0f;
    float backButtonPadding = 25.0f;

    if (useModernTheme) {
        // Modern Theme
        DrawBackground(useModernTheme, frameCounter);

        Rectangle backButtonRect = { padding, 36.0f, backButtonSize, backButtonSize };
        DrawRoundedRectWithBorder(backButtonRect, 0.5f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);

        float iconSize = 15.0f;
        float iconXPosition = padding + (backButtonSize - iconSize) / 2;
        float iconYPosition = 40.0f + (backButtonSize - iconSize) / 2 - 4;
        DrawTextureEx(backIcon, { iconXPosition, iconYPosition }, 0.0f, iconSize / backIcon.width, CUSTOM_WHITE);

        float titleXPosition = padding + backButtonSize + backButtonPadding;
        DrawTextEx(modernFont, "Games", { titleXPosition, 40 }, 32, 2, CUSTOM_WHITE);

        if (availableGames.empty()) {
            Vector2 text1Size = MeasureTextEx(modernFont, "No games available", 32, 2);

            std::string text2Line1 = "Please populate the /home/pi/game_console/games folder";
            std::string text2Line2 = "with some executables to start playing.";

            Vector2 text2Line1Size = MeasureTextEx(modernFont, text2Line1.c_str(), 18, 2);
            Vector2 text2Line2Size = MeasureTextEx(modernFont, text2Line2.c_str(), 18, 2);

            float totalHeight = text1Size.y + text2Line1Size.y + text2Line2Size.y + 30;

            float startY = (screenHeight - totalHeight) / 2.0f;

            DrawTextEx(modernFont, "No games available", { screenWidth / 2.0f - text1Size.x / 2.0f, startY }, 32, 2, WHITE);
            DrawTextEx(modernFont, text2Line1.c_str(), { screenWidth / 2.0f - text2Line1Size.x / 2.0f, startY + text1Size.y + 25 }, 18, 2, GRAY);
            DrawTextEx(modernFont, text2Line2.c_str(), { screenWidth / 2.0f - text2Line2Size.x / 2.0f, startY + text1Size.y + text2Line1Size.y + 30 }, 18, 2, GRAY);

        } else {
            float cardWidth = 150.0f;
            float cardHeight = 250.0f;
            float spacing = 20.0f;
            float startX = 40.0f;
            float startY = screenHeight / 2.0f - cardHeight / 2.0f;

            /*float maxScrollPosition = (availableGames.size() - 1) * (cardWidth + spacing);
            float viewportWidth = screenWidth - startX * 2;
            if (maxScrollPosition > viewportWidth) {
                maxScrollPosition -= viewportWidth - cardWidth;
            } else {
                maxScrollPosition = 0.0f;
            }*/
            float viewportWidth = screenWidth - startX * 2;

            /*float maxScrollPosition = availableGames.size() * (cardWidth + spacing) - spacing - viewportWidth;
            if (maxScrollPosition < 0) {
                maxScrollPosition = 0.0f;
            }*/

            float maxScrollPosition = 0.0f;
            if (availableGames.size() > 4) {
                maxScrollPosition = (availableGames.size() * (cardWidth + spacing)) - viewportWidth;
            }



            float targetPosition = selectedGame * (cardWidth + spacing);
            targetPosition = Clamp(targetPosition, 0.0f, maxScrollPosition);

            SmoothScroll(currentScrollPosition, targetPosition, 0.2f);

            for (int i = 0; i < availableGames.size(); i++) {
                float cardX = startX + i * (cardWidth + spacing) - currentScrollPosition;
                if (cardX + cardWidth < startX || cardX > screenWidth - startX) {
                    continue;
                }
                Rectangle cardRect = { cardX, startY, cardWidth, cardHeight };
                DrawMenuButton(availableGames[i].c_str(), cardRect, selectedGame == i, useModernTheme, modernFont);
            }

            DrawTextEx(modernFont, availableGameDescriptions[selectedGame].c_str(), { startX, startY + 280 }, 20, 2, GRAY);
        }
    } else {
        // Retro Theme
        DrawBackground(useModernTheme, frameCounter);

        Rectangle backButtonRect = { padding, 36.0f, backButtonSize, backButtonSize };
        DrawRoundedRectWithBorder(backButtonRect, 0.5f, BLACK, WHITE, 2.0f);

        float iconSize = 15.0f;
        float iconXPosition = padding + (backButtonSize - iconSize) / 2;
        float iconYPosition = 40.0f + (backButtonSize - iconSize) / 2 - 4;
        DrawTextureEx(backIcon, { iconXPosition, iconYPosition }, 0.0f, iconSize / backIcon.width, BLACK);

        float titleXPosition = padding + backButtonSize + backButtonPadding;
        DrawText("Games", titleXPosition, 40, 32, BLACK);

        if (availableGames.empty()) {
            int text1Width = MeasureText("No games available", 32);

            std::string text2Line1 = "Please populate the /home/pi/game_console/games folder";
            std::string text2Line2 = "with some executables to start playing.";

            int text2Line1Width = MeasureText(text2Line1.c_str(), 18);
            int text2Line2Width = MeasureText(text2Line2.c_str(), 18);

            float totalHeight = 32 + 18 + 18 + 30;  // Adding 10 as a spacing between the lines

            float startY = (screenHeight - totalHeight) / 2.0f;

            DrawText("No games available", screenWidth / 2.0f - text1Width / 2.0f, startY, 32, RED);
            DrawText(text2Line1.c_str(), screenWidth / 2.0f - text2Line1Width / 2.0f, startY + 32 + 25, 18, WHITE);
            DrawText(text2Line2.c_str(), screenWidth / 2.0f - text2Line2Width / 2.0f, startY + 32 + 18 + 30, 18, WHITE);

        } else {
            float cardWidth = 150.0f;
            float cardHeight = 250.0f;
            float spacing = 20.0f;
            float startX = 40.0f;
            float startY = screenHeight / 2.0f - cardHeight / 2.0f;

            float maxScrollPosition = (availableGames.size() - 1) * (cardWidth + spacing);
            float viewportWidth = screenWidth - startX * 2;
            if (maxScrollPosition > viewportWidth) {
                maxScrollPosition -= viewportWidth - cardWidth;
            } else {
                maxScrollPosition = 0.0f;
            }

            float targetPosition = selectedGame * (cardWidth + spacing);
            targetPosition = Clamp(targetPosition, 0.0f, maxScrollPosition);

            SmoothScroll(currentScrollPosition, targetPosition, 0.2f);

            for (int i = 0; i < availableGames.size(); i++) {
                float cardX = startX + i * (cardWidth + spacing) - currentScrollPosition;
                if (cardX + cardWidth < startX || cardX > screenWidth - startX) {
                    continue;
                }
                Rectangle cardRect = { cardX, startY, cardWidth, cardHeight };
                DrawMenuButton(availableGames[i].c_str(), cardRect, selectedGame == i, useModernTheme, modernFont);
            }

            DrawText(availableGameDescriptions[selectedGame].c_str(), startX, startY + 280, 20, GRAY);
        }
    }
}










/* --------------------------------------------------------------------------- */
/*                                Settings Menu                                */
/* --------------------------------------------------------------------------- */

const char* themeOptions[] = { "Modern", "Retro" };

void DrawSegmentedControl(int &selectedIndex, Vector2 position, float width, float height) {
    int numSegments = sizeof(themeOptions) / sizeof(themeOptions[0]);
    float segmentWidth = width / numSegments;
    float borderRadius = 10.0f; // Rounded corners for better aesthetics

    static float transitionProgress = 0.0f; // Progress of the animation
    static int previousIndex = selectedIndex; // Store the previous index
    float animationSpeed = 0.15f; // Speed of sliding animation

    // Update transition progress
    if (selectedIndex != previousIndex) {
        transitionProgress += animationSpeed;
        if (transitionProgress >= 1.0f) {
            transitionProgress = 0.0f;
            previousIndex = selectedIndex;
        }
    }

    // Calculate sliding offset for the blue background
    float offsetX = (previousIndex * (1.0f - transitionProgress) + selectedIndex * transitionProgress) * segmentWidth;

    // Draw sliding blue background
    Rectangle slidingBackground = { position.x + offsetX, position.y, segmentWidth, height };
    DrawRectangleRounded(slidingBackground, 0.2f, 10, CUSTOM_BLUE);

    for (int i = 0; i < numSegments; i++) {
        Rectangle segmentRect = { position.x + i * segmentWidth, position.y, segmentWidth, height };

        Color textColor;

        if (useModernTheme == 1) {
            textColor = (selectedIndex == i) ? CUSTOM_WHITE : CUSTOM_TEXT_COLOR;
        } else {
            textColor = (selectedIndex == i) ? CUSTOM_WHITE : GRAY;
        }

        // Draw border around each segment
        #if defined(__APPLE__)
                DrawRectangleRoundedLines(segmentRect, 0.2f, 10, 2.0f, CUSTOM_BORDER_COLOR);
        #else
                DrawRectangleRoundedLines(segmentRect, 0.2f, 10, CUSTOM_BORDER_COLOR); // Default behavior
        #endif

        // Draw text centered within the segment
        Vector2 textSize = MeasureTextEx(modernFont, themeOptions[i], 20, 2);
        Vector2 textPosition = { segmentRect.x + (segmentWidth - textSize.x) / 2, segmentRect.y + (height - textSize.y) / 2 };
        DrawTextEx(modernFont, themeOptions[i], textPosition, 20, 2, textColor);

        // Handle touch toggling
        if (CheckCollisionPointRec(GetMousePosition(), segmentRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedIndex = i;
            useModernTheme = (selectedIndex == 0);
            transitionProgress = 0.0f; // Reset animation progress for smooth transition
        }
    }
}





void DrawSlider(int &value, Vector2 position, int minValue, int maxValue, bool &isSliding, Texture2D iconTexture, float &currentBarHeight) {
    int baseBarHeight = 5;
    int expandedBarHeight = 15;
    int handleRadius = 10;
    int sliderOffset = 40;
    float animationSpeed = 0.2f;  // Speed of thickness animation

    // Adjust bar height if sliding
    if (isSliding) {
        currentBarHeight += (expandedBarHeight - currentBarHeight) * animationSpeed; // Grow
    } else {
        currentBarHeight += (baseBarHeight - currentBarHeight) * animationSpeed; // Shrink
    }

    int barWidth = static_cast<int>(GetScreenWidth() - 180); // Full width with padding

    // Calculate value width for filled part of slider
    int valueWidth = ((value - minValue) * barWidth) / (maxValue - minValue);

    // Adjust vertical centering of slider based on current height
    float centerOffset = (expandedBarHeight - baseBarHeight) / 2.0f;
    float adjustedPositionY = position.y - (currentBarHeight - baseBarHeight) / 2.0f;

    // Draw icon
    DrawTextureEx(iconTexture, {position.x, position.y - 10}, 0.0f, 1.0f, GRAY);

    // Draw slider background
    DrawRectangle(position.x + sliderOffset, adjustedPositionY, barWidth, currentBarHeight, DARKGRAY);

    // Draw slider value
    DrawRectangle(position.x + sliderOffset, adjustedPositionY, valueWidth, currentBarHeight, CUSTOM_BLUE);

    // Draw handle
    Vector2 handlePosition = { position.x + sliderOffset + valueWidth - handleRadius, adjustedPositionY + currentBarHeight / 2 - handleRadius };
    DrawCircle(handlePosition.x + handleRadius, handlePosition.y + handleRadius, handleRadius, GRAY);
    DrawCircle(handlePosition.x + handleRadius, handlePosition.y + handleRadius, handleRadius - 2, CUSTOM_DARK_GRAY);

    // Handle interaction
    Vector2 mousePosition = GetMousePosition();
    Rectangle sliderRect = { position.x + sliderOffset, adjustedPositionY - 10, (float)barWidth, currentBarHeight + 20 }; // Adjust hitbox

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePosition, sliderRect)) {
        isSliding = true;
    } else {
        isSliding = false;
    }

}






// Draw Modern Settings Menu
void DrawModernSettingsMenu(int brightnessLevel, bool &isBrightnessSliding, int &selectedIndex) {
    if (!showNetworkManager) {
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

        // Title next to Back Button
        float titleXPosition = padding + backButtonSize + backButtonPadding;
        DrawTextEx(modernFont, "System Settings", { titleXPosition, 40 }, 32, 2, CUSTOM_WHITE);

        // Draw sections with padding
        // Rectangle volumeSectionRect = { padding, 120.0f, GetScreenWidth() - 2 * padding, sectionHeight };
        Rectangle brightnessSectionRect = { padding, 120.0f, GetScreenWidth() - 2 * padding, sectionHeight };
        Rectangle themeSectionRect = { padding, 120.0f + sectionHeight + 20.0f, GetScreenWidth() - 2 * padding, sectionHeight };
        Rectangle wifiSectionRect = { padding, 120.0f + 2 * (sectionHeight + 20.0f), GetScreenWidth() - 2 * padding, sectionHeight };

        // Highlight selected row
        // if (selectedIndex == 0) {
        //     DrawRoundedRectWithBorder(volumeSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
        // } else {
        //     DrawRoundedRectWithBorder(volumeSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
        // }

        if (selectedIndex == 0) {
            DrawRoundedRectWithBorder(brightnessSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
        } else {
            DrawRoundedRectWithBorder(brightnessSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
        }

        if (selectedIndex == 1) {
            DrawRoundedRectWithBorder(themeSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
        } else {
            DrawRoundedRectWithBorder(themeSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
        }

        if (selectedIndex == 2) {
            DrawRoundedRectWithBorder(wifiSectionRect, 0.4f, CUSTOM_SELECTED_BACKGROUND_COLOR, CUSTOM_BORDER_COLOR, 2.0f);
        } else {
            DrawRoundedRectWithBorder(wifiSectionRect, 0.4f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);
        }

        // Draw volume section
        // Vector2 volumeSliderPosition = { volumeSectionRect.x + 25.0f, volumeSectionRect.y + sectionHeight / 2.0f - 3 };
        // static float currentVolumeBarHeight = 5;
        // DrawSlider(volumeLevel, volumeSliderPosition, 0, 10, isVolumeSliding, audioIcon, currentVolumeBarHeight);

        // Draw brightness section
        Vector2 brightnessSliderPosition = { brightnessSectionRect.x + 25.0f, brightnessSectionRect.y + sectionHeight / 2.0f - 3 };
        static float currentBrightnessBarHeight = 5;
        DrawSlider(brightnessLevel, brightnessSliderPosition, 0, 10, isBrightnessSliding, brightnessIcon, currentBrightnessBarHeight);

        // Draw theme selection section
        DrawTextEx(modernFont, "Theme", { themeSectionRect.x + 25, themeSectionRect.y + themeSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);
        Vector2 segmentedPosition = { themeSectionRect.x + themeSectionRect.width - 250.0f - 10.0f, themeSectionRect.y + themeSectionRect.height / 2.0f - 20.0f };
        DrawSegmentedControl(selectedThemeIndex, segmentedPosition, 250, 40);

        DrawTextEx(modernFont, "More Settings", { wifiSectionRect.x + 25, wifiSectionRect.y + wifiSectionRect.height / 2.0f - 10.0f }, 20, 2, CUSTOM_WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), wifiSectionRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            showNetworkManager = true;
        }

    } else {
        DrawMoreSettingsUI(selectedIndex, modernFont, backIcon, username);
    }
}




void DrawRetroSettingsMenu(int brightnessLevel, bool &isBrightnessSliding, int &selectedIndex) {
    if (!showNetworkManager) {
        ClearBackground(BLACK);

        // Title
        float titleFontSize = 36.0f;
        DrawText("SYSTEM SETTINGS", GetScreenWidth() / 2.0f - MeasureText("SYSTEM SETTINGS", titleFontSize) / 2.0f, 30.0f, titleFontSize, WHITE);

        // Draw sections with padding
        float padding = 40.0f;
        float sectionHeight = 60.0f;
        float sectionPadding = 20.0f;
        Rectangle sectionRect = { padding, 120.0f, GetScreenWidth() - 2 * padding, sectionHeight };

        // Draw volume section
        // if (selectedIndex == 0) {
        //     DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);
        // } else {
        //     DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, BLACK);
        // }
        // DrawRectangleLines(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);

        // Vector2 volumeSliderPosition = { sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 5.0f };
        // static float currentVolumeBarHeight = 5;
        // DrawSlider(volumeLevel, volumeSliderPosition, 0, 10, isVolumeSliding, audioIcon, currentVolumeBarHeight);


        // Draw brightness section
        if (selectedIndex == 0) {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);
        } else {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, BLACK);
        }
        DrawRectangleLines(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);

        Vector2 brightnessSliderPosition = { sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 5.0f };
        static float currentBrightnessBarHeight = 5;
        DrawSlider(brightnessLevel, brightnessSliderPosition, 0, 10, isBrightnessSliding, brightnessIcon, currentBrightnessBarHeight);

        // Draw theme section
        sectionRect.y += sectionHeight + sectionPadding;
        if (selectedIndex == 1) {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);
        } else {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, BLACK);
        }
        DrawRectangleLines(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);

        Vector2 segmentedPosition = { sectionRect.x + sectionRect.width - 200.0f - 15.0f, sectionRect.y + sectionHeight / 2.0f - 20.0f };
        DrawSegmentedControl(selectedThemeIndex, segmentedPosition, 200, 40);

        if (selectedIndex == 1) {
            DrawText("Theme", sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 10.0f, 20, BLACK);
        } else {
            DrawText("Theme", sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 10.0f, 20, WHITE);
        }


        /* Network Manager Section */
        sectionRect.y += sectionHeight + sectionPadding;
        if (selectedIndex == 2) {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);
        } else {
            DrawRectangle(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, BLACK);
        }
        DrawRectangleLines(sectionRect.x, sectionRect.y, sectionRect.width, sectionRect.height, WHITE);

        if (selectedIndex == 2) {
            DrawText("More Settings", sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 10.0f, 20, BLACK);
        } else {
            DrawText("More Settings", sectionRect.x + 15.0f, sectionRect.y + sectionHeight / 2.0f - 10.0f, 20, WHITE);
        }


        if (CheckCollisionPointRec(GetMousePosition(), sectionRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            showNetworkManager = true;
        }

        // Draw the Back button
        DrawText("Back", backButton.x + 10, backButton.y + 5, 20, WHITE);
        DrawRectangleLinesEx(backButton, 2, WHITE);
    } else {
        DrawMoreSettingsUI(selectedIndex, modernFont, backIcon, username);
    }
}






void HandleSettingsMenuInput(int &selectedIndex) {
    if (!showNetworkManager) {
        if (IsKeyPressed(KEY_DOWN)) selectedIndex = (selectedIndex + 1) % 4;
        if (IsKeyPressed(KEY_UP)) selectedIndex = (selectedIndex - 1 + 4) % 4;

        // if (selectedIndex == 0) { // Volume control
        //     if (IsKeyPressed(KEY_RIGHT) && volumeLevel < 10) volumeLevel++;
        //     if (IsKeyPressed(KEY_LEFT) && volumeLevel > 0) volumeLevel--;
        if (selectedIndex == 0) { // Brightness control
            if (IsKeyPressed(KEY_RIGHT) && brightnessLevel < 10) brightnessLevel++;
            if (IsKeyPressed(KEY_LEFT) && brightnessLevel > 0) brightnessLevel--;
        } else if (selectedIndex == 1) { // Theme control
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT)) {
                selectedThemeIndex = (selectedThemeIndex == 0) ? 1 : 0;
                useModernTheme = (selectedThemeIndex == 0);
            }
        } else if (selectedIndex == 2 && IsKeyPressed(KEY_ENTER)) {
            showNetworkManager = true;
        }
    } else {
        HandleNetworkManagerInput(selectedIndex, username);
    }
}








bool isVolumeSliding = false;
bool isBrightnessSliding = false;
bool useModernThemeToggle = useModernTheme; // Temporary toggle value


void DrawSettingsMenu() {
    DrawBackground(useModernTheme, frameCounter); // Use the appropriate background

    // Load icons
    static Texture2D volumeIcon = audioIcon;
    static Texture2D brightnessIcon = audioIcon;

    if (useModernTheme) {
        // Draw Modern Settings Menu
        DrawModernSettingsMenu(brightnessLevel, isBrightnessSliding, selectedMenu);
    } else {
        // Draw Retro Settings Menu
        DrawRetroSettingsMenu(brightnessLevel, isBrightnessSliding, selectedMenu);
    }
}




/* ----------------------------------------------------------------------------- */
/*                                Developer Menu                                */
/* ----------------------------------------------------------------------------- */

SystemStats stats;

void DrawDeveloperMenu() {

    if (useModernTheme) {

        DrawBackground(useModernTheme, frameCounter); // Use the appropriate background

        float padding = 40.0f;
        float backButtonSize = 40.0f;
        float backButtonPadding = 25.0f;

        // Back Button
        Rectangle backButtonRect = { padding, 36.0f, backButtonSize, backButtonSize };
        DrawRoundedRectWithBorder(backButtonRect, 0.5f, CUSTOM_SECONDARY_BACKGROUND, CUSTOM_BORDER_COLOR, 2.0f);

        // Calculate icon centered position
        float iconSize = 15.0f;
        float iconXPosition = padding + (backButtonSize - iconSize) / 2;
        float iconYPosition = 40.0f + (backButtonSize - iconSize) / 2 - 4;
        DrawTextureEx(backIcon, { iconXPosition, iconYPosition }, 0.0f, iconSize / backIcon.width, CUSTOM_WHITE);

        // Title next to Back Button
        float titleXPosition = padding + backButtonSize + backButtonPadding;
        DrawTextEx(modernFont, "Developer", { titleXPosition, 40 }, 32, 2, CUSTOM_WHITE);

        // Display system information
        float posY = 80;
        int fontSize = 20;
        posY += 60;

        DrawTextEx(modernFont, TextFormat("FPS: %i", GetFPS()), {padding, posY}, fontSize, 2, WHITE);
        posY += 30;
        DrawTextEx(modernFont, TextFormat("CPU Usage: %.2f%%", stats.GetCPUUsage()), {padding, posY}, fontSize, 2, WHITE);
        posY += 30;
        DrawTextEx(modernFont, TextFormat("Memory Usage: %.2f MB", stats.GetMemoryUsageMB()), {padding, posY}, fontSize, 2, WHITE);
        posY += 30;
        DrawTextEx(modernFont, TextFormat("Storage Usage: %.2f GB", stats.GetStorageUsageGB()), {padding, posY}, fontSize, 2, WHITE);
        posY += 30;
        DrawTextEx(modernFont, TextFormat("CPU Temperature: %.2f\u00B0C", stats.GetCPUTemperature()), {padding, posY}, fontSize, 2, WHITE);
        posY += 40;

        DrawLine(padding, posY, screenWidth - padding, posY, GRAY);

        posY += 25;

        DrawTextEx(modernFont, "Game Console v1.0.0", {padding, posY}, fontSize, 2, RAYWHITE);

        posY += 30;

        DrawTextEx(modernFont, "Developed by Gustav Lubker", {padding, posY}, fontSize, 2, RAYWHITE);

    } else {

        float padding = 40.0f;

        // Display system information
        int posY = 100;
        int fontSize = 20;
        DrawText("Developer", screenWidth / 2 - MeasureText("Developer", 40) / 2, posY, 40, WHITE);
        posY += 60;

        DrawText(TextFormat("FPS: %i", GetFPS()), padding, posY, fontSize, WHITE);
        posY += 30;
        DrawText(TextFormat("CPU Usage: %.2f%%", stats.GetCPUUsage()), padding, posY, fontSize, WHITE);
        posY += 30;
        DrawText(TextFormat("Memory Usage: %.2f MB", stats.GetMemoryUsageMB()), padding, posY, fontSize, WHITE);
        posY += 30;
        DrawText(TextFormat("Storage Usage: %.2f GB", stats.GetStorageUsageGB()), padding, posY, fontSize, WHITE);
        posY += 30;
        DrawText(TextFormat("CPU Temperature: %.2f°C", stats.GetCPUTemperature()), padding, posY, fontSize, WHITE);
        posY += 40;

        DrawLine(padding, posY, screenWidth - padding, posY, RAYWHITE);

        posY += 25;

        DrawText("Game Console v1.0.0", padding, posY, fontSize, RAYWHITE);

        posY += 30;

        DrawText("Developed by Gustav Lübker", padding, posY, fontSize, RAYWHITE);

        // Draw the Back button
        DrawText("Back", backButton.x + 10, backButton.y + 5, 20, WHITE);
        DrawRectangleLinesEx(backButton, 2, WHITE);
    }
}




/* --------------------------------------------------------------------- */
/*                                Header                                 */
/* --------------------------------------------------------------------- */

void DrawHeader() {
    float padding = 40.0f;
    float verticalPadding = 15.0f;
    float iconSize = 28.0f;

    // Update time and date
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream ossTime;
    std::ostringstream ossDate;
    ossTime << std::put_time(&tm, "%H:%M");
    ossDate << std::put_time(&tm, "%a %d %b");

    auto strTime = ossTime.str();
    auto strDate = ossDate.str();

    std::transform(strDate.begin(), strDate.end(), strDate.begin(), ::toupper);

    // Draw background or divider for the header
    DrawRectangle(0, 0, screenWidth, 60, Fade(CUSTOM_SECONDARY_BACKGROUND, 0.8f));
    DrawLine(0, 60, screenWidth, 60, Fade(WHITE, 0.2f));

    // Draw time and date
    DrawAlignedText(strTime.c_str(), 28, TOP_LEFT, WHITE, (Vector2){padding, verticalPadding}, modernFont);
    DrawAlignedText(strDate.c_str(), 28, TOP_CENTER, WHITE, (Vector2){0.0f, verticalPadding}, modernFont);

    if (IsConnectedToWiFi()) {
        Vector2 wifiIconPos = {screenWidth - padding - iconSize - verticalPadding, verticalPadding};
        DrawTextureEx(wifiIcon, wifiIconPos, 0.0f, iconSize / wifiIcon.width, WHITE);
    }

    // Draw battery icon with labels
    // std::ostringstream batteryStr;
    // batteryStr << batteryLevel << "%";
    // Vector2 batteryTextSize = MeasureTextEx(modernFont, batteryStr.str().c_str(), 28, 2);

    // Vector2 batteryIconPos = {screenWidth - padding - batteryTextSize.x - iconSize - verticalPadding, verticalPadding};
    // DrawTextureEx(batteryIcon, batteryIconPos, 0.0f, iconSize / batteryIcon.width, WHITE);
    // DrawAlignedText(batteryStr.str().c_str(), 28, TOP_RIGHT, WHITE, (Vector2){-padding, verticalPadding}, modernFont);

    // Clean up textures if loaded each frame (you may load these textures once globally instead)
    /*UnloadTexture(batteryIcon);
    UnloadTexture(volumeIcon);*/
}



void DrawFadeTransition() {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, transitionAlpha));
}





/* ---------------------------------------------------------------------–––––––––––– */
/*                                Handle Touch Input                                 */
/* ---------------------------------------------------------------------–––––––––––– */

void HandleTouchInput() {
    Vector2 touchPosition = GetMousePosition();

    if (currentPage == SETTINGS_MENU && !showNetworkManager) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Handle touch on the volume slider
            // Rectangle volumeSectionRect = { 40.0f, 120.0f, GetScreenWidth() - 80.0f, 60.0f };
            int sliderWidth = static_cast<int>(GetScreenWidth() - 180);
            int sliderOffset = 40;

            // if (CheckCollisionPointRec(touchPosition, volumeSectionRect)) {
            //     int newVolume = ((touchPosition.x - volumeSectionRect.x - sliderOffset) * 10) / sliderWidth;
            //     volumeLevel = Clamp(newVolume, 0, 10);
            // }

            // Handle touch on the brightness slider
            Rectangle brightnessSectionRect = { 40.0f, 120.0f, GetScreenWidth() - 80.0f, 60.0f };

            if (CheckCollisionPointRec(touchPosition, brightnessSectionRect)) {
                int newBrightness = ((touchPosition.x - brightnessSectionRect.x - sliderOffset) * 10) / sliderWidth;
                brightnessLevel = Clamp(newBrightness, 0, 10);
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

        // Handle Back Logic
        // --------------------------------------------
        if (currentPage != GAME_PLAY && currentPage != MAIN_MENU && !showNetworkManager) {

            if (useModernTheme) {

                // Modern Button
                Rectangle backButtonRect = { 40.0f, 36.0f, 40.0f, 40.0f };

                // Exit Settings and save to JSOn
                if (CheckCollisionPointRec(touchPosition, backButtonRect)) {
                    nextPage = MAIN_MENU;
                    isTransitioning = true;
                    entering = false;

                    if (currentPage == SETTINGS_MENU) {
                        SaveSettings();
                    }
                }

            } else {

                // Retro Button
                float backButtonHeight = 40.0f;
                float backButtonWidth = 120.0f;
                float padding = 40.0f;
                Rectangle retroBackButtonRect = { padding, GetScreenHeight() - padding - backButtonHeight, backButtonWidth, backButtonHeight };

                if (CheckCollisionPointRec(touchPosition, retroBackButtonRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    nextPage = MAIN_MENU;
                    isTransitioning = true;
                    entering = false;

                    if (currentPage == SETTINGS_MENU) {
                        SaveSettings();
                    }
                }

            }
        }
        // --------------------------------------------

        if (currentPage == MAIN_MENU) {
            if (useModernTheme) {

                // Horizontal button layout
                float buttonWidth = 180.0f;
                float buttonHeight = 100.0f;
                float spacing = 20.0f;
                float startX = (screenWidth - (totalMenus * buttonWidth + (totalMenus - 1) * spacing)) / 2.0f;
                float startY = screenHeight / 2.0f - buttonHeight / 2.0f;

                for (int i = 0; i < totalMenus; i++) {
                    Rectangle buttonRect = { startX + i * (buttonWidth + spacing), startY, buttonWidth, buttonHeight };
                    if (CheckCollisionPointRec(touchPosition, buttonRect)) {
                        selectedMenu = i;
                        nextPage = (selectedMenu == 0) ? GAMES_MENU : (selectedMenu == 1) ? SETTINGS_MENU : DEVELOPER_MENU;
                        isTransitioning = true;
                        entering = true;
                        break;
                    }
                }
            } else {
                // Retro theme handling

                // Vertical button layout
                for (int i = 0; i < totalMenus; i++) {
                    if (CheckCollisionPointRec(touchPosition, menuRectangles[i])) {
                        selectedMenu = i;
                        nextPage = (selectedMenu == 0) ? GAMES_MENU : (selectedMenu == 1) ? SETTINGS_MENU : DEVELOPER_MENU;
                        isTransitioning = true;
                        entering = true;
                        break;
                    }
                }
            }
        } else if (currentPage == GAMES_MENU) {
            if (currentPage == GAMES_MENU) {
                                if (useModernTheme) {
                    // Modern Theme
                    float cardWidth = 150.0f;
                    float cardHeight = 250.0f;
                    float spacing = 20.0f;
                    float startX = 40.0f;
                    float startY = screenHeight / 2.0f - cardHeight / 2.0f;

                    for (int i = 0; i < availableGames.size(); i++) {
                        float cardX = startX + i * (cardWidth + spacing) - currentScrollPosition;
                        Rectangle cardRect = { cardX, startY, cardWidth, cardHeight };
                        if (CheckCollisionPointRec(touchPosition, cardRect)) {
                            selectedGame = i;
                            std::string gamePath = "/home/pi/game_console/games/" + availableGames[selectedGame];
                            pid_t pid = fork();
                            if (pid == 0) {
                                execl(gamePath.c_str(), availableGames[selectedGame].c_str(), (char *)NULL);
                                perror("execl"); // If execl fails
                                exit(1);  // Exit child process if exec fails
                            } else if (pid < 0) {
                                perror("fork"); // If fork fails
                            }
                        }
                    }
                } else {
                    // Retro Theme
                    int startIndex = selectedGame - selectedGame % gameDisplayCount;
                    for (int i = 0; i < gameDisplayCount; i++) {
                        int gameIndex = startIndex + i;
                        if (gameIndex >= availableGames.size()) break;
                        Rectangle gameRect = { screenWidth / 2.0f - menuWidth / 2.0f, screenHeight / 2.0f - menuHeight * 2.0f + i * (menuHeight + 10.0f), (float)menuWidth, (float)menuHeight };
                        if (CheckCollisionPointRec(touchPosition, gameRect)) {
                            selectedGame = gameIndex;
                            std::string gamePath = "/home/pi/game_console/games/" + availableGames[selectedGame];
                            pid_t pid = fork();
                            if (pid == 0) {
                                execl(gamePath.c_str(), availableGames[selectedGame].c_str(), (char *)NULL);
                                perror("execl"); // If execl fails
                                exit(1);  // Exit child process if exec fails
                            } else if (pid < 0) {
                                perror("fork"); // If fork fails
                            }
                        }
                    }
                }

                // Handle scrolling via touch input
                if (CheckCollisionPointRec(touchPosition, (Rectangle){
                    (float)(screenWidth / 2.0f + menuWidth / 2.0f + 10),
                    (float)(screenHeight / 2.0f - menuHeight * 2.0f),
                    10.0f,
                    (float)(menuHeight * gameDisplayCount + 20)
                })) {
                    float scrollAmount = (touchPosition.y - (screenHeight / 2.0f - menuHeight * 2.0f)) / (menuHeight * gameDisplayCount + 20);
                    selectedGame = (int)(scrollAmount * totalGames);
                    if (selectedGame < 0) selectedGame = 0;
                    if (selectedGame >= totalGames) selectedGame = totalGames - 1;
                }
            }
        }
    }
}





/* ----------------------------------------------------------------------- */
/*                                Main Loop                                */
/* ----------------------------------------------------------------------- */

int main(void) {
    // Initialization
    InitWindow(screenWidth, screenHeight, "Game Console");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    LoadResources();  // Load all necessary resources

    LoadResources();
    LoadGames();

    LoadSettings();

    SetExitKey(KEY_NULL);  // Disable default exit key
    HideCursor();

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        HandleTouchInput();

        // Handle input and transitions
        if (isTransitioning) {
            transitionTimer += GetFrameTime();
            if (transitionTimer < transitionDuration / 2) {
                transitionAlpha = transitionTimer / (transitionDuration / 2);
            } else if (transitionTimer < transitionDuration) {
                transitionAlpha = 1.0f - (transitionTimer - transitionDuration / 2) / (transitionDuration / 2);
            } else {
                transitionTimer = 0.0f;
                isTransitioning = false;
                currentPage = nextPage;
                transitionAlpha = 0.0f;
            }
        } else {
            if (currentPage == MAIN_MENU) {
                if (useModernTheme) {
                    if (IsKeyPressed(KEY_RIGHT)) selectedMenu = (selectedMenu + 1) % (totalMenus + 1);
                    if (IsKeyPressed(KEY_LEFT)) selectedMenu = (selectedMenu - 1 + totalMenus + 1) % (totalMenus + 1);
                    if (IsKeyPressed(KEY_DOWN) && selectedMenu < totalMenus) selectedMenu = totalMenus;
                    if (IsKeyPressed(KEY_UP) && selectedMenu == totalMenus) selectedMenu = 0;
                } else {
                    if (IsKeyPressed(KEY_DOWN)) selectedMenu = (selectedMenu + 1) % (totalMenus + 1);
                    if (IsKeyPressed(KEY_UP)) selectedMenu = (selectedMenu - 1 + totalMenus + 1) % (totalMenus + 1);
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    if (selectedMenu == totalMenus) {
                        system("sudo shutdown -h now");
                    } else {
                        nextPage = (selectedMenu == 0) ? GAMES_MENU : (selectedMenu == 1) ? SETTINGS_MENU : DEVELOPER_MENU;
                        isTransitioning = true;
                        entering = true; // Start entering transition
                    }
                }
            } else if (currentPage == GAMES_MENU) {
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                    nextPage = MAIN_MENU;
                    isTransitioning = true;
                    entering = false; // Start exiting transition
                }
                if (useModernTheme) {
                    if (IsKeyPressed(KEY_RIGHT)) selectedGame = (selectedGame + 1) % totalGames;
                    if (IsKeyPressed(KEY_LEFT)) selectedGame = (selectedGame - 1 + totalGames) % totalGames;
                } else {
                    if (IsKeyPressed(KEY_DOWN)) selectedGame = (selectedGame + 1) % totalGames;
                    if (IsKeyPressed(KEY_UP)) selectedGame = (selectedGame - 1 + totalGames) % totalGames;
                }

                if (!availableGames.empty()) {
                    if (IsKeyPressed(KEY_ENTER)) {
                        std::string gamePath = "/home/pi/game_console/games/" + availableGames[selectedGame];
                        pid_t pid = fork();
                        if (pid == 0) {
                            execl(gamePath.c_str(), availableGames[selectedGame].c_str(), (char *)NULL);
                            perror("execl"); // If execl fails
                            exit(1);  // Exit child process if exec fails
                        } else if (pid < 0) {
                            perror("fork"); // If fork fails
                        }
                        nextPage = GAME_PLAY;
                        isTransitioning = true;
                        entering = true; // Start entering transition
                    }
                }
            } else if (currentPage == SETTINGS_MENU) {
                // Exit Settings and save to JSON
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                    if (showKeyboard) {
                        showKeyboard = false;
                    } else if (showNetworkManager) {
                        showNetworkManager = false;
                    } else {
                        nextPage = MAIN_MENU;
                        isTransitioning = true;
                        entering = false; // Start exiting transition

                        SaveSettings();
                    }
                } else {
                    HandleSettingsMenuInput(selectedMenu); // Call input handler for settings menu with brightnessLevel
                }
            } else if (currentPage == DEVELOPER_MENU) {
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                    nextPage = MAIN_MENU;
                    isTransitioning = true;
                    entering = false; // Start exiting transition
                }
            }
        }

        BeginDrawing();

        ClearBackground(BLACK);

        if (isTransitioning) {
            if (transitionTimer < transitionDuration / 2) {
                // Draw the current page during the first half of the transition
                if (currentPage != GAME_PLAY) {
                    DrawBackground(useModernTheme, frameCounter);
                    DrawHeader();

                    if (currentPage == MAIN_MENU) {
                        DrawMainMenu();
                    } else if (currentPage == GAMES_MENU) {
                        DrawGamesMenu();
                    } else if (currentPage == SETTINGS_MENU) {
                        DrawSettingsMenu();
                    } else if (currentPage == DEVELOPER_MENU) {
                        DrawDeveloperMenu();
                    }
                }
            } else {
                // Draw the next page during the second half of the transition
                if (nextPage != GAME_PLAY) {
                    DrawBackground(useModernTheme, frameCounter);
                    DrawHeader();

                    if (nextPage == MAIN_MENU) {
                        DrawMainMenu();
                    } else if (nextPage == GAMES_MENU) {
                        DrawGamesMenu();
                    } else if (nextPage == SETTINGS_MENU) {
                        DrawSettingsMenu();
                    } else if (nextPage == DEVELOPER_MENU) {
                        DrawDeveloperMenu();
                    }
                }
            }

            DrawFadeTransition();
        } else {
            if (currentPage != GAME_PLAY) {
                DrawBackground(useModernTheme, frameCounter);

                if (useModernTheme == false && currentPage != SETTINGS_MENU) {
                    DrawHeader();
                }

                if (currentPage == MAIN_MENU) {
                    DrawHeader();

                    DrawMainMenu();
                } else if (currentPage == GAMES_MENU) {
                    DrawGamesMenu();
                } else if (currentPage == SETTINGS_MENU) {
                    DrawSettingsMenu();
                } else if (currentPage == DEVELOPER_MENU) {
                    DrawDeveloperMenu();
                }
            }
        }

        // Brightness Overlay
        // ----------------------------------------------------------------
        // Calculate the overlay opacity based on the brightness level
        float minOpacity = 0; // Minimum opacity (screen won't go below this brightness)
        float maxOpacity = 0.8f; // Maximum opacity (dimmest screen)

        // Scale brightnessLevel (0-10) to opacity (maxOpacity to minOpacity)
        float opacity = maxOpacity - (brightnessLevel / 10.0f) * (maxOpacity - minOpacity);


        // Draw a semi-transparent black rectangle over the screen
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, opacity));
        // ----------------------------------------------------------------

        EndDrawing();
        frameCounter++;
    }

    UnloadFont(modernFont);
    UnloadTexture(backIcon);
    UnloadTexture(audioIcon);
    UnloadTexture(brightnessIcon);
    UnloadTexture(gamesIcon);
    UnloadTexture(settingsIcon);
    UnloadTexture(developerIcon);
    //UnloadTexture(batteryIcon);
    UnloadTexture(wifiIcon);

    CloseWindow();
    return 0;
}
