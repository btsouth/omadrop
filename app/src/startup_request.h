#pragma once

#include <QString>

struct StartupRequest {
    bool play = false;
    bool quit = false;

    bool accept(const QString& argument) {
        if (argument == QLatin1String("--play")) play = true;
        else if (argument == QLatin1String("--controls")) play = false;
        else if (argument == QLatin1String("--quit")) quit = true;
        else return false;
        return true;
    }
};
