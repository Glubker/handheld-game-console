#include "raylib.h"
// #include "game.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm> // Include this for std::remove_if

const int screenWidth = 800;
const int screenHeight = 480;
const int paddleSpeed = 10;
const float initialCpuSpeedFactor = 0.65f; // CPU speed factor relative to ball speed
const float ballInitialSpeed = 7.0f; // Slightly faster initial ball speed
const float ballSpeedIncrement = 0.5f; // Much higher increment for ball speed increase
const float maxAngleVariation = 10.0f * PI / 180.0f; // Max angle variation from 45 degrees

bool isInitialized = false;
int playerScore = 0;
int cpuScore = 0;
float trailOpacity = 0.5f;
float ballSpeed = ballInitialSpeed; // Current ball speed
float maxBallSpeed = 30.0f; // Maximum ball speed
float particleLifetime = 0.5f; // Lifetime of particles

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
        lifetime = particleLifetime;
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
            DrawCircle(x, y, 2, color);
        }
    }
};

class Trail {
public:
    float x, y;
    float lifetime;

    Trail(float startX, float startY) {
        x = startX;
        y = startY;
        lifetime = 0.3f;
    }

    void Update(float deltaTime) {
        lifetime -= deltaTime;
    }

    void Draw() {
        if (lifetime > 0) {
            Color trailColor = Fade(WHITE, lifetime * trailOpacity);
            DrawCircle(x, y, 4, trailColor);
        }
    }
};

std::vector<Particle> particles;
std::vector<Trail> trails;

void AddGoalParticles(Color particleColor) {
    for (int i = 0; i < 100; i++) {
        particles.emplace_back(screenWidth / 2, GetRandomValue(0, screenHeight), particleColor);
    }
}

class Ball {
private:
    float curveFactor = 0.1f; // Factor to control the curve

public:
    float x = 0;
    float y = 0;
    float xDirection = 1;
    float yDirection = 1;

    void Init() {
        x = screenWidth / 2 - 8;
        y = screenHeight / 2 - 8;
        ballSpeed = ballInitialSpeed; // Reset ball speed
        float angle = GetRandomValue(0, 1) == 0 ? PI / 4 : -PI / 4; // Start at 45 degrees in either direction
        xDirection = cos(angle);
        yDirection = sin(angle);
    }

    void Update() {
        x += ballSpeed * xDirection;
        y += ballSpeed * yDirection;

        if (y <= 0 || y >= screenHeight - 8) {
            yDirection *= -1;
            y += yDirection * ballSpeed * 0.5f; // Bounce back slightly
        }

        if (x >= screenWidth - 8) { // Goal for CPU
            cpuScore++;
            Init();
            isInitialized = false;
            AddGoalParticles(WHITE); // Add goal particles
        }
        if (x <= 8) { // Goal for Player
            playerScore++;
            Init();
            isInitialized = false;
            AddGoalParticles(WHITE); // Add goal particles
        }
        trails.emplace_back(x, y);
    }

    void Draw() {
        for (auto& trail : trails) {
            trail.Draw();
        }
        DrawCircleLines(x, y, 8, WHITE);
    }

    void Reflect(float hitFactor, bool isPlayer) {
        float baseAngle = isPlayer ? PI / 4 : -PI / 4; // 45 degrees in respective direction
        float angleVariation = hitFactor * maxAngleVariation;
        float angle = baseAngle + angleVariation;
        xDirection = cos(angle) * (isPlayer ? -1 : 1);
        yDirection = sin(angle);
    }

    void IncreaseSpeed(float increment) {
        if (ballSpeed < maxBallSpeed) {
            ballSpeed += increment;
            if (ballSpeed > maxBallSpeed) ballSpeed = maxBallSpeed; // Cap at max speed
        }
    }
};


class Player {
public:
    float x = 0;
    float y = 0;

    void Init() {
        x = screenWidth - 24;
        y = screenHeight / 2 - 32;
    }

    void Update() {
        if (IsKeyDown(KEY_UP)) y -= paddleSpeed;
        if (IsKeyDown(KEY_DOWN)) y += paddleSpeed;
        if (y < 0) y = 0;
        if (y > screenHeight - 64) y = screenHeight - 64;
    }

    void Draw() {
        DrawRectangleLines(x, y, 8, 64, WHITE);
    }
};

class Cpu {
public:
    float x = 0;
    float y = 0;

    void Init() {
        x = 16;
        y = screenHeight / 2 - 32;
    }

    void Update(float ballY, float ballSpeed) {
        float cpuSpeed = ballSpeed * initialCpuSpeedFactor; // CPU speed is always slightly slower than the ball speed
        if (ballY > y + 32) y += cpuSpeed;
        if (ballY < y + 32) y -= cpuSpeed;
        if (y < 0) y = 0;
        if (y > screenHeight - 64) y = screenHeight - 64;
    }

    void Draw() {
        DrawRectangleLines(x, y, 8, 64, WHITE);
    }
};

Ball ball;
Player player;
Cpu cpu;

void ResetPongGame() {
    isInitialized = false;
    playerScore = 0;
    cpuScore = 0;
    particles.clear();
    trails.clear();
}

void DrawScores() {
    std::string playerScoreStr = std::to_string(playerScore);
    std::string cpuScoreStr = std::to_string(cpuScore);

    DrawText(playerScoreStr.c_str(), screenWidth / 2 + 20, 20, 40, WHITE);
    DrawText(cpuScoreStr.c_str(), screenWidth / 2 - 40 - MeasureText(cpuScoreStr.c_str(), 40), 20, 40, WHITE);
}

void AddParticles(float x, float y, Color particleColor) {
    for (int i = 0; i < 30; i++) {
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

void UpdateTrails(float deltaTime) {
    for (auto& trail : trails) {
        trail.Update(deltaTime);
    }
    trails.erase(std::remove_if(trails.begin(), trails.end(),
        [](const Trail& t) { return t.lifetime <= 0; }), trails.end());
}

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

void DrawNet() {
    for (int i = 0; i < GetScreenHeight(); i += 10) {
        DrawRectangle(GetScreenWidth() / 2 - 1, i, 2, 5, GRAY);
    }
}

void PingPong() {

    ClearBackground(BLACK); // Changed background color
    DrawCheckerboardBackground();

    DrawNet();

    if (!isInitialized) {
        ball.Init();
        player.Init();
        cpu.Init();
        isInitialized = true;
    }

    if (CheckCollisionCircleRec(Vector2{ ball.x, ball.y }, 8, Rectangle{ player.x, player.y, 8, 64 })) {
        float hitFactor = (ball.y - player.y - 32) / 32.0f; // Calculate hit factor
        ball.Reflect(hitFactor, true); // Reflect with variation
        ball.IncreaseSpeed(ballSpeedIncrement); // Increase ball speed
        AddParticles(ball.x, ball.y, WHITE);
    }
    if (CheckCollisionCircleRec(Vector2{ ball.x, ball.y }, 8, Rectangle{ cpu.x, cpu.y, 8, 64 })) {
        float hitFactor = (ball.y - cpu.y - 32) / 32.0f; // Calculate hit factor
        ball.Reflect(hitFactor, false); // Reflect with variation
        ball.IncreaseSpeed(ballSpeedIncrement); // Increase ball speed
        AddParticles(ball.x, ball.y, WHITE);
    }

    ball.Update();
    ball.Draw();

    cpu.Update(ball.y, ballSpeed);
    cpu.Draw();

    player.Update();
    player.Draw();

    DrawScores();

    UpdateParticles(GetFrameTime());
    DrawParticles();

    UpdateTrails(GetFrameTime());
}

int main() {
    InitWindow(screenWidth, screenHeight, "Pong Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    SetTargetFPS(60);

    HideCursor();

    while (!WindowShouldClose()) {
        BeginDrawing();
        PingPong();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
