QT += core gui qml quick opengl
CONFIG += c++17 link_pkgconfig
PKGCONFIG += fftw3f
TEMPLATE = app
TARGET = omadrop-journey
SOURCES += src/main.cpp src/backend.cpp src/audio.cpp src/score.cpp src/headless.cpp src/canvas.cpp src/gpu.cpp \
    src/rig.cpp src/world.cpp src/osaka.cpp src/title.cpp src/kanagawa.cpp src/crossing.cpp src/direction_study.cpp src/ending_water.cpp
HEADERS += src/backend.h src/audio.h src/scene.h src/score.h src/headless.h src/glcore.h src/canvas.h src/gpu.h \
    src/art.h src/rig.h src/world.h src/osaka_shaders.h src/parts.h
LIBS += -lEGL -lOpenGL
QMAKE_CXXFLAGS_RELEASE += -O2
DISTFILES += src/caption-assets/title-outlines.inc src/caption-assets/title_generate.py src/caption-assets/outlines.inc src/caption-assets/generate.py \
    src/caption-assets/README.md src/caption-assets/sign-outlines.inc src/caption-assets/sign_generate.py src/caption-assets/OFL.txt
RESOURCES += src/resources.qrc
