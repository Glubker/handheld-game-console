// MenuButton.cpp
#include "MenuButton.h"
#include "../CustomColors.h" // Include the custom colors
#include <string>

// Draw Retro style menu button
void DrawRetroMenuButton(const char *text, Rectangle rect, bool selected) {
    Color bgColor = selected ? WHITE : BLACK;
    Color borderColor = selected ? BLACK : WHITE;
    Color textColor = selected ? BLACK : WHITE;

    DrawRectangleRec(rect, bgColor);
    DrawRectangleLinesEx(rect, 2, borderColor);

    int textWidth = MeasureText(text, 20);
    Vector2 textPosition = { rect.x + rect.width / 2.0f - textWidth / 2.0f, rect.y + rect.height / 2.0f - 10.0f };
    DrawText(text, textPosition.x, textPosition.y, 20, textColor);
}

// Draw Modern style menu button
void DrawModernMenuButton(const char *text, const Rectangle rect, bool selected, const Font &font) {
    static float scale = 1.0f;
    static float targetScale = 1.0f;

    if (selected) {
        targetScale = 1.1f;  // Scale up when selected
    } else {
        targetScale = 1.0f;  // Scale down when not selected
    }

    // Adjust for scale
    float scaledWidth = rect.width * scale;
    float scaledHeight = rect.height * scale;
    float offsetX = (rect.width - scaledWidth) / 2.0f;
    float offsetY = (rect.height - scaledHeight) / 2.0f;

    Rectangle scaledRect = {
        rect.x + offsetX,
        rect.y + offsetY,
        scaledWidth,
        scaledHeight
    };

    float roundness = 0.15f;  // Adjust roundness (0.0f to 1.0f)
    Color borderColor = selected ? CUSTOM_BLUE : CUSTOM_BORDER_COLOR;
    Color glowColor = Fade(CUSTOM_BLUE, selected ? 0.6f : 0.0f);

    // Adjust border width
    float borderWidth = 2.0f;

    // Create a rectangle for the outer border
    Rectangle outerRect = scaledRect;

    // Create a smaller rectangle for the image to simulate a border
    Rectangle innerRect = {
        scaledRect.x + borderWidth,
        scaledRect.y + borderWidth,
        scaledRect.width - 2 * borderWidth,
        scaledRect.height - 2 * borderWidth
    };

    // Create a glow effect
    if (selected) {
        DrawRectangleRounded(outerRect, roundness, 6, glowColor);
    }

    // Draw the button border with rounded corners
    #if defined(__APPLE__)
        DrawRectangleRoundedLines(outerRect, roundness, 6, borderWidth, borderColor);
    #else
        DrawRectangleRoundedLines(outerRect, roundness, 6, borderColor);
    #endif

    // Draw the inner background rectangle
    DrawRectangleRounded(innerRect, roundness, 6, CUSTOM_SECONDARY_BACKGROUND);

    // Calculate text position to be centered
    Vector2 textSize = MeasureTextEx(font, text, 20, 2);
    Vector2 textPosition = { innerRect.x + innerRect.width / 2.0f - textSize.x / 2.0f, innerRect.y + innerRect.height / 2.0f - textSize.y / 2.0f };

    DrawTextEx(font, text, {textPosition.x, textPosition.y}, 20, 2, CUSTOM_TEXT_COLOR);
}

// Draw Menu button (Retro or Modern based on theme)
void DrawMenuButton(const char *text, Rectangle rect, bool selected, bool useModernTheme, const Font &font) {
    if (useModernTheme) {
        DrawModernMenuButton(text, rect, selected, font);
    } else {
        DrawRetroMenuButton(text, rect, selected);
    }
}

// Draw Main Menu button with icon
void DrawMainMenuButton(const char *text, Rectangle rect, bool selected, const std::string& iconPath, Font font, Texture2D iconTexture) {
    // Calculate scaled size and maintain the text position fixed
    float scale = selected ? 1.1f : 1.0f;
    float scaledWidth = rect.width * scale;
    float scaledHeight = rect.height * scale;
    float offsetX = (rect.width - scaledWidth) / 2.0f;
    float offsetY = (rect.height - scaledHeight) / 2.0f;

    Rectangle scaledRect = {
        rect.x + offsetX,
        rect.y + offsetY,
        scaledWidth,
        scaledHeight
    };

    float roundness = 0.15f;  // Adjust roundness
    Color borderColor = selected ? CUSTOM_BLUE : CUSTOM_BORDER_COLOR;

    // Draw the button border with rounded corners
    #if defined(__APPLE__)
        DrawRectangleRoundedLines(scaledRect, roundness, 6, 2.0f, borderColor);
    #else
        DrawRectangleRoundedLines(scaledRect, roundness, 6, borderColor);
    #endif

    // Draw the icon
    if (iconTexture.id > 0) { // Ensure iconTexture is valid
        Rectangle iconRect = {
            scaledRect.x + (scaledRect.width - iconTexture.width) / 2,
            scaledRect.y + (scaledRect.height - iconTexture.height) / 2 - 10, // Center the icon
            (float)iconTexture.width,
            (float)iconTexture.height
        };
        DrawTexturePro(iconTexture,
                       Rectangle{0, 0, static_cast<float>(iconTexture.width), static_cast<float>(iconTexture.height)},
                       iconRect,
                       Vector2{0, 0}, 0.0f, WHITE);
    }

    // Draw the text
    Vector2 textSize = MeasureTextEx(font, text, 20, 2);
    Vector2 textPosition = { rect.x + rect.width / 2.0f - textSize.x / 2.0f, rect.y + rect.height - 30.0f };

    DrawTextEx(font, text, {textPosition.x, textPosition.y}, 20, 2, CUSTOM_TEXT_COLOR);
}
