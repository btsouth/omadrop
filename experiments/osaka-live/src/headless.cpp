#include "headless.h"
#include "glcore.h"

#define EGL_EGLEXT_PROTOTYPES 1
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace Journey {
HeadlessContext::~HeadlessContext() {
    if (display_) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_) eglDestroyContext(display_, context_);
        eglTerminate(display_);
    }
}

bool HeadlessContext::create(QString& error) {
    auto platformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    EGLDisplay display = platformDisplay
        ? platformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr)
        : EGL_NO_DISPLAY;
    if (display == EGL_NO_DISPLAY) {
        error = QStringLiteral("No surfaceless EGL display is available.");
        return false;
    }
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor)) {
        error = QStringLiteral("eglInitialize failed (0x%1).").arg(eglGetError(), 0, 16);
        return false;
    }
    display_ = display;
    if (!eglBindAPI(EGL_OPENGL_API)) {
        error = QStringLiteral("EGL cannot bind desktop OpenGL.");
        return false;
    }
    const EGLint attributes[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
    EGLContext context = eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attributes);
    if (context == EGL_NO_CONTEXT) {
        error = QStringLiteral("Cannot create an OpenGL 3.3 core context (0x%1).").arg(eglGetError(), 0, 16);
        return false;
    }
    context_ = context;
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        error = QStringLiteral("Cannot make the surfaceless context current.");
        return false;
    }
    renderer_ = QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return true;
}
}
