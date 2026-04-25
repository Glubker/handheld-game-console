#include "Row.h"

// Define the function
void DrawRoundedRectWithBorder(Rectangle rect, float roundness, Color bgColor, Color borderColor, float borderWidth) {
    DrawRectangleRounded(rect, roundness, 6, borderColor); // Draw the border first
    Rectangle innerRect = { rect.x + borderWidth, rect.y + borderWidth, rect.width - 2 * borderWidth, rect.height - 2 * borderWidth };
    DrawRectangleRounded(innerRect, roundness, 6, bgColor); // Draw the background rectangle inside the border
}
