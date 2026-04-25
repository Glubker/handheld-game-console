#ifndef MENU_BUTTON_H
#define MENU_BUTTON_H

#include <string>

#include "raylib.h"

// Function prototypes
void DrawRetroMenuButton(const char *text, Rectangle rect, bool selected);
void DrawModernMenuButton(const char *text, Rectangle rect, bool selected, const Font &font);
void DrawMenuButton(const char *text, Rectangle rect, bool selected, bool useModernTheme, const Font &font);
void DrawMainMenuButton(const char *text, Rectangle rect, bool selected, const std::string& iconPath, Font font, Texture2D iconTexture);

#endif // MENU_BUTTON_H
