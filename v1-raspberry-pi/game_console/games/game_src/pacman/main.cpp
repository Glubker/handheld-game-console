#include <chrono>

#include "raylib.h"
#include "rlgl.h"
#include <queue>
#include <unordered_map>
#include <vector>
#include <cmath>

#include <string>
#include <algorithm> // Include this for std::remove_if
#include <fstream> // Include for file operations


const int screenWidth = 1280;
const int screenHeight = 800;

// Map dimensions and scaling
const int mapRows = 15;
const int mapCols = 25;
const float tileSize = (float)screenWidth / mapCols; // Dynamic tile size based on screen width

const int maxSpecialOrbs = 4; // Max number of special orbs allowed on the map
int specialOrbCooldown = 5000; // Time in milliseconds between potential spawns
int lastSpecialOrbSpawn = 0;   // Track last spawn time
std::vector<std::pair<int, int>> specialOrbs; // Track special orb positions

// Game state variables
bool isInitialized = false;
bool gameOver = false;
bool hasWon = false;
bool showMenu = false;
bool showEnemies = true;

// Menu state
enum MenuState { NONE, PAUSE, GAME_OVER, WIN };
MenuState menuState = NONE;
int selectedMenuItem = 0;

// Level system
int currentLevel = 1;
const int maxLevels = 5;

// Screen shake variables
float screenShake = 0.0f;
Vector2 screenOffset = { 0, 0 };

// Global variables
float elapsedTime = 0.0f;
auto startTime = std::chrono::steady_clock::now();  // Move to global scope

bool canCyanEnemyLeave = false;
bool canOrangeEnemyLeave = false;

bool forceNormalRedGhost = false;
bool forceNormalPinkGhost = false;
bool forceNormalCyanGhost = false;
bool forceNormalOrangeGhost = false;

bool canEatGhosts = false;

// Define the map array (0 = empty, X = wall), matching screen resolution
char map[mapRows][mapCols + 1] = { // +1 to accommodate null terminator for each row
    "XXXXXXXXXXXXXXXXXXXXXXXXX",
    "X11111111111111111111111X",
    "X1XXX1XXX1XXXXX1XXX1XXX1X",
    "X1X11111X111X111X11111X1X",
    "X1X1XXXXXXX1X1XXXXXXX1X1X",
    "X1111111X1111111X1111111X",
    "XXXX1XX1X1XXxXX1X1XX1XXXX",
    "00001X1111X000X1111X10000",
    "XXXX1X1XX1XXXXX1XX1X1XXXX",
    "X1111111X1111111X1111111X",
    "X1X1XXX1X1XXXXX1X1XXX1X1X",
    "X1X111X11111X11111X111X1X",
    "X1XXX1XXXXX1X1XXXXX1XXX1X",
    "X11111111111111111111111X",
    "XXXXXXXXXXXXXXXXXXXXXXXXX"
};

char initialMap[mapRows][mapCols + 1] = { // +1 to accommodate null terminator for each row
    "XXXXXXXXXXXXXXXXXXXXXXXXX",
    "X11111111111111111111111X",
    "X1XXX1XXX1XXXXX1XXX1XXX1X",
    "X1X11111X111X111X11111X1X",
    "X1X1XXXXXXX1X1XXXXXXX1X1X",
    "X1111111X1111111X1111111X",
    "XXXX1XX1X1XXxXX1X1XX1XXXX",
    "00001X1111X000X1111X10000",
    "XXXX1X1XX1XXXXX1XX1X1XXXX",
    "X1111111X1111111X1111111X",
    "X1X1XXX1X1XXXXX1X1XXX1X1X",
    "X1X111X11111X11111X111X1X",
    "X1XXX1XXXXX1X1XXXXX1XXX1X",
    "X11111111111111111111111X",
    "XXXXXXXXXXXXXXXXXXXXXXXXX"
};


Color lightYellow = { 250, 185, 176, 255 }; // Slightly lighter than Raylib's YELLOW


Color backgroundColor = {10, 14, 39, 255};
Color wallColor = {44, 47, 86, 255};
Color playerColor = {255, 235, 59, 255};
Color enemyRedColor = {255, 23, 68, 255};    // Red ghost (Blinky)
Color enemyPinkColor = {255, 182, 193, 255}; // Pink ghost (Pinky)
Color enemyCyanColor = {0, 255, 255, 255};   // Cyan ghost (Inky)
Color enemyOrangeColor = {255, 165, 0, 255}; // Orange ghost (Clyde)
Color orbColor = {249, 217, 118, 255};
Color uiTextColor = {178, 235, 242, 255};

enum Direction { LEFT, UP, RIGHT, DOWN };

bool AreAllOrbsCollected() {
    for (int row = 0; row < mapRows; row++) {
        for (int col = 0; col < mapCols; col++) {
            if (map[row][col] == '1') {
                return false;  // Orb found, not all orbs collected
            }
        }
    }
    return true;  // No orbs found, all are collected
}

float GetLevelSpeedMultiplier() {
    return 1.0f + (currentLevel - 1) * 0.3f; // 30% speed increase per level
}

int GetLevelScoreRequired() {
    return 300 + (currentLevel - 1) * 200; // Increasing score requirements for enemy release
}

void ResetMap() {
    for (int row = 0; row < mapRows; row++) {
        for (int col = 0; col < mapCols; col++) {
            map[row][col] = initialMap[row][col];
        }
    }
}

void NextLevel() {
    if (currentLevel < maxLevels) {
        currentLevel++;
        ResetMap();
        isInitialized = false;
        hasWon = false;
        showEnemies = true;
        canCyanEnemyLeave = false;
        canOrangeEnemyLeave = false;
        canEatGhosts = false;
        specialOrbs.clear();
    } else {
        // Player has completed all levels
        hasWon = true;
        menuState = WIN;
    }
}

/* ============================================== *
 * |             SCREEN SHAKE & PARTICLES       | *
 * ============================================== */

void ApplyScreenShake() {
    if (screenShake > 0) {
        screenOffset.x = (float)(GetRandomValue(-100, 100) / 100.0f) * screenShake;
        screenOffset.y = (float)(GetRandomValue(-100, 100) / 100.0f) * screenShake;
        screenShake *= 0.9f;  // Gradually reduce shake effect
    } else {
        screenOffset = { 0, 0 };
    }
}

/* ============================================== *
 * |                    MENU                    | *
 * ============================================== */

void DrawMenu() {
    // Semi-transparent overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
    
    const char* title = "";
    const char* options[3];
    int optionCount = 3;
    
    switch (menuState) {
        case PAUSE:
            title = "PAUSED";
            options[0] = "RESUME";
            options[1] = "RESTART";
            options[2] = "QUIT";
            break;
        case GAME_OVER:
            title = "GAME OVER";
            options[0] = "RESTART";
            options[1] = "NEXT LEVEL";
            options[2] = "QUIT";
            if (currentLevel >= maxLevels) {
                options[1] = "RESTART GAME";
            }
            break;
        case WIN:
            if (currentLevel >= maxLevels) {
                title = "CONGRATULATIONS! ALL LEVELS COMPLETED!";
                options[0] = "RESTART GAME";
                options[1] = "QUIT";
                optionCount = 2;
            } else {
                title = "LEVEL COMPLETED!";
                options[0] = "NEXT LEVEL";
                options[1] = "RESTART";
                options[2] = "QUIT";
            }
            break;
        default:
            return;
    }
    
    // Draw title
    int titleWidth = MeasureText(title, 48);
    DrawText(title, (screenWidth - titleWidth) / 2, 200, 48, playerColor);
    
    // Draw current level info
    if (menuState != WIN || currentLevel < maxLevels) {
        char levelText[32];
        sprintf(levelText, "LEVEL %d", currentLevel);
        int levelWidth = MeasureText(levelText, 32);
        DrawText(levelText, (screenWidth - levelWidth) / 2, 260, 32, uiTextColor);
    }
    
    // Draw menu options
    for (int i = 0; i < optionCount; i++) {
        Color color = (i == selectedMenuItem) ? playerColor : uiTextColor;
        int textWidth = MeasureText(options[i], 32);
        int yPos = 350 + i * 60;
        
        if (i == selectedMenuItem) {
            // Draw selection background
            DrawRectangleRounded({(float)(screenWidth - textWidth) / 2 - 20, (float)yPos - 10, (float)textWidth + 40, 50}, 0.3f, 10, Fade(playerColor, 0.2f));
        }
        
        DrawText(options[i], (screenWidth - textWidth) / 2, yPos, 32, color);
    }
    
    // Draw controls
    DrawText("USE ARROW KEYS TO NAVIGATE, ENTER TO SELECT", 
             (screenWidth - MeasureText("USE ARROW KEYS TO NAVIGATE, ENTER TO SELECT", 20)) / 2, 
             screenHeight - 50, 20, Fade(uiTextColor, 0.7f));
}

void HandleMenuInput() {
    int maxOptions = 3;
    if (menuState == WIN && currentLevel >= maxLevels) maxOptions = 2;
    
    if (IsKeyPressed(KEY_UP)) {
        selectedMenuItem = (selectedMenuItem - 1 + maxOptions) % maxOptions;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        selectedMenuItem = (selectedMenuItem + 1) % maxOptions;
    }
    
    if (IsKeyPressed(KEY_ENTER)) {
        switch (menuState) {
            case PAUSE:
                if (selectedMenuItem == 0) { // Resume
                    showMenu = false;
                    menuState = NONE;
                } else if (selectedMenuItem == 1) { // Restart
                    currentLevel = 1;
                    ResetMap();
                    isInitialized = false;
                    gameOver = false;
                    hasWon = false;
                    showMenu = false;
                    menuState = NONE;
                } else if (selectedMenuItem == 2) { // Quit
                    CloseWindow();  // This shuts down everything initialized by raylib
                    exit(0);        // This terminates the program immediately
                }
                break;
            case GAME_OVER:
                if (selectedMenuItem == 0) { // Restart
                    currentLevel = 1;
                    ResetMap();
                    isInitialized = false;
                    gameOver = false;
                    hasWon = false;
                    showMenu = false;
                    menuState = NONE;
                } else if (selectedMenuItem == 1) { // Next Level or Restart Game
                    if (currentLevel >= maxLevels) {
                        currentLevel = 1;
                    }
                    ResetMap();
                    isInitialized = false;
                    gameOver = false;
                    hasWon = false;
                    showMenu = false;
                    menuState = NONE;
                } else if (selectedMenuItem == 2) { // Quit
                    CloseWindow();  // This shuts down everything initialized by raylib
                    exit(0);        // This terminates the program immediately
                }
                break;
            case WIN:
                if (currentLevel >= maxLevels) {
                    if (selectedMenuItem == 0) { // Restart Game
                        currentLevel = 1;
                        ResetMap();
                        isInitialized = false;
                        gameOver = false;
                        hasWon = false;
                        showMenu = false;
                        menuState = NONE;
                    } else if (selectedMenuItem == 1) { // Quit
                        CloseWindow();  // This shuts down everything initialized by raylib
                        exit(0);        // This terminates the program immediately
                    }
                } else {
                    if (selectedMenuItem == 0) { // Next Level
                        NextLevel();
                        showMenu = false;
                        menuState = NONE;
                    } else if (selectedMenuItem == 1) { // Restart
                        currentLevel = 1;
                        ResetMap();
                        isInitialized = false;
                        gameOver = false;
                        hasWon = false;
                        showMenu = false;
                        menuState = NONE;
                    } else if (selectedMenuItem == 2) { // Quit
                        CloseWindow();  // This shuts down everything initialized by raylib
                        exit(0);        // This terminates the program immediately
                    }
                }
                break;
            default:
                break;
        }
        selectedMenuItem = 0; // Reset selection
    }
}

/* ============================================== *
 * |                    PLAYER                  | *
 * ============================================== */
class Player {
public:
    float x, y;  // Current position in pixels
    int gridX, gridY;  // Current grid position
    int direction;
    int plannedDirection;
    bool animateMouth;
    bool started = false;
    bool isMoving = false;  // Flag to check if player is moving
    float speed = 3.0f;     // Speed for smooth movement
    float targetX, targetY; // Target position for smooth transition
    float mouthAngle;
    bool mouthOpening;

    int score = 0;
    int health = 3;

    // Power-up variables
    bool powerUpActive = false;
    std::chrono::steady_clock::time_point powerUpStartTime;
    const int powerUpDuration = 5;  // Duration of power-up in seconds

    void Init() {
        // Initialize grid and screen position
        gridX = mapCols / 2;
        gridY = mapRows / 2 + 2;
        x = gridX * tileSize + tileSize / 2;
        y = gridY * tileSize + tileSize / 2;

        animateMouth = false;
        direction = 0;  // Default direction
        plannedDirection = 0;  // Default direction
        mouthAngle = 0.0f;
        mouthOpening = true;
    }

    void LoseHealth() {
        health -= 1;
        isMoving = false;
        started = false;
        isInitialized = false;
        screenShake = 10.0f;  // Trigger screen shake effect

        canEatGhosts = false;

        // Reset the timer on death
        startTime = std::chrono::steady_clock::now();
        elapsedTime = 0.0f;  // Reset elapsed time

        if (health == 0) {
            gameOver = true;
            showMenu = true;
            menuState = GAME_OVER;
        }
    }

    void Update() {
        // Check if the power-up timer is active and disable `canEatGhosts` if 5 seconds have passed
        if (powerUpActive) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - powerUpStartTime).count();
            if (elapsed >= powerUpDuration) {
                canEatGhosts = false;
                powerUpActive = false;  // Stop the timer
            }
        }

        if (animateMouth) {
            // Animate mouth opening and closing
            float maxMouthAngle = 50.0f;
            float mouthSpeed = 5.0f;

            if (mouthOpening) {
                mouthAngle += mouthSpeed;
                if (mouthAngle >= maxMouthAngle) {
                    mouthAngle = maxMouthAngle;
                    mouthOpening = false;
                }
            } else {
                mouthAngle -= mouthSpeed;
                if (mouthAngle <= 0.0f) {
                    mouthAngle = 0.0f;
                    mouthOpening = true;
                }
            }
        }

        if (!started) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
                if (!gameOver && !hasWon) {
                    started = true;
                    animateMouth = true;
                }
            }
        }

        // Update direction based on key input
        if (IsKeyPressed(KEY_LEFT)) plannedDirection = 0;
        if (IsKeyPressed(KEY_UP)) plannedDirection = 1;
        if (IsKeyPressed(KEY_RIGHT)) plannedDirection = 2;
        if (IsKeyPressed(KEY_DOWN)) plannedDirection = 3;

        if (map[gridY][gridX - 1] != 'X' && plannedDirection == 0) direction = 0;
        if (map[gridY - 1][gridX] != 'X' && plannedDirection == 1) direction = 1;
        if (map[gridY][gridX + 1] != 'X' && plannedDirection == 2) direction = 2;
        if (map[gridY + 1][gridX] != 'X' && plannedDirection == 3) direction = 3;

        if (!isMoving && started) {
            switch (direction) {
                case 0: if (map[gridY][gridX - 1] != 'X') {gridX--; isMoving = true;} break;
                case 1: if (map[gridY - 1][gridX] != 'X') {gridY--; isMoving = true;} break;
                case 2: if (map[gridY][gridX + 1] != 'X') {gridX++; isMoving = true;} break;
                case 3: if (map[gridY + 1][gridX] != 'X') {gridY++; isMoving = true;} break;
            }
            if (isMoving) {
                targetX = gridX * tileSize + tileSize / 2;
                targetY = gridY * tileSize + tileSize / 2;
                animateMouth = true;
            } else {
                animateMouth = false;
            }
        }

        if (isMoving) {
            float currentSpeed = speed * GetLevelSpeedMultiplier();
            if (fabs(x - targetX) <= currentSpeed && fabs(y - targetY) <= currentSpeed) {
                x = targetX;
                y = targetY;
                isMoving = false;
            } else {
                if (x < targetX) x += currentSpeed;
                if (x > targetX) x -= currentSpeed;
                if (y < targetY) y += currentSpeed;
                if (y > targetY) y -= currentSpeed;
            }
        }

        // Check for screen wrap and adjust both grid and screen positions
        if (x < 0) {
            x = screenWidth;
            gridX = mapCols - 1;
            targetX = x;
        } else if (x > screenWidth) {
            x = 0;
            gridX = 0;
            targetX = x;
        }

        if (y < 0) {
            y = screenHeight;
            gridY = mapRows - 1;
            targetY = y;
        } else if (y > screenHeight) {
            y = 0;
            gridY = 0;
            targetY = y;
        }

        // Eat the food (Replace ones with zeros)
        if (map[gridY][gridX] == '1') {
            map[gridY][gridX] = '0';
            score += 10;
            if (AreAllOrbsCollected()) {
                isMoving = false;
                canEatGhosts = false;
                isInitialized = false;
                started = false;
                showEnemies = false;
                hasWon = true;
                showMenu = true;
                menuState = WIN;
            }
        }

        // Check if a special orb is eaten
        for (auto it = specialOrbs.begin(); it != specialOrbs.end();) {
            if (it->first == gridX && it->second == gridY) {
                screenShake = 5.0f;
                it = specialOrbs.erase(it);  // Remove special orb from the list
                canEatGhosts = true;  // Enable power-up
                forceNormalRedGhost = false;
                forceNormalCyanGhost = false;
                forceNormalOrangeGhost = false;
                forceNormalPinkGhost = false;
                powerUpActive = true;  // Start the timer
                powerUpStartTime = std::chrono::steady_clock::now();  // Reset power-up timer
            } else {
                ++it;
            }
        }
    }

    void Draw() {
        rlPushMatrix();
        rlTranslatef(x, y, 0);

        // Apply rotation based on direction
        switch (direction) {
            case 0: rlRotatef(0, 0, 0, 1); break;       // Left (0 degrees)
            case 1: rlRotatef(90, 0, 0, 1); break;      // Up (90 degrees)
            case 2: rlRotatef(180, 0, 0, 1); break;     // Right (180 degrees)
            case 3: rlRotatef(-90, 0, 0, 1); break;     // Down (-90 degrees)
        }

        // Scale Pacman based on tile size
        float pacmanRadius = tileSize * 0.35f;

        // Draw Pacman glow layers
        DrawCircle(0, 0, pacmanRadius + 4, Fade(YELLOW, 0.2f));
        DrawCircle(0, 0, pacmanRadius + 2, Fade(YELLOW, 0.4f));
        DrawCircle(0, 0, pacmanRadius, Fade(YELLOW, 0.6f));

        // Draw Pacman body
        DrawCircle(0, 0, pacmanRadius, playerColor);

        // Draw the mouth as a triangle with dynamic angle
        if (mouthAngle > 0.0f) {
            float radAngle = mouthAngle * DEG2RAD;
            float mouthSize = pacmanRadius;
            DrawTriangle(
                {-pacmanRadius - 4, -mouthSize * sin(radAngle)},   // Left corner of mouth
                {-pacmanRadius - 4, mouthSize * sin(radAngle)},    // Right corner of mouth
                {mouthSize * cos(radAngle), 0},      // Tip of mouth
                BLACK
            );
        }

        rlPopMatrix();

        // Draw indicator triangle based on plannedDirection
        Vector2 p1, p2, p3; // Points of the indicator triangle
        float offsetDistance = tileSize * 0.6f; // Distance from Pacman
        float triangleSize = tileSize * 0.25f;

        switch (plannedDirection) {
            case 0:  // Left
                p1 = {x - offsetDistance, y - triangleSize};
                p2 = {x - offsetDistance - triangleSize, y};
                p3 = {x - offsetDistance, y + triangleSize};
                break;
            case 1:  // Up
                p1 = {x - triangleSize, y - offsetDistance};
                p2 = {x + triangleSize, y - offsetDistance};
                p3 = {x, y - offsetDistance - triangleSize};
                break;
            case 2:  // Right
                p1 = {x + offsetDistance, y - triangleSize};
                p2 = {x + offsetDistance, y + triangleSize};
                p3 = {x + offsetDistance + triangleSize, y};
                break;
            case 3:  // Down
                p1 = {x - triangleSize, y + offsetDistance};
                p2 = {x, y + offsetDistance + triangleSize};
                p3 = {x + triangleSize, y + offsetDistance};
                break;
            default:
                return; // No triangle if `plannedDirection` is invalid
        }

        // Draw the direction indicator triangle
        DrawTriangle(p1, p2, p3, lightYellow);
    }
};

Player player = {};

/* ============================================== *
 * |                    ENEMIES                 | *
 * ============================================== */

// Red Enemy
class Red_Enemy {
public:
    float x, y;  // Current position in pixels
    int gridX, gridY;  // Grid position
    float speed;  // Speed in pixels per frame
    float targetX, targetY;  // Next tile target position in pixels

    void Init() {
        // Set initial grid position
        gridX = mapCols / 2;
        gridY = mapRows / 2 - 1; // Start in enemy box
        x = gridX * tileSize + tileSize / 2;
        y = gridY * tileSize + tileSize / 2;
        targetX = x;
        targetY = y;
        speed = 2.25f;
    }

    void Update(int playerX, int playerY) {
        // Set speed based on `canEatGhosts` state and level
        if (canEatGhosts && !forceNormalRedGhost) {
            speed = 1.5f * GetLevelSpeedMultiplier();
        } else {
            float baseSpeed = 2.0f * GetLevelSpeedMultiplier();
            float timeFactor = elapsedTime / 70.0f;
            speed = std::min(baseSpeed + timeFactor, 3.5f * GetLevelSpeedMultiplier());
        }

        // Determine the target based on `canEatGhosts`
        std::pair<int, int> targetGridPos = canEatGhosts && !forceNormalRedGhost ? FindFurthestPoint(playerX, playerY) : std::make_pair(playerX, playerY);

        if (player.started) {
            // Check if the enemy reached its target grid position
            if (fabs(x - targetX) < speed && fabs(y - targetY) < speed) {
                x = targetX;
                y = targetY;

                // Determine next step using pathfinding
                std::pair<int, int> nextStep = AStarPathfinding(targetGridPos.first, targetGridPos.second);
                if (nextStep.first != -1 && nextStep.second != -1) {
                    gridX = nextStep.first;
                    gridY = nextStep.second;
                    targetX = gridX * tileSize + tileSize / 2;
                    targetY = gridY * tileSize + tileSize / 2;
                }
            }

            // Smoothly move towards the target position
            if (x < targetX) x += speed;
            if (x > targetX) x -= speed;
            if (y < targetY) y += speed;
            if (y > targetY) y -= speed;

            // Check collision with player
            float collisionRadius = tileSize * 0.4f;
            Rectangle playerRect = {player.x - collisionRadius, player.y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            Rectangle selfRect = {x - collisionRadius, y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            if (CheckCollisionRecs(selfRect, playerRect)) {
                if (canEatGhosts && !forceNormalRedGhost) {
                    forceNormalRedGhost = true;
                    screenShake = 5.0f;
                    player.score += 200 * currentLevel; // Bonus points increase with level
                    Init();  // Reset enemy if eaten
                } else {
                    player.LoseHealth();
                }
            }
        }
    }

    void Draw() {
        Color enemyColor = canEatGhosts && !forceNormalRedGhost ? BLUE : enemyRedColor;
        Color eyeColor = canEatGhosts && !forceNormalRedGhost ? BLACK : WHITE;

        float ghostRadius = tileSize * 0.35f;

        // Draw body
        DrawCircle(static_cast<int>(x), static_cast<int>(y), ghostRadius, enemyColor);
        DrawRectangle(static_cast<int>(x) - ghostRadius - 1, static_cast<int>(y), ghostRadius * 2 + 2, ghostRadius + 2, enemyColor);

        // Draw eyes
        float eyeSize = tileSize * 0.08f;
        DrawEllipse(static_cast<int>(x) - ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, eyeColor);
        DrawEllipse(static_cast<int>(x) + ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, eyeColor);
    }

private:
    std::pair<int, int> FindFurthestPoint(int playerX, int playerY) {
        int furthestX = gridX, furthestY = gridY;
        int maxDistance = 0;

        for (int row = 0; row < mapRows; row++) {
            for (int col = 0; col < mapCols; col++) {
                if (map[row][col] != 'X') {  // Only non-wall tiles
                    int dist = (row - playerY) * (row - playerY) + (col - playerX) * (col - playerX);
                    if (dist > maxDistance) {
                        maxDistance = dist;
                        furthestX = col;
                        furthestY = row;
                    }
                }
            }
        }

        return {furthestX, furthestY};
    }

    int Heuristic(int x1, int y1, int x2, int y2) {
        return abs(x1 - x2) + abs(y1 - y2);
    }

    std::pair<int, int> AStarPathfinding(int targetX, int targetY) {
        struct Node {
            int x, y, cost, priority;
            bool operator>(const Node& other) const { return priority > other.priority; }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
        std::unordered_map<int, std::pair<int, int>> cameFrom;
        std::unordered_map<int, int> costSoFar;

        int startKey = gridY * mapCols + gridX;
        int targetKey = targetY * mapCols + targetX;

        openList.push({gridX, gridY, 0, Heuristic(gridX, gridY, targetX, targetY)});
        costSoFar[startKey] = 0;

        while (!openList.empty()) {
            Node current = openList.top();
            openList.pop();
            int currentKey = current.y * mapCols + current.x;

            if (currentKey == targetKey) {
                return ReconstructPath(cameFrom, targetX, targetY);
            }

            std::vector<std::pair<int, int>> neighbors = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto& neighbor : neighbors) {
                int newX = current.x + neighbor.first;
                int newY = current.y + neighbor.second;
                int nextKey = newY * mapCols + newX;

                if (newX < 0 || newX >= mapCols || newY < 0 || newY >= mapRows || map[newY][newX] == 'X') {
                    continue;
                }

                int newCost = costSoFar[currentKey] + 1;
                if (costSoFar.find(nextKey) == costSoFar.end() || newCost < costSoFar[nextKey]) {
                    costSoFar[nextKey] = newCost;
                    int priority = newCost + Heuristic(newX, newY, targetX, targetY);
                    openList.push({newX, newY, newCost, priority});
                    cameFrom[nextKey] = {current.x, current.y};
                }
            }
        }

        return {-1, -1};  // No path found
    }

    std::pair<int, int> ReconstructPath(std::unordered_map<int, std::pair<int, int>>& cameFrom, int targetX, int targetY) {
        int currentX = targetX;
        int currentY = targetY;
        std::pair<int, int> nextStep = {gridX, gridY};

        while (cameFrom.find(currentY * mapCols + currentX) != cameFrom.end()) {
            nextStep = {currentX, currentY};
            auto prev = cameFrom[currentY * mapCols + currentX];
            currentX = prev.first;
            currentY = prev.second;
            if (currentX == gridX && currentY == gridY) break;
        }

        return nextStep;
    }
};

// Pink Enemy
class Pink_Enemy {
public:
    float x, y;
    int gridX, gridY;
    float speed;
    float targetX, targetY;
    int stuckCounter;
    std::pair<int, int> lastPos;

    void Init() {
        gridX = mapCols / 2;
        gridY = mapRows / 2;
        x = gridX * tileSize + tileSize / 2;
        y = gridY * tileSize + tileSize / 2;
        targetX = x;
        targetY = y;
        speed = 2.0f;
        stuckCounter = 0;
        lastPos = {gridX, gridY};
    }

    void Update(int playerX, int playerY, int playerDirection) {
        int interceptX = playerX;
        int interceptY = playerY;

        if (canEatGhosts && !forceNormalPinkGhost) {
            speed = 1.5f * GetLevelSpeedMultiplier();
            std::pair<int, int> farthestPoint = FindFurthestPoint(playerX, playerY);
            interceptX = farthestPoint.first;
            interceptY = farthestPoint.second;
        } else {
            speed = 2.0f * GetLevelSpeedMultiplier();
            switch (playerDirection) {
                case LEFT: interceptX = playerX - 2; break;
                case UP: interceptY = playerY - 2; break;
                case RIGHT: interceptX = playerX + 2; break;
                case DOWN: interceptY = playerY + 2; break;
            }
            interceptX = std::max(0, std::min(mapCols - 1, interceptX));
            interceptY = std::max(0, std::min(mapRows - 1, interceptY));
        }

        if (player.started) {
            if (fabs(x - targetX) < speed && fabs(y - targetY) < speed) {
                x = targetX;
                y = targetY;

                if (lastPos == std::make_pair(gridX, gridY)) {
                    stuckCounter++;
                } else {
                    stuckCounter = 0;
                    lastPos = {gridX, gridY};
                }

                std::pair<int, int> nextStep = BFSPathfinding(interceptX, interceptY);
                if (nextStep.first != -1 && nextStep.second != -1) {
                    gridX = nextStep.first;
                    gridY = nextStep.second;
                    targetX = gridX * tileSize + tileSize / 2;
                    targetY = gridY * tileSize + tileSize / 2;
                }
            }

            if (x < targetX) x += speed;
            if (x > targetX) x -= speed;
            if (y < targetY) y += speed;
            if (y > targetY) y -= speed;

            float collisionRadius = tileSize * 0.4f;
            Rectangle playerRect = {player.x - collisionRadius, player.y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            Rectangle selfRect = {x - collisionRadius, y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            if (CheckCollisionRecs(selfRect, playerRect)) {
                if (canEatGhosts && !forceNormalPinkGhost) {
                    forceNormalPinkGhost = true;
                    screenShake = 5.0f;
                    player.score += 200 * currentLevel;
                    Init();
                } else {
                    player.LoseHealth();
                }
            }
        }
    }

    void Draw() {
        Color enemyColor = canEatGhosts && !forceNormalPinkGhost ? BLUE : enemyPinkColor;
        float ghostRadius = tileSize * 0.35f;

        DrawCircle(static_cast<int>(x), static_cast<int>(y), ghostRadius, enemyColor);
        DrawRectangle(static_cast<int>(x) - ghostRadius - 1, static_cast<int>(y), ghostRadius * 2 + 2, ghostRadius + 2, enemyColor);
        
        float eyeSize = tileSize * 0.08f;
        DrawEllipse(static_cast<int>(x) - ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
        DrawEllipse(static_cast<int>(x) + ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
    }

private:
    std::pair<int, int> FindFurthestPoint(int playerX, int playerY) {
        int furthestX = gridX, furthestY = gridY;
        int maxDistance = 0;

        for (int row = 0; row < mapRows; row++) {
            for (int col = 0; col < mapCols; col++) {
                if (map[row][col] != 'X') {
                    int dist = (row - playerY) * (row - playerY) + (col - playerX) * (col - playerX);
                    if (dist > maxDistance) {
                        maxDistance = dist;
                        furthestX = col;
                        furthestY = row;
                    }
                }
            }
        }

        return {furthestX, furthestY};
    }

    std::pair<int, int> BFSPathfinding(int targetX, int targetY) {
        struct Node { int x, y; };
        std::queue<Node> queue;
        std::unordered_map<int, std::pair<int, int>> cameFrom;
        queue.push({gridX, gridY});
        cameFrom[gridY * mapCols + gridX] = {-1, -1};

        while (!queue.empty()) {
            Node current = queue.front();
            queue.pop();
            if (current.x == targetX && current.y == targetY) {
                return ReconstructPath(cameFrom, targetX, targetY);
            }

            std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto& dir : directions) {
                int newX = current.x + dir.first;
                int newY = current.y + dir.second;
                int newKey = newY * mapCols + newX;

                if (newX >= 0 && newX < mapCols && newY >= 0 && newY < mapRows && map[newY][newX] != 'X' &&
                    cameFrom.find(newKey) == cameFrom.end()) {
                    queue.push({newX, newY});
                    cameFrom[newKey] = {current.x, current.y};
                }
            }
        }

        return {-1, -1};
    }

    std::pair<int, int> ReconstructPath(std::unordered_map<int, std::pair<int, int>>& cameFrom, int targetX, int targetY) {
        int currentX = targetX;
        int currentY = targetY;
        std::pair<int, int> nextStep = {gridX, gridY};

        while (cameFrom.find(currentY * mapCols + currentX) != cameFrom.end()) {
            nextStep = {currentX, currentY};
            auto prev = cameFrom[currentY * mapCols + currentX];
            currentX = prev.first;
            currentY = prev.second;
            if (currentX == gridX && currentY == gridY) break;
        }

        return nextStep;
    }
};

class Cyan_Enemy {
public:
    float x, y;
    int gridX, gridY;
    float speed;
    float targetX, targetY;
    int stuckCounter;
    std::pair<int, int> lastPos;

    void Init() {
        gridX = mapCols / 2 + 1;
        gridY = mapRows / 2;
        x = gridX * tileSize + tileSize / 2;
        y = gridY * tileSize + tileSize / 2;
        targetX = x;
        targetY = y;
        speed = 1.75f;
        stuckCounter = 0;
        lastPos = {gridX, gridY};
        canCyanEnemyLeave = false;
    }

    void Update(int playerX, int playerY, int playerDirection) {
        if (player.score > GetLevelScoreRequired()) {
            canCyanEnemyLeave = true;
        } else {
            canCyanEnemyLeave = false;
        }

        if (canEatGhosts && !forceNormalCyanGhost) {
            speed = 1.5f * GetLevelSpeedMultiplier();
        } else {
            speed = 1.75f * GetLevelSpeedMultiplier();
        }

        if (player.started && canCyanEnemyLeave) {
            std::pair<int, int> targetGridPos = canEatGhosts && !forceNormalCyanGhost ? FindFurthestPoint(playerX, playerY) : PredictIntercept(playerX, playerY, playerDirection);

            if (fabs(x - targetX) < speed && fabs(y - targetY) < speed) {
                x = targetX;
                y = targetY;

                if (lastPos == std::make_pair(gridX, gridY)) {
                    stuckCounter++;
                } else {
                    stuckCounter = 0;
                    lastPos = {gridX, gridY};
                }

                std::pair<int, int> nextStep = BFSPathfinding(targetGridPos.first, targetGridPos.second);
                if (nextStep.first != -1 && nextStep.second != -1) {
                    gridX = nextStep.first;
                    gridY = nextStep.second;
                    targetX = gridX * tileSize + tileSize / 2;
                    targetY = gridY * tileSize + tileSize / 2;
                }
            }

            if (x < targetX) x += speed;
            if (x > targetX) x -= speed;
            if (y < targetY) y += speed;
            if (y > targetY) y -= speed;

            float collisionRadius = tileSize * 0.4f;
            Rectangle playerRect = {player.x - collisionRadius, player.y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            Rectangle selfRect = {x - collisionRadius, y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            if (CheckCollisionRecs(selfRect, playerRect)) {
                if (canEatGhosts && !forceNormalCyanGhost) {
                    forceNormalCyanGhost = true;
                    screenShake = 5.0f;
                    player.score += 200 * currentLevel;
                    Init();
                } else {
                    player.LoseHealth();
                }
            }
        }
    }

    void Draw() {
        Color enemyColor = canEatGhosts && !forceNormalCyanGhost ? BLUE : enemyCyanColor;
        float ghostRadius = tileSize * 0.35f;

        DrawCircle(static_cast<int>(x), static_cast<int>(y), ghostRadius, enemyColor);
        DrawRectangle(static_cast<int>(x) - ghostRadius - 1, static_cast<int>(y), ghostRadius * 2 + 2, ghostRadius + 2, enemyColor);
        
        float eyeSize = tileSize * 0.08f;
        DrawEllipse(static_cast<int>(x) - ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
        DrawEllipse(static_cast<int>(x) + ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
    }

private:
    std::pair<int, int> FindFurthestPoint(int playerX, int playerY) {
        int furthestX = gridX, furthestY = gridY;
        int maxDistance = 0;

        for (int row = 0; row < mapRows; row++) {
            for (int col = 0; col < mapCols; col++) {
                if (map[row][col] != 'X') {
                    int dist = (row - playerY) * (row - playerY) + (col - playerX) * (col - playerX);
                    if (dist > maxDistance) {
                        maxDistance = dist;
                        furthestX = col;
                        furthestY = row;
                    }
                }
            }
        }

        return {furthestX, furthestY};
    }

    std::pair<int, int> PredictIntercept(int playerX, int playerY, int playerDirection) {
        int interceptX = playerX;
        int interceptY = playerY;

        switch (playerDirection) {
            case LEFT: interceptX = playerX - 2; break;
            case UP: interceptY = playerY - 2; break;
            case RIGHT: interceptX = playerX + 2; break;
            case DOWN: interceptY = playerY + 2; break;
        }

        interceptX = std::max(0, std::min(mapCols - 1, interceptX));
        interceptY = std::max(0, std::min(mapRows - 1, interceptY));

        return {interceptX, interceptY};
    }

    std::pair<int, int> BFSPathfinding(int targetX, int targetY) {
        struct Node { int x, y; };
        std::queue<Node> queue;
        std::unordered_map<int, std::pair<int, int>> cameFrom;
        queue.push({gridX, gridY});
        cameFrom[gridY * mapCols + gridX] = {-1, -1};

        while (!queue.empty()) {
            Node current = queue.front();
            queue.pop();
            if (current.x == targetX && current.y == targetY) {
                return ReconstructPath(cameFrom, targetX, targetY);
            }

            std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto& dir : directions) {
                int newX = current.x + dir.first;
                int newY = current.y + dir.second;
                int newKey = newY * mapCols + newX;

                if (newX >= 0 && newX < mapCols && newY >= 0 && newY < mapRows && map[newY][newX] != 'X' &&
                    cameFrom.find(newKey) == cameFrom.end()) {
                    queue.push({newX, newY});
                    cameFrom[newKey] = {current.x, current.y};
                }
            }
        }

        return {-1, -1};
    }

    std::pair<int, int> ReconstructPath(std::unordered_map<int, std::pair<int, int>>& cameFrom, int targetX, int targetY) {
        int currentX = targetX;
        int currentY = targetY;
        std::pair<int, int> nextStep = {gridX, gridY};

        while (cameFrom.find(currentY * mapCols + currentX) != cameFrom.end()) {
            nextStep = {currentX, currentY};
            auto prev = cameFrom[currentY * mapCols + currentX];
            currentX = prev.first;
            currentY = prev.second;
            if (currentX == gridX && currentY == gridY) break;
        }

        return nextStep;
    }
};

// Orange Enemy
class Orange_Enemy {
public:
    float x, y;
    int gridX, gridY;
    float speed;
    float targetX, targetY;
    int destX, destY;

    void Init() {
        gridX = mapCols / 2 - 1;
        gridY = mapRows / 2;
        x = gridX * tileSize + tileSize / 2;
        y = gridY * tileSize + tileSize / 2;
        targetX = x;
        targetY = y;
        speed = 1.5f;
        PickRandomDestination();
        canOrangeEnemyLeave = false;
    }

    void Update() {
        if (player.score > GetLevelScoreRequired() * 2) {
            canOrangeEnemyLeave = true;
        } else {
            canOrangeEnemyLeave = false;
        }

        speed = 1.5f * GetLevelSpeedMultiplier();

        if (player.started && canOrangeEnemyLeave) {
            if (fabs(x - targetX) < speed && fabs(y - targetY) < speed) {
                x = targetX;
                y = targetY;

                if (gridX == destX && gridY == destY) {
                    PickRandomDestination();
                } else {
                    std::pair<int, int> nextStep = AStarPathfinding(destX, destY);
                    if (nextStep.first != -1 && nextStep.second != -1) {
                        gridX = nextStep.first;
                        gridY = nextStep.second;
                        targetX = gridX * tileSize + tileSize / 2;
                        targetY = gridY * tileSize + tileSize / 2;
                    }
                }
            }

            if (x < targetX) x += speed;
            if (x > targetX) x -= speed;
            if (y < targetY) y += speed;
            if (y > targetY) y -= speed;

            float collisionRadius = tileSize * 0.4f;
            Rectangle playerRect = {player.x - collisionRadius, player.y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            Rectangle selfRect = {x - collisionRadius, y - collisionRadius, collisionRadius * 2, collisionRadius * 2};
            if (CheckCollisionRecs(selfRect, playerRect)) {
                if (canEatGhosts && !forceNormalOrangeGhost) {
                    forceNormalOrangeGhost = true;
                    screenShake = 5.0f;
                    player.score += 200 * currentLevel;
                    Init();
                } else {
                    player.LoseHealth();
                }
            }
        }
    }

    void Draw() {
        Color enemyColor = canEatGhosts && !forceNormalOrangeGhost ? BLUE : enemyOrangeColor;
        float ghostRadius = tileSize * 0.35f;

        DrawCircle(static_cast<int>(x), static_cast<int>(y), ghostRadius, enemyColor);
        DrawRectangle(static_cast<int>(x) - ghostRadius - 1, static_cast<int>(y), ghostRadius * 2 + 2, ghostRadius + 2, enemyColor);
        
        float eyeSize = tileSize * 0.08f;
        DrawEllipse(static_cast<int>(x) - ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
        DrawEllipse(static_cast<int>(x) + ghostRadius * 0.3f, static_cast<int>(y) - ghostRadius * 0.3f, eyeSize, eyeSize * 1.5f, BLACK);
    }

private:
    void PickRandomDestination() {
        do {
            destX = GetRandomValue(0, mapCols - 1);
            destY = GetRandomValue(0, mapRows - 1);
        } while (map[destY][destX] == 'X');
    }

    std::pair<int, int> AStarPathfinding(int targetX, int targetY) {
        struct Node {
            int x, y, cost, priority;
            bool operator>(const Node& other) const { return priority > other.priority; }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
        std::unordered_map<int, std::pair<int, int>> cameFrom;
        std::unordered_map<int, int> costSoFar;

        int startKey = gridY * mapCols + gridX;
        int targetKey = targetY * mapCols + targetX;

        openList.push({gridX, gridY, 0, Heuristic(gridX, gridY, targetX, targetY)});
        costSoFar[startKey] = 0;

        while (!openList.empty()) {
            Node current = openList.top();
            openList.pop();
            int currentKey = current.y * mapCols + current.x;

            if (currentKey == targetKey) {
                return ReconstructPath(cameFrom, targetX, targetY);
            }

            std::vector<std::pair<int, int>> neighbors = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto& neighbor : neighbors) {
                int newX = current.x + neighbor.first;
                int newY = current.y + neighbor.second;
                int nextKey = newY * mapCols + newX;

                if (newX < 0 || newX >= mapCols || newY < 0 || newY >= mapRows || map[newY][newX] == 'X') {
                    continue;
                }

                int newCost = costSoFar[currentKey] + 1;
                if (costSoFar.find(nextKey) == costSoFar.end() || newCost < costSoFar[nextKey]) {
                    costSoFar[nextKey] = newCost;
                    int priority = newCost + Heuristic(newX, newY, targetX, targetY);
                    openList.push({newX, newY, newCost, priority});
                    cameFrom[nextKey] = {current.x, current.y};
                }
            }
        }

        return {-1, -1};
    }

    std::pair<int, int> ReconstructPath(std::unordered_map<int, std::pair<int, int>>& cameFrom, int targetX, int targetY) {
        int currentX = targetX;
        int currentY = targetY;
        std::pair<int, int> nextStep = {gridX, gridY};

        while (cameFrom.find(currentY * mapCols + currentX) != cameFrom.end()) {
            nextStep = {currentX, currentY};
            auto prev = cameFrom[currentY * mapCols + currentX];
            currentX = prev.first;
            currentY = prev.second;
            if (currentX == gridX && currentY == gridY) break;
        }

        return nextStep;
    }

    int Heuristic(int x1, int y1, int x2, int y2) {
        return abs(x1 - x2) + abs(y1 - y2);
    }
};

Red_Enemy redEnemy = {};
Pink_Enemy pinkEnemy = {};
Cyan_Enemy cyanEnemy = {};
Orange_Enemy orangeEnemy = {};

/* ============================================== *
 * |                   MAP DRAWING              | *
 * ============================================== */

void DrawOrb(float x, float y) {
    float orbRadius = tileSize * 0.1f;
    DrawCircle(x, y, orbRadius, Fade(orbColor, 0.8f));
    DrawCircle(x, y, orbRadius * 0.7f, orbColor);
}

void DrawSpecialOrb(float x, float y) {
    float pulse = sin(GetTime() * 4.0f) * 0.2f + 1.0f;
    float orbRadius = tileSize * 0.2f;

    Color coreColor = {255, 223, 0, 255};
    Color glowColor = Fade(coreColor, 0.6f + 0.3f * pulse);

    DrawCircle(x, y, orbRadius * 1.5f * pulse, Fade(coreColor, 0.3f + 0.3f * pulse));
    DrawCircle(x, y, orbRadius * pulse, glowColor);
    DrawCircle(x, y, orbRadius * 0.6f, coreColor);

    if ((int)(GetTime() * 6) % 2 == 0) {
        DrawCircle(x + GetRandomValue(-3, 3), y + GetRandomValue(-3, 3), 1.5f, Fade(WHITE, 0.8f));
    }
}

void DrawMap() {
    float wallThickness = tileSize / 3.0f;

    for (int row = 0; row < mapRows; row++) {
        for (int col = 0; col < mapCols; col++) {
            if (map[row][col] == 'X') {
                float xCenter = col * tileSize + tileSize / 2.0f;
                float yCenter = row * tileSize + tileSize / 2.0f;

                if (col < mapCols - 1 && map[row][col + 1] == 'X') {
                    float xRightCenter = (col + 1) * tileSize + tileSize / 2.0f;
                    DrawLineEx({xCenter, yCenter}, {xRightCenter, yCenter}, wallThickness, wallColor);
                }

                if (row < mapRows - 1 && map[row + 1][col] == 'X') {
                    float yBottomCenter = (row + 1) * tileSize + tileSize / 2.0f;
                    DrawLineEx({xCenter, yCenter}, {xCenter, yBottomCenter}, wallThickness, wallColor);
                }
            }
            else if (map[row][col] == '1') {
                float xCenter = col * tileSize + tileSize / 2.0f;
                float yCenter = row * tileSize + tileSize / 2.0f;

                bool isSpecialOrb = false;
                for (auto& pos : specialOrbs) {
                    if (pos.first == col && pos.second == row) {
                        isSpecialOrb = true;
                        break;
                    }
                }

                if (isSpecialOrb) {
                    DrawSpecialOrb(xCenter, yCenter);
                } else {
                    DrawOrb(xCenter, yCenter);
                }
            }
        }
    }
}

/* ============================================== *
 * |                 USER INTERFACE             | *
 * ============================================== */
void DrawScore(int score) {
    float overlayWidth = 120;
    float overlayHeight = 35;
    float overlayX = (screenWidth - overlayWidth) / 2;
    float overlayY = 11;

    DrawRectangleRounded((Rectangle){overlayX, overlayY, overlayWidth, overlayHeight}, 0.5f, 10, wallColor);

    std::string scoreText = std::to_string(score);
    int textWidth = MeasureText(scoreText.c_str(), 24);
    int textX = overlayX + (overlayWidth - textWidth) / 2;
    int textY = overlayY + (overlayHeight - 24) / 2;

    DrawText(std::to_string(score).c_str(), textX, textY, 24, uiTextColor);
}

void DrawPacmanIcon(float x, float y, Color color) {
    float iconRadius = tileSize * 0.15f;
    DrawCircle(x, y, iconRadius, color);
    DrawTriangle({x + iconRadius, y + iconRadius * 0.6f}, {x + iconRadius, y - iconRadius * 0.6f}, {x - iconRadius * 0.3f, y}, wallColor);
}

void DrawHealth(int health) {
    float overlayWidth = 90;
    float overlayHeight = 35;
    float overlayX = screenWidth - overlayWidth - 11;
    float overlayY = 11;

    DrawRectangleRounded((Rectangle){overlayX, overlayY, overlayWidth, overlayHeight}, 0.5f, 10, wallColor);

    float iconSpacing = 24.0f;
    float totalWidth = (health - 1) * iconSpacing;
    float startX = overlayX + (overlayWidth - totalWidth) / 2;
    float iconY = overlayY + overlayHeight / 2;

    for (int i = 0; i < health; i++) {
        DrawPacmanIcon(startX + i * iconSpacing, iconY, PINK);
    }
}

void DrawLevel() {
    float overlayWidth = 80;
    float overlayHeight = 35;
    float overlayX = 11;
    float overlayY = 11;

    DrawRectangleRounded((Rectangle){overlayX, overlayY, overlayWidth, overlayHeight}, 0.5f, 10, wallColor);

    char levelText[16];
    sprintf(levelText, "LV %d", currentLevel);
    int textWidth = MeasureText(levelText, 20);
    int textX = overlayX + (overlayWidth - textWidth) / 2;
    int textY = overlayY + (overlayHeight - 20) / 2;

    DrawText(levelText, textX, textY, 20, uiTextColor);
}

void DrawOutlinedText(const char *text, int posX, int posY, int fontSize, Color color, int outlineSize, Color outlineColor) {
    DrawText(text, posX - outlineSize, posY - outlineSize, fontSize, outlineColor);
    DrawText(text, posX + outlineSize, posY - outlineSize, fontSize, outlineColor);
    DrawText(text, posX - outlineSize, posY + outlineSize, fontSize, outlineColor);
    DrawText(text, posX + outlineSize, posY + outlineSize, fontSize, outlineColor);
    DrawText(text, posX, posY, fontSize, color);
}

// Function to spawn a special orb if conditions are met
void SpawnSpecialOrb() {
    int currentTime = GetTime() * 1000;
    if (currentTime - lastSpecialOrbSpawn > specialOrbCooldown && specialOrbs.size() < maxSpecialOrbs) {
        lastSpecialOrbSpawn = currentTime;

        int row, col;
        do {
            row = GetRandomValue(0, mapRows - 1);
            col = GetRandomValue(0, mapCols - 1);
        } while (map[row][col] != '1' || map[row][col] == 'X');

        specialOrbs.emplace_back(col, row);
    }
}

void ResetSpecialOrbs() {
    specialOrbs.clear();
}

/* ============================================== *
 * |                 MAIN GAME LOGIC            | *
 * ============================================== */
void Game() {
    ApplyScreenShake();

    BeginDrawing();
    ClearBackground(backgroundColor);

    if (!isInitialized) {
        isInitialized = true;
        player.Init();

        canEatGhosts = false;
        canCyanEnemyLeave = false;
        canOrangeEnemyLeave = false;

        redEnemy.Init();
        pinkEnemy.Init();
        cyanEnemy.Init();
        orangeEnemy.Init();
    }

    // Handle menu input if menu is shown
    if (showMenu) {
        HandleMenuInput();
    } else {
        // Handle escape key to show pause menu
        if (IsKeyPressed(KEY_ESCAPE) && !gameOver && !hasWon) {
            showMenu = true;
            menuState = PAUSE;
            selectedMenuItem = 0;
        }
    }

    Camera2D camera = { 0 };
    camera.offset = screenOffset;
    camera.target = { 0, 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    if (!gameOver && !hasWon && !showMenu) {
        SpawnSpecialOrb();
    } else if (gameOver || hasWon) {
        ResetSpecialOrbs();
    }

    BeginMode2D(camera);

    DrawMap();

    // Only update game logic if not showing menu
    if (!showMenu) {
        player.Update();
        player.Draw();

        if (showEnemies) {
            redEnemy.Update(player.gridX, player.gridY);
            redEnemy.Draw();

            pinkEnemy.Update(player.gridX, player.gridY, player.direction);
            pinkEnemy.Draw();

            cyanEnemy.Update(player.gridX, player.gridY, player.direction);
            cyanEnemy.Draw();

            orangeEnemy.Update();
            orangeEnemy.Draw();
        }
    } else {
        // Still draw player and enemies when paused
        player.Draw();
        if (showEnemies) {
            redEnemy.Draw();
            pinkEnemy.Draw();
            cyanEnemy.Draw();
            orangeEnemy.Draw();
        }
    }

    // Draw UI elements
    DrawScore(player.score);
    DrawHealth(player.health);
    DrawLevel();

    // Start Text
    if (!player.started && !gameOver && !hasWon && !showMenu) {
        DrawOutlinedText("MOVE TO START", screenWidth / 2 - MeasureText("MOVE TO START", 32) / 2, screenHeight / 2, 32, playerColor, 2, backgroundColor);
        DrawOutlinedText("PRESS ESC FOR MENU", screenWidth / 2 - MeasureText("PRESS ESC FOR MENU", 24) / 2, screenHeight / 2 + 50, 24, uiTextColor, 2, backgroundColor);
    }

    EndMode2D();

    // Draw menu overlay if needed
    if (showMenu) {
        DrawMenu();
    }

    EndDrawing();
}

int main() {
    InitWindow(screenWidth, screenHeight, "Pacman Remake - Multi-Level Edition");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    SetExitKey(0);

    SetTargetFPS(60);
    HideCursor();

    const float timeStep = 1.0f / 60.0f;
    float accumulator = 0.0f;

    auto previousTime = std::chrono::steady_clock::now();

    while (!WindowShouldClose()) {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        accumulator += deltaTime;

        while (accumulator >= timeStep) {
            if (player.started && !showMenu) {
                elapsedTime += timeStep;
            }
            accumulator -= timeStep;
        }

        Game();
    }

    CloseWindow();
    return 0;
}