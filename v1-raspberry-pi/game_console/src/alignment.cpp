#include "raylib.h"

const int screenWidth = 640;
const int screenHeight = 480;

typedef enum {
    TOP_LEFT, TOP_CENTER, TOP_RIGHT,
    CENTER_LEFT, CENTER_CENTER, CENTER_RIGHT,
    BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT
} Alignment;

// Generalized position calculation function
Vector2 GetPosition(int elementWidth, int elementHeight, Alignment alignment, Vector2 offset) {
    Vector2 position = {0.0f, 0.0f};  // Initialize with floats explicitly
    switch (alignment) {
        case TOP_LEFT:
            position = (Vector2){ 10.0f, 10.0f };
            break;
        case TOP_CENTER:
            position = (Vector2){ (float)(screenWidth - elementWidth) / 2, 10.0f };
            break;
        case TOP_RIGHT:
            position = (Vector2){ (float)(screenWidth - elementWidth - 10), 10.0f };
            break;
        case CENTER_LEFT:
            position = (Vector2){ 10.0f, (float)(screenHeight - elementHeight) / 2 };
            break;
        case CENTER_CENTER:
            position = (Vector2){ (float)(screenWidth - elementWidth) / 2, (float)(screenHeight - elementHeight) / 2 };
            break;
        case CENTER_RIGHT:
            position = (Vector2){ (float)(screenWidth - elementWidth - 10), (float)(screenHeight - elementHeight) / 2 };
            break;
        case BOTTOM_LEFT:
            position = (Vector2){ 10.0f, (float)(screenHeight - elementHeight - 10) };
            break;
        case BOTTOM_CENTER:
            position = (Vector2){ (float)(screenWidth - elementWidth) / 2, (float)(screenHeight - elementHeight - 10) };
            break;
        case BOTTOM_RIGHT:
            position = (Vector2){ (float)(screenWidth - elementWidth - 10), (float)(screenHeight - elementHeight - 10) };
            break;
    }
    position.x += offset.x;
    position.y += offset.y;
    return position;
}

void DrawAlignedText(const char* text, int fontSize, Alignment alignment, Color color, Vector2 offset) {
    int textWidth = MeasureText(text, fontSize);
    Vector2 position = GetPosition(textWidth, fontSize, alignment, offset);
    DrawText(text, position.x, position.y, fontSize, color);
}
