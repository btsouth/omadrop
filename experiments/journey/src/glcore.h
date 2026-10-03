#pragma once

// Plain OpenGL 3.3 core. GLVND's libOpenGL exports every entry point and
// dispatches to the current context, so headless EGL export and the Qt Quick
// preview share one code path without a loader.
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/glcorearb.h>
