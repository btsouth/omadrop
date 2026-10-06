#pragma once
// World check: tells a contributor whether a world folder is ready. Runs
// headless on the current GPU and reports plain results plus an optional JSON
// report. The pure analysis lives in check-analysis.h.
#include <QJsonObject>
#include <QString>
#include <QTextStream>
#include <vector>

namespace Journey::Kit::Check {

struct Options {
    QString world = QStringLiteral("osaka-jade");   // folder name under the worlds root
    QString fixture;                                // stereo float32 44100 Hz; built-in music when empty
    QString jsonPath;                               // full JSON report, when set
    QString reference;                              // Osaka Jade folder to compare cost with; found automatically when empty
    int seed = 1;
    double analysisSeconds = 30;                    // length of the music window that is rendered and analysed
    int fps = 30;                                   // 30 or 60
    int analysisWidth = 480, analysisHeight = 270;
    // Frame budget measurement at 1080p. Zero picks the defaults for the GPU
    // (fewer frames when the GPU is a software renderer).
    int budgetWarmupFrames = 0, budgetFrames = 0, budgetRounds = 0;
};

enum class Status { Pass, Fail, Informational, Skipped };

struct Item {
    QString id, title;
    Status status = Status::Skipped;
    bool required = true;
    QString message;
    QJsonObject details;
};

struct Report {
    QString world, gpu, fixture;
    bool softwareRendering = false;
    std::vector<Item> items;
    QJsonObject info;
    bool ready() const;
    const Item* find(const QString& id) const;
    QJsonObject json() const;
    QString summary() const;
};

// Runs every check. Throws std::runtime_error for problems that are not about
// the world itself (unreadable music file, no GPU context at all).
Report runWorldCheck(const Options&);
// Prints the summary to out, writes the JSON report when asked, and returns the
// exit code: 0 when ready, 1 when a required check fails, 2 on a usage error.
int runCheckCommand(const Options&, QTextStream& out, QTextStream& err);
}
