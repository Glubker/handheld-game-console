#include "Background.h"
#include "../CustomColors.h" // Include custom colors

const int screenWidth = 800;
const int screenHeight = 480;
static int myFrameCounter; // Static to limit scope to this file

void DrawRetroBackground() {
    // Retro style background
    for (int i = 0; i < screenWidth; i += 20) {
        DrawLine(i, 0, i, screenHeight, (myFrameCounter / 10 + i / 20) % 2 == 0 ? DARKGRAY : GRAY);
    }
    for (int j = 0; j < screenHeight; j += 20) {
        DrawLine(0, j, screenWidth, j, (myFrameCounter / 10 + j / 20) % 2 == 0 ? DARKGRAY : GRAY);
    }
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.3f));
}

void DrawModernBackground() {
    // Modern style background
    DrawRectangle(0, 0, screenWidth, screenHeight, CUSTOM_BACKGROUND);
}

void DrawBackground(bool useModernTheme, int frameCount) {
    myFrameCounter = frameCount; // Update frame counter

    if (useModernTheme) {
        DrawModernBackground();
    } else {
        DrawRetroBackground();
    }
}
