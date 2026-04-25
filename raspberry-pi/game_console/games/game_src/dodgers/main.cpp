#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm> // Include this for std::remove_if
#include <fstream> // Include for file operations

int highScore = 0;

// Function to load the high score from a file
void LoadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

// Function to save the high score to a file
void SaveHighScore() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}


const int screenWidth = 800;
const int screenHeight = 480;
int playerSpeed = 10;
const float baseObstacleSpeed = 200.0f;
const float obstacleGap = 200.0f;
const float baseCoinSpeed = 200.0f;

float speedMultiplier = 1.0f;

bool isInitialized = false;
bool isGameover = false;

// Screen shake variables
float screenShake = 0.0f;
Vector2 screenOffset = { 0, 0 };

// Particle structure
class Particle {
public:
    float x, y;
    float speedX, speedY;
    float lifetime;
    Color color;

    Particle(float startX, float startY, Color particleColor) {
        x = startX;
        y = startY;
        speedX = GetRandomValue(-50, 50) / 25.0f;
        speedY = GetRandomValue(-50, 50) / 25.0f;
        lifetime = 0.5f; // Set particle lifetime
        color = particleColor;
    }

    void Update(float deltaTime) {
        x += speedX;
        y += speedY;
        lifetime -= deltaTime;
        color.a = (unsigned char)(lifetime * 255);
    }

    void Draw() {
        if (lifetime > 0) {
            DrawCircle(x, y, 3, Fade(color, 0.5f));  // Slightly smaller glow
            DrawCircle(x, y, 2, color);              // Slightly smaller main particle
        }
    }
};


std::vector<Particle> particles;

void CreateExplosion(float x, float y, Color particleColor, int amount = 50) {
    for (int i = 0; i < amount; i++) {
        particles.emplace_back(x, y, particleColor);
    }
}

void UpdateParticles(float deltaTime) {
    for (auto& particle : particles) {
        particle.Update(deltaTime);
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.lifetime <= 0; }), particles.end());
}

void DrawParticles() {
    for (auto& particle : particles) {
        particle.Draw();
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
 * |                    PLAYER                  | *
 * ============================================== */
class Player {
public:
    float x = 0;
    float y = 0;
    int score = 0;

    void Init() {
        x = screenWidth / 2;
        y = screenHeight - 48;
    }

    void Update() {
        if (IsKeyDown(KEY_LEFT)) x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT)) x += playerSpeed;
        if (x < 0) x = 0;
        if (x > screenWidth - 32) x = screenWidth - 32;
    }

    void Draw() {
        DrawRectangle(x, y, 32, 32, WHITE);
        DrawRectangleLines(x + 2, y + 2, 28, 28, BLACK);
    }
};

Player player;

/* ============================================== *
 * |                    COINS                   | *
 * ============================================== */
void SpawnCoins(std::vector<Rectangle>& coins) {
    coins.clear();

    for (int i = 0; i < 10; i++) {
        float yOffset = (float)(i * -obstacleGap - screenHeight);
        coins.push_back({ (float)GetRandomValue(50, screenWidth - 50), yOffset - obstacleGap / 2.0f, 20.0f, 20.0f });
    }
}

void UpdateCoins(std::vector<Rectangle>& coins, float effectiveCoinSpeed) {
    for (auto& coin : coins) {
        coin.y += effectiveCoinSpeed * GetFrameTime();
        if (coin.y > screenHeight) {
            coin.y = (float)(GetRandomValue(-obstacleGap, -obstacleGap / 2));
            coin.x = (float)GetRandomValue(50, screenWidth - 50);
        }

        /* Collect Coin */
        Rectangle playerRect = Rectangle{player.x, player.y, 32, 32};
        if (CheckCollisionRecs(coin, playerRect)) {
            CreateExplosion(coin.x + coin.width / 2.0f, coin.y + coin.height / 2.0f, GOLD, 50);

            coin.y = (float)(GetRandomValue(-obstacleGap, -obstacleGap / 2));
            coin.x = (float)GetRandomValue(50, screenWidth - 50);

            player.score += 10;

            // Small screen shake for coin collection
            screenShake = 5.0f;
        }
    }
}

void DrawCoins(std::vector<Rectangle>& coins) {
    for (const auto& coin : coins) {
        // Draw a glow effect behind the coin
        DrawCircleV(Vector2{ coin.x + coin.width / 2.0f, coin.y + coin.height / 2.0f }, coin.width / 1.5f, Fade(YELLOW, 0.3f));
        // Draw the coin itself
        DrawCircleV(Vector2{ coin.x + coin.width / 2.0f, coin.y + coin.height / 2.0f }, coin.width / 2.0f, GOLD);
        // Add a white shine effect on top
        DrawCircleV(Vector2{ coin.x + coin.width / 2.0f - 3, coin.y + coin.height / 2.0f - 3 }, coin.width / 4.0f, WHITE);
    }
}

/* ============================================== *
 * |                  OBSTACLES                 | *
 * ============================================== */
void SpawnObstacles(std::vector<Rectangle>& obstacles) {
    obstacles.clear();

    for (int i = 0; i < 10; i++) {
        float yOffset = (float)(i * -obstacleGap - screenHeight);
        obstacles.push_back({ (float)GetRandomValue(0, screenWidth - 100), yOffset, 100.0f, 30.0f });
    }
}

void UpdateObstacles(std::vector<Rectangle>& obstacles, float effectiveObstacleSpeed) {
    for (auto& obstacle : obstacles) {
        obstacle.y += effectiveObstacleSpeed * GetFrameTime();
        if (obstacle.y > screenHeight) {
            obstacle.y = (float)(GetRandomValue(-obstacleGap, -obstacleGap / 2));
            obstacle.x = (float)GetRandomValue(0, screenWidth - (int)obstacle.width);
        }

        /* Gameover */
        Rectangle playerRect = Rectangle{player.x, player.y, 32, 32};
        if (CheckCollisionRecs(obstacle, playerRect)) {
            CreateExplosion(player.x + 16, player.y + 16, RED, 100); // Larger explosion on collision

            obstacle.x = (float)GetRandomValue(0, screenWidth - 50);
            obstacle.y = (float)(GetRandomValue(0, screenWidth - 50));

            isGameover = true;
            speedMultiplier = 0;
            playerSpeed = 0;


            // Trigger larger screen shake on collision
            screenShake = 10.0f;
        }
    }
}

void DrawObstacles(std::vector<Rectangle>& obstacles) {
    for (const auto& obstacle : obstacles) {
        // Draw a glow effect behind the obstacle
        DrawRectangle(obstacle.x - 5, obstacle.y - 5, obstacle.width + 10, obstacle.height + 10, Fade(RED, 0.3f));
        // Draw the obstacle itself
        DrawRectangle(obstacle.x, obstacle.y, obstacle.width, obstacle.height, MAROON);
        // Add a border to the obstacle
        DrawRectangleLines(obstacle.x, obstacle.y, obstacle.width, obstacle.height, RED);
    }
}

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

/* ============================================== *
 * |                 MAIN GAME LOGIC            | *
 * ============================================== */
void Game() {
    ApplyScreenShake();

    BeginDrawing();
    ClearBackground(BLACK);
    DrawCheckerboardBackground();

    static std::vector<Rectangle> coins;
    static std::vector<Rectangle> obstacles;

    if (!isInitialized) {
        LoadHighScore();  // Load the high score when the game initializes
        player.Init();
        SpawnCoins(coins);
        SpawnObstacles(obstacles);
        isInitialized = true;
    }

    // Calculate the effective speeds for this frame
    float effectiveCoinSpeed = baseCoinSpeed * speedMultiplier;
    float effectiveObstacleSpeed = baseObstacleSpeed * speedMultiplier;

    BeginMode2D((Camera2D){screenOffset, {0, 0}, 0, 1});  // Apply screen shake
    player.Update();
    player.Draw();

    DrawCoins(coins);
    UpdateCoins(coins, effectiveCoinSpeed);

    DrawObstacles(obstacles);
    UpdateObstacles(obstacles, effectiveObstacleSpeed);

    UpdateParticles(GetFrameTime());
    DrawParticles();  // Draw particles

    DrawText(std::to_string(player.score).c_str(), screenWidth / 2 - MeasureText(std::to_string(player.score).c_str(), 32) / 2, 50, 32, WHITE);

    EndMode2D();

    if (!isGameover) {
        // Update the speed multiplier based on the score
        speedMultiplier = 1.0f + player.score / 1000.0f;
    } else {
        // Check and save high score
        if (player.score > highScore) {
            highScore = player.score;
            SaveHighScore();
        }

        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6));

        DrawText("GAME OVER!", screenWidth / 2 - MeasureText("GAME OVER!", 48) / 2, 100, 48, RED);
        DrawText("Press ENTER to restart", screenWidth / 2 - MeasureText("Press ENTER to restart", 32) / 2, 175, 32, WHITE);

        DrawText(("SCORE: " + std::to_string(player.score)).c_str(), screenWidth / 2 - MeasureText(("SCORE: " + std::to_string(player.score)).c_str(), 24) / 2, screenHeight - 100, 24, GRAY);

        // Display high score
        DrawText(("HIGH SCORE: " + std::to_string(highScore)).c_str(), screenWidth / 2 - MeasureText(("HIGH SCORE: " + std::to_string(highScore)).c_str(), 24) / 2, screenHeight - 70, 24, YELLOW);

        if (IsKeyPressed(KEY_ENTER)) {
            player.score = 0;
            speedMultiplier = 1;
            playerSpeed = 10;
            isInitialized = false;
            isGameover = false;
            particles.clear();  // Clear particles on restart
        }
    }

    EndDrawing();
}


int main() {
    InitWindow(screenWidth, screenHeight, "Dodgers");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    SetTargetFPS(60);

    HideCursor();

    while (!WindowShouldClose()) {
        Game();
    }

    CloseWindow();

    return 0;
}
