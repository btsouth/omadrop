QT += core gui qml quick opengl
CONFIG += c++17 link_pkgconfig
PKGCONFIG += fftw3f
TARGET = omadrop-osaka-live
SOURCES += src/main.cpp src/preview.cpp src/session.cpp src/audio.cpp src/score.cpp src/schedule.cpp \
    src/world.cpp src/canvas.cpp src/gpu.cpp src/rig.cpp src/osaka.cpp src/signs.cpp src/headless.cpp \
    ../projectm-ascii/pipewire_capture.cpp
HEADERS += src/preview.h
LIBS += -lEGL -lOpenGL -lpthread
RESOURCES += src/resources.qrc
QMAKE_CXXFLAGS_RELEASE += -O2
