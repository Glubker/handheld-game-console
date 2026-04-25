#include "gameLoader.h"
#include <dirent.h>  // for opendir, readdir, closedir
#include <iostream>

std::vector<std::string> LoadAvailableGames() {
    std::vector<std::string> gameNames;
    //std::string gamesPath = "/home/pi/game_console/games";
    std::string gamesPath = "games";

    DIR *dir = opendir(gamesPath.c_str());
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {  // Check if it is a regular file
                gameNames.push_back(entry->d_name);
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Failed to open directory: " << gamesPath << std::endl;
    }

    return gameNames;
}

std::vector<std::string> LoadAvailableGameDescriptions() {
    std::vector<std::string> gameDescriptions;
    std::string gamesPath = "/home/pi/game_console/games";

    DIR *dir = opendir(gamesPath.c_str());
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {  // Check if it is a regular file
                gameDescriptions.push_back("Launch " + std::string(entry->d_name));
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Failed to open directory: " << gamesPath << std::endl;
    }

    return gameDescriptions;
}
