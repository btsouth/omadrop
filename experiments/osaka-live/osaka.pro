QT += core gui qml quick opengl
CONFIG += c++17 link_pkgconfig
PKGCONFIG += fftw3f
TARGET = omadrop-osaka
SOURCES += src/main.cpp src/preview.cpp src/session.cpp src/audio.cpp src/score.cpp src/schedule.cpp \
    src/world.cpp src/canvas.cpp src/gpu.cpp src/rig.cpp src/osaka.cpp src/kit/sign-outlines.cpp src/headless.cpp \
    ../projectm-ascii/pipewire_capture.cpp
HEADERS += src/preview.h
LIBS += -lEGL -lOpenGL -lpthread
RESOURCES += src/resources.qrc
QMAKE_CXXFLAGS_RELEASE += -O3 -flto=8 -fno-math-errno
QMAKE_LFLAGS_RELEASE += -flto=8

SOURCES += src/kit/primitives.cpp
HEADERS += src/kit/primitives.h

SOURCES += src/kit/disc.cpp
HEADERS += src/kit/disc.h

SOURCES += src/kit/mountain.cpp
HEADERS += src/kit/mountain.h

SOURCES += src/kit/haze.cpp
HEADERS += src/kit/haze.h

SOURCES += src/kit/sky.cpp
HEADERS += src/kit/sky.h

SOURCES += src/kit/ridges.cpp
HEADERS += src/kit/ridges.h

SOURCES += src/kit/town.cpp
HEADERS += src/kit/town.h

SOURCES += src/kit/pane.cpp
HEADERS += src/kit/pane.h

SOURCES += src/kit/rooms.cpp
HEADERS += src/kit/rooms.h

SOURCES += src/kit/neon.cpp
HEADERS += src/kit/neon.h

HEADERS += src/kit/palette.h src/kit/disc_shaders.h src/kit/haze_shaders.h \
    src/kit/sky_shaders.h src/kit/layout.h src/kit/osaka-legacy.h

SOURCES += src/kit/lanterns.cpp
HEADERS += src/kit/lanterns.h

SOURCES += src/kit/cloth.cpp
HEADERS += src/kit/cloth.h

SOURCES += src/kit/festoon.cpp
HEADERS += src/kit/festoon.h

SOURCES += src/kit/sky-lanterns.cpp
HEADERS += src/kit/sky-lanterns.h

SOURCES += src/kit/animals.cpp
HEADERS += src/kit/animals.h

SOURCES += src/kit/chime.cpp
HEADERS += src/kit/chime.h

SOURCES += src/kit/city.cpp
HEADERS += src/kit/city.h

SOURCES += src/kit/downhill.cpp
HEADERS += src/kit/downhill.h

SOURCES += src/kit/grass.cpp
HEADERS += src/kit/grass.h

SOURCES += src/kit/wisteria.cpp
HEADERS += src/kit/wisteria.h

SOURCES += src/kit/network.cpp
HEADERS += src/kit/network.h
