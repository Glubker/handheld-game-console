#include "raylib.h"
#include <vector>

const int screenWidth = 800;
const int screenHeight = 480;
const int moveSpeed = 7;
const float gravity = 0.5f;
const float jumpSpeed = -10.0f;

// Coyote time variables
const float maxCoyoteTime = 0.1f; // 100 milliseconds
float coyoteTime = 0.0f;

bool isInitialized = false;

int level = 1;

class Player {
public:
    float x, y;
    float velocityY;
    bool isOnGround;

    Player() : x(0), y(0), velocityY(0), isOnGround(false) {}

    void Init() {
        x = 80;
        y = screenHeight - 120;
        velocityY = 0;
        isOnGround = false;
    }

    void ApplyGravity() {
        if (!isOnGround) {
            velocityY += gravity;
        }
    }

    void Update(const std::vector<Rectangle>& platforms, const std::vector<Rectangle>& finishCubes, const std::vector<Rectangle>& killCubes) {
        // Handle horizontal movement
        float newX = x;
        if (IsKeyDown(KEY_LEFT)) newX -= moveSpeed;
        if (IsKeyDown(KEY_RIGHT)) newX += moveSpeed;

        // Apply gravity
        ApplyGravity();

        // Update vertical position
        float newY = y + velocityY;

        // Check for horizontal collisions
        Rectangle playerRect = Rectangle{newX, y, 24, 24};
        for (const Rectangle& platform : platforms) {
            if (CheckCollisionRecs(playerRect, platform)) {
                if (newX < platform.x) {
                    newX = platform.x - 24; // Collision on the right side
                } else if (newX + 24 > platform.x + platform.width) {
                    newX = platform.x + platform.width; // Collision on the left side
                }
            }
        }

        // Update horizontal position
        x = newX;

        // Check for vertical collisions
        playerRect = Rectangle{x, newY, 24, 24};
        isOnGround = false;
        for (const Rectangle& platform : platforms) {
            if (CheckCollisionRecs(playerRect, platform)) {
                if (velocityY > 0 && y + 24 <= platform.y) {
                    newY = platform.y - 24; // Land on top of the platform
                    isOnGround = true;
                    velocityY = 0;
                } else if (velocityY < 0 && y >= platform.y + platform.height) {
                    newY = platform.y + platform.height; // Hit the bottom of the platform
                    velocityY = 0;
                }
            }
        }

        // Update vertical position
        y = newY;

        // Check for collision with finish cubes
        playerRect = Rectangle{x, y, 24, 24};
        for (const Rectangle& finishCube : finishCubes) {
            if (CheckCollisionRecs(playerRect, finishCube)) {
                level++;
                x = 80;  // Reset player position for the next level
                y = screenHeight - 120;
                break;  // Stop checking other finish cubes once one is touched
            }
        }

        // Check for collision with kill cubes
        playerRect = Rectangle{x, y, 24, 24};
        for (const Rectangle& killCube : killCubes) {
            if (CheckCollisionRecs(playerRect, killCube)) {
                x = 80;  // Reset player position
                y = screenHeight - 120;
                break;  // Stop checking other finish cubes once one is touched
            }
        }

        // Handle coyote time
        if (isOnGround) {
            coyoteTime = maxCoyoteTime;  // Reset coyote time when on ground
        } else {
            coyoteTime -= GetFrameTime(); // Decrease coyote time
        }

        // Jumping
        if (IsKeyDown(KEY_ENTER) && (isOnGround || coyoteTime > 0)) {
            velocityY = jumpSpeed;
            isOnGround = false;
            coyoteTime = 0; // Prevent multiple jumps during coyote time
        }

        // Prevent falling off the screen
        if (y > screenHeight - 24) {
            y = screenHeight - 24;
            x = 80;  // Reset player position
            y = screenHeight - 120;
            velocityY = 0;
        }

        if (x > screenWidth - 24) x = screenWidth - 24;
        if (x < 0) x = 0;
    }

    void Draw() {
        // Draw player with a border and a gradient/shadow effect
        DrawRectangle((int)x, (int)y, 24, 24, DARKGRAY); // Shadow
        DrawRectangle((int)x + 2, (int)y + 2, 20, 20, WHITE); // Main body
        DrawRectangleLines((int)x, (int)y, 24, 24, LIGHTGRAY); // Border
    }

    Rectangle GetPlayerRect() const {
        return Rectangle{x, y, 24, 24}; // Rectangle around the player
    }
};

Player player;

void DrawCheckerboardBackground() {
    int checkerSize = 20;
    for (int y = 0; y < GetScreenHeight(); y += checkerSize) {
        for (int x = 0; x < GetScreenWidth(); x += checkerSize) {
            if ((x / checkerSize + y / checkerSize) % 2 == 0) {
                DrawRectangle(x, y, checkerSize, checkerSize, (Color){16, 16, 16, 255});
            } else {
                DrawRectangle(x, y, checkerSize, checkerSize, BLACK);
            }
        }
    }
}

void DrawMap(std::vector<Rectangle>& platforms) {
    platforms.clear();  // Clear previous platforms
    switch(level) {
        case 1:
            platforms.push_back(Rectangle{0, screenHeight - 28, screenWidth, 28}); // Ground level
            platforms.push_back(Rectangle{100, screenHeight - 100, 100, 20}); // Small platform near the left
            platforms.push_back(Rectangle{250, screenHeight - 160, 150, 20}); // Higher platform in the middle
            platforms.push_back(Rectangle{450, screenHeight - 220, 100, 20}); // Higher platform leading to right
            platforms.push_back(Rectangle{600, screenHeight - 280, 150, 20}); // Final platform near the top-right
            break;
        case 2:
            platforms.push_back(Rectangle{0, screenHeight - 28, screenWidth, 28}); // Ground level
            platforms.push_back(Rectangle{500, screenHeight - 100, 50, 20});
            platforms.push_back(Rectangle{250, screenHeight - 175, 75, 20});
            platforms.push_back(Rectangle{50, screenHeight - 250, 50, 20});
            platforms.push_back(Rectangle{150, screenHeight - 325, 100, 20});
            platforms.push_back(Rectangle{450, screenHeight - 400, 200, 20});
            platforms.push_back(Rectangle{screenWidth - 50, screenHeight - 300, 50, 20});
            break;
        case 3:
            platforms.push_back(Rectangle{0, screenHeight - 28, screenWidth, 28}); // Ground level
            platforms.push_back(Rectangle{150, screenHeight - 46, 100, 20}); // Small platform near the left
            platforms.push_back(Rectangle{350, screenHeight - 120, 75, 20}); // Small platform in the middle
            platforms.push_back(Rectangle{200, screenHeight - 200, 100, 20}); // Higher platform slightly left
            platforms.push_back(Rectangle{50, screenHeight - 280, 35, 20}); // Platform near the right
            platforms.push_back(Rectangle{250, screenHeight - 350, 35, 20}); // Platform near the right
            platforms.push_back(Rectangle{450, screenHeight - 400, 35, 20}); // Platform near the right
            break;
        case 4:
            platforms.push_back(Rectangle{0, screenHeight - 28, screenWidth, 28});
            platforms.push_back(Rectangle{screenWidth - 100, screenHeight - 100, 100, 20});
            platforms.push_back(Rectangle{screenWidth - 300, screenHeight - 175, 100, 20});
            platforms.push_back(Rectangle{screenWidth - 360, screenHeight - 250, 30, 30});
            platforms.push_back(Rectangle{screenWidth - 450, screenHeight - 300, 30, 30});
            platforms.push_back(Rectangle{0, screenHeight - 300, 200, 30});
            break;
        case 5:
            platforms.push_back(Rectangle{50, screenHeight - 60, 150, 20}); // Bottom-left platform
            platforms.push_back(Rectangle{150, screenHeight - 140, 15, 15}); // Higher platform in the middle
            platforms.push_back(Rectangle{250, screenHeight - 220, 15, 15}); // Higher platform to the right
            platforms.push_back(Rectangle{350, screenHeight - 300, 15, 15}); // Final platform in the middle
            platforms.push_back(Rectangle{600, screenHeight - 130, 50, 15}); // Final platform in the middle
            platforms.push_back(Rectangle{750, screenHeight - 220, 15, 15}); // Final platform in the middle
            platforms.push_back(Rectangle{650, screenHeight - 300, 15, 15}); // Final platform in the middle
            break;
            break;
        default:
            platforms.push_back(Rectangle{0, screenHeight - 28, screenWidth, 28}); // Default ground level
    }

    for (const Rectangle& platform : platforms) {
        // Draw platform with a border and a textured pattern
        DrawRectangleRec(platform, (Color){0, 0, 0, 255});
        DrawRectangleLinesEx(platform, 2, GRAY); // Border
    }
}

void DrawFinish(std::vector<Rectangle>& finishCubes) {
    finishCubes.clear();  // Clear previous finish cubes
    switch(level) {
        case 1:
            finishCubes.push_back(Rectangle{700, screenHeight - 300, 20, 20});
            break;
        case 2:
            finishCubes.push_back(Rectangle{screenWidth - 20, screenHeight - 320, 20, 20});
            break;
        case 3:
            finishCubes.push_back(Rectangle{750, screenHeight - 250, 20, 20});
            break;
        case 4:
            finishCubes.push_back(Rectangle{0, screenHeight - 320, 20, 20});
            break;
        case 5:
            finishCubes.push_back(Rectangle{screenWidth - 50, screenHeight - 400, 20, 20});
            break;
        default:
            break;
    }

    for (const Rectangle& finishCube : finishCubes) {
        // Draw finish cube
        DrawRectangleRec(finishCube, WHITE);
        DrawRectangleLinesEx(finishCube, 2, GRAY); // Border
    }
}

void DrawKillCube(std::vector<Rectangle>& killCubes) {
    killCubes.clear();  // Clear previous finish cubes
    switch(level) {
        case 4:
            killCubes.push_back(Rectangle{200, screenHeight - 48, 150, 20});
            killCubes.push_back(Rectangle{500, screenHeight - 48, 150, 20});

            killCubes.push_back(Rectangle{screenWidth - 330, screenHeight - 250, 30, 30});
            killCubes.push_back(Rectangle{screenWidth - 420, screenHeight - 300, 30, 30});

            killCubes.push_back(Rectangle{120, screenHeight - 350, 20, 50});
            killCubes.push_back(Rectangle{50, screenHeight - 350, 20, 50});
            break;
        case 5:
            killCubes.push_back(Rectangle{600, screenHeight - 350, 15, 50});
            break;
        default:
            break;
    }

    for (const Rectangle& killCube : killCubes) {
        // Draw kill cube
        DrawRectangleRec(killCube, RED);
        DrawRectangleLinesEx(killCube, 2, RED); // Border
    }
}



void Platformer() {
    ClearBackground(BLACK);
    DrawCheckerboardBackground();

    static std::vector<Rectangle> platforms; // Static so it keeps the platforms' positions between frames
    static std::vector<Rectangle> finishCubes; // Static so it keeps the platforms' positions between frames
    static std::vector<Rectangle> killCubes; // Static so it keeps the platforms' positions between frames

    if (!isInitialized) {
        player.Init();
        isInitialized = true;
    }

    DrawMap(platforms);
    DrawFinish(finishCubes);
    DrawKillCube(killCubes);
    player.Update(platforms, finishCubes, killCubes);
    player.Draw();

    if (level == 6) {
        DrawText("You beat the game!", screenWidth / 2 - MeasureText("You beat the game!", 38) / 2, 150, 38, WHITE);
    }
}

int main() {
    InitWindow(screenWidth, screenHeight, "Platformer");
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

    SetTargetFPS(60);
    HideCursor();

    while (!WindowShouldClose()) {
        BeginDrawing();
        Platformer();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
