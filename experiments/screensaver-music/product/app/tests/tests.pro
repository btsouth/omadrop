QT += core testlib
CONFIG += c++17 console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = backend-tests

INCLUDEPATH += ../src

SOURCES += \
    tst_backend.cpp \
    ../src/backend.cpp \
    ../src/theme.cpp

HEADERS += \
    ../src/backend.h \
    ../src/theme.h
