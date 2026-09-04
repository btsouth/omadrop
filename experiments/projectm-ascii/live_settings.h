#pragma once

#include <string>

bool loadAsciiEnabled();
void saveAsciiEnabled(bool enabled);
unsigned int loadSyncDelay(const std::string& sink);
void saveSyncDelay(unsigned int milliseconds, const std::string& sink);
std::string defaultSinkName();
