//
// Created by Gustav Lübker on 27/05/2024.
//

#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>

class SystemStats {
public:
    float GetCPUUsage();
    float GetMemoryUsageMB();
    float GetStorageUsageGB();
    float GetCPUTemperature();
};

#endif // SYSTEM_STATS_H
