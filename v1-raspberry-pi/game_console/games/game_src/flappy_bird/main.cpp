#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm> // Include this for std::remove_if
#include <fstream> // Include for file operations

int highScore = 0;

// Function to load the high score from a file
void LoadHighScore() {
    std::ifstream file("flappy_bird_highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

// Function to save the high score to a file
void SaveHighScore() {
    std::ofstream file("flappy_bird_highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}


const int screenWidth = 800;
const int screenHeight = 480;

// Pipe spawning timer
float pipeSpawnTimer = 0.0f;
float pipeSpawnInterval = 2.0f; // Spawn a new pipe every 2 seconds

float speedMultiplier = 1.0f;

bool isInitialized = false;
bool gameOver = false;

bool started = false;

// Screen shake variables
float screenShake = 0.0f;
Vector2 screenOffset = { 0, 0 };

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
 * |                    PLAYER                  | *
 * ============================================== */
class Player {
public:
    float x;
    float y;
    float gravity;
    float velocity;
    float jumpStrength;

    int score;

    bool canJump;

    void Init() {
        x = 300;
        y = screenHeight / 2;
        gravity = 0.5f;         // Gravity acceleration (tweak as needed)
        velocity = 0.0f;        // Initial velocity
        jumpStrength = -8.0f;  // Upward force when jumping (negative because y goes up)
        score = 0;
    }

    void Update() {
        velocity += gravity;   // Accumulate gravity to velocity

        if (IsKeyPressed(KEY_ENTER)) {
            velocity = jumpStrength;  // Apply jump force
            started = true;
        }

        if (started)
            y += velocity;         // Update the position based on velocity

        // Add bounds checking to prevent falling off the screen (optional)
        if (y > screenHeight) {
            y = screenHeight;  // Reset to bottom of the screen
            velocity = 0;      // Reset velocity
            gameOver = true;
            screenShake = 10.0f;
        } else if (y < 0) {
            y = 0;             // Reset to top of the screen
            velocity = 0;      // Reset velocity
        }
    }

    void Draw() {
        DrawRectangle(x, y, 30, 30, YELLOW);
        DrawRectangle(x + 1, y + 1, 28, 28, GRAY);
        DrawRectangle(x + 2, y + 2, 26, 26, YELLOW);
    }
};

Player player = {};

class Pipe {
public:
    float x;
    float containerY;  // This will act as the invisible container's Y offset
    float speed;
    int height;
    int width;

    Rectangle successRect;       // Rectangle for the passable area
    Rectangle lowerRect;         // Rectangle for the bottom pipe collision
    Rectangle lowerExtendedRect; // Extended rectangle for bottom pipe collision off-screen
    Rectangle upperRect;         // Rectangle for the top pipe collision
    Rectangle upperExtendedRect; // Extended rectangle for top pipe collision off-screen

    Color pipeColor;
    Color capColor;
    Color borderColor;
    int borderThickness;

    bool disabled = false;

    Pipe() {
        x = screenWidth;
        containerY = GetRandomValue(-120, 120);  // Randomize vertical offset

        speed = 3.0f;
        height = 160;
        width = 50;
        pipeColor = DARKGREEN;
        capColor = GREEN;
        borderColor = BLACK;
        borderThickness = 4;

        // Define the passable area for success detection
        successRect = { x + width / 2, containerY + static_cast<float>(screenHeight - 2 * height), 1.0f, static_cast<float>(height) };

        // Define the main collision rectangles for the pipes
        lowerRect = { x, containerY + static_cast<float>(screenHeight - height), static_cast<float>(width), static_cast<float>(height) };
        upperRect = { x, containerY, static_cast<float>(width), static_cast<float>(height) };

        // Define the extended collision rectangles for off-screen area coverage
        lowerExtendedRect = { x, containerY + static_cast<float>(screenHeight - height) + static_cast<float>(height), static_cast<float>(width), static_cast<float>(screenHeight) };
        upperExtendedRect = { x, containerY - static_cast<float>(screenHeight), static_cast<float>(width), static_cast<float>(screenHeight) };
    }

    void Update() {
        x -= speed * speedMultiplier;

        // Update collision rectangles as the pipe moves
        successRect.x = x + width / 2;
        lowerRect.x = x;
        upperRect.x = x;
        lowerExtendedRect.x = x;
        upperExtendedRect.x = x;

        // Define the player's rectangle for collision detection
        Rectangle playerRect = { player.x, player.y, 30, 30 };

        // Check for collision with both main and extended rectangles
        if (CheckCollisionRecs(lowerRect, playerRect) || CheckCollisionRecs(upperRect, playerRect) ||
            CheckCollisionRecs(lowerExtendedRect, playerRect) || CheckCollisionRecs(upperExtendedRect, playerRect)) {
            gameOver = true;
            screenShake = 10.0f;
        }

        // Check if the player successfully passes through the gap
        if (CheckCollisionRecs(successRect, playerRect)) {
            if (!disabled) {
                disabled = true;
                player.score++;
                speedMultiplier += 0.025f;
                screenShake = 2.0f;
            }
        }
    }

    void Draw() {
        // Draw bottom pipe
        DrawRectangle(x, containerY + (screenHeight - height), width, height, pipeColor);
        DrawRectangle(x, containerY + (screenHeight - height) + height, width, screenHeight, pipeColor); // Extend pipe to end of screen

        // Draw cap on bottom pipe
        DrawRectangle(x - 5, containerY + (screenHeight - height - 10), width + 10, 10, capColor);
        DrawCircle(x - 5, containerY + (screenHeight - height - 10), borderThickness, borderColor);
        DrawCircle(x + width + 5, containerY + (screenHeight - height - 10), borderThickness, borderColor);

        // Draw top pipe
        DrawRectangle(x, containerY, width, height, pipeColor);
        DrawRectangle(x, containerY - screenHeight, width, screenHeight, pipeColor); // Extend pipe to end of screen

        // Draw cap on top pipe
        DrawRectangle(x - 5, containerY + height, width + 10, 10, capColor);
        DrawCircle(x - 5, containerY + height + 10, borderThickness, borderColor);
        DrawCircle(x + width + 5, containerY + height + 10, borderThickness, borderColor);
    }

    bool IsOffScreen() const {
        return x + width < 0;  // Check if the pipe has moved off the left side of the screen
    }
};






/* ============================================== *
 * |              BACKGROUND DRAWING            | *
 * ============================================== */
void DrawCheckerboardBackground() {
    int checkerSize = 20;
    for (int y = 0; y < GetScreenHeight(); y += checkerSize) {
        for (int x = 0; x < GetScreenWidth(); x += checkerSize) {
            if ((x / checkerSize + y / checkerSize) % 2 == 0) {
                DrawRectangle(x, y, checkerSize, checkerSize, (Color){ 16, 16, 16, 255 });
            } else {
                DrawRectangle(x, y, checkerSize, checkerSize, BLACK);
            }
        }
    }
}

std::vector<Pipe> pipes;  // Vector to store multiple pipes

/* ============================================== *
 * |                 MAIN GAME LOGIC            | *
 * ============================================== */
void Game() {
    ApplyScreenShake();  // Apply shake effect to screenOffset

    if (started){
        // Spawn new pipes at regular intervals
        pipeSpawnTimer += GetFrameTime();
        if (pipeSpawnTimer >= pipeSpawnInterval / speedMultiplier) {
            pipes.push_back(Pipe());  // Add a new pipe to the vector
            pipeSpawnTimer = 0.0f;    // Reset the timer
        }

        // Remove pipes that have moved off-screen
        pipes.erase(std::remove_if(pipes.begin(), pipes.end(),
            [](const Pipe& pipe) { return pipe.IsOffScreen(); }),
            pipes.end());
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawCheckerboardBackground();

    if (!isInitialized) {
        isInitialized = true;
        player.Init();
    }

    // Define and apply the camera with the screen shake offset
    Camera2D camera = { 0 };
    camera.offset = screenOffset;  // Apply shake offset here
    camera.target = { 0, 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);  // Start drawing with camera

    if (!started) {
        DrawText("Press ENTER to start", screenWidth / 2 - MeasureText("Press ENTER to start", 32) / 2, 150, 32, WHITE);
    }

    // Draw and update game elements with screen shake active
    if (!gameOver) {
        for (auto& pipe : pipes) {
            pipe.Update();
            pipe.Draw();
        }

        player.Draw();
        player.Update();

        DrawText(std::to_string(player.score).c_str(), screenWidth / 2 - MeasureText(std::to_string(player.score).c_str(), 32) / 2, 50, 32, WHITE);
    }
    else {
        // Game over screen
        if (player.score > highScore) {
            highScore = player.score;
            SaveHighScore();
        }

        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6));
        DrawText("GAME OVER!", screenWidth / 2 - MeasureText("GAME OVER!", 48) / 2, 100, 48, RED);
        DrawText("Press ENTER to restart", screenWidth / 2 - MeasureText("Press ENTER to restart", 32) / 2, 175, 32, WHITE);

        DrawText(("SCORE: " + std::to_string(player.score)).c_str(), screenWidth / 2 - MeasureText(("SCORE: " + std::to_string(player.score)).c_str(), 24) / 2, screenHeight - 100, 24, GRAY);
        DrawText(("HIGH SCORE: " + std::to_string(highScore)).c_str(), screenWidth / 2 - MeasureText(("HIGH SCORE: " + std::to_string(highScore)).c_str(), 24) / 2, screenHeight - 70, 24, YELLOW);

        if (IsKeyPressed(KEY_ENTER)) {
            pipes.clear();
            player.Init();
            speedMultiplier = 1;
            isInitialized = false;
            gameOver = false;
        }
    }

    EndMode2D();  // End camera mode

    EndDrawing();
}


int main() {
    InitWindow(screenWidth, screenHeight, "Flappy Bird");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    SetTargetFPS(60);

    HideCursor();

    while (!WindowShouldClose()) {
        Game();
    }

    CloseWindow();

    return 0;
}
