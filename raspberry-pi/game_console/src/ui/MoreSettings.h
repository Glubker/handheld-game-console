#ifndef MORE_SETTINGS_H
#define MORE_SETTINGS_H

#include "raylib.h"
#include "Row.h" // Include this to use DrawRoundedRectWithBorder
#include <cstddef>  // Include this to define size_t

extern bool showNetworkManager;
extern bool showKeyboard;
extern char *currentField;

void DrawMoreSettingsUI(int &selectedIndex, Font font, Texture2D backIcon, char* username);
void DrawOnScreenKeyboard(Font font);
void HandleTextInput(char *buffer, size_t bufferSize, int type, Font font);  // Add bufferSize parameter here
void HandleNetworkManagerInput(int &selectedIndex, char* username);

#endif // MORE_SETTINGS_H
