#ifndef GAMELOADER_H
#define GAMELOADER_H

#include <vector>
#include <string>
#include <filesystem>

std::vector<std::string> LoadAvailableGames();
std::vector<std::string> LoadAvailableGameDescriptions();

void LaunchGame(const std::string& gameName);
void CloseGame();

#endif // GAMELOADER_H
