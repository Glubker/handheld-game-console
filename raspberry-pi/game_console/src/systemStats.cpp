//
// Created by Gustav Lübker on 27/05/2024.
//

#include "systemStats.h"
#include <string>

float SystemStats::GetCPUUsage() {
    static long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    long long totalUser, totalUserLow, totalSys, totalIdle, total;
    std::ifstream file("/proc/stat");
    std::string line;

    if (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string cpu;
        ss >> cpu >> totalUser >> totalUserLow >> totalSys >> totalIdle;
        if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow || totalSys < lastTotalSys || totalIdle < lastTotalIdle) {
            // Overflow detection. Just skip this value.
            return -1.0;
        } else {
            total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) + (totalSys - lastTotalSys);
            float percent = total;
            total += (totalIdle - lastTotalIdle);
            percent /= total;
            percent *= 100;
            lastTotalUser = totalUser;
            lastTotalUserLow = totalUserLow;
            lastTotalSys = totalSys;
            lastTotalIdle = totalIdle;
            return percent;
        }
    }
    return -1.0;
}

float SystemStats::GetMemoryUsageMB() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    long long totalMemory = 0, freeMemory = 0;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key;
        long long value;
        std::string unit;
        ss >> key >> value >> unit;

        if (key == "MemTotal:") {
            totalMemory = value;
        } else if (key == "MemAvailable:") {
            freeMemory = value;
            break;
        }
    }

    long long usedMemory = totalMemory - freeMemory;
    return usedMemory / 1024.0;
}

float SystemStats::GetStorageUsageGB() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return -1.0;
    }
    long long totalSpace = stat.f_blocks * stat.f_frsize;
    long long freeSpace = stat.f_bfree * stat.f_frsize;
    long long usedSpace = totalSpace - freeSpace;
    return usedSpace / 1024.0 / 1024.0 / 1024.0;
}

float SystemStats::GetCPUTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    float temp;
    file >> temp;
    return temp / 1000.0f; // Convert to Celsius
}