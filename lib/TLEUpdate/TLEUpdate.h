
#ifndef TLE_UPDATE_H
#define TLE_UPDATE_H

#include <Arduino.h>
#include <vector>

struct SatelliteData {
    String name;
    String tle_line1;
    String tle_line2;
};

extern std::vector<SatelliteData> satellites;
extern int numSatellites;

void loadTLEs();
void updateTLEs(const char* host, const char* path);

#endif // TLE_UPDATE_H
