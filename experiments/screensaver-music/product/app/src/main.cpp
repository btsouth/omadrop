#include "backend.h"
#include "theme.h"

#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTimer>
#include <QThread>

#include <cstdio>
#include <unistd.h>

namespace {
const char* kUsage =
    "Usage: omadrop-ui [--controls] [--quit]\n"
    "\n"
    "Native Omadrop product controller. Without arguments the visuals start\n"
    "immediately; --controls opens the native controls only. A second launch\n"
    "asks the running instance to stop its renderer and show the controls;\n"
    "--quit stops the session and the application.\n";

QString serverName() {
    return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
           + QStringLiteral("/omadrop-%1.sock").arg(static_cast<qulonglong>(::getuid()));
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("omadrop"));
    QGuiApplication::setOrganizationName(QStringLiteral("omadrop"));
    QGuiApplication::setDesktopFileName(QStringLiteral("omadrop"));

    bool controls = false;
    bool quit = false;
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit(argv[index]);
        if (argument == QLatin1String("--controls")) {
            controls = true;
        } else if (argument == QLatin1String("--quit")) {
            quit = true;
        } else if (argument == QLatin1String("-h") || argument == QLatin1String("--help")) {
            fputs(kUsage, stdout);
            return 0;
        } else {
            fprintf(stderr, "omadrop-ui: unknown option: %s\n", qPrintable(argument));
            fputs(kUsage, stderr);
            return 2;
        }
    }

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(qEnvironmentVariable("OMADROP_UI_STYLE", QStringLiteral("Basic")));
    }

    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    const QString name = serverName();
    QDir().mkpath(QFileInfo(name).absolutePath());
    QLockFile lock(name + QStringLiteral(".lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) {
        // Someone already owns the session: hand the request over and leave.
        QLocalSocket socket;
        bool connected = false;
        for (int attempt = 0; attempt < 20 && !connected; ++attempt) {
            socket.connectToServer(name);
            connected = socket.waitForConnected(25);
            if (!connected) {
                socket.abort();
                QThread::msleep(25);
            }
        }
        if (connected) {
            QByteArray command = quit ? QByteArray("quit\n") : QByteArray("controls\n");
            const QString requestedMode = qEnvironmentVariable("OMADROP_UI_MODE");
            if (!quit && (requestedMode == QLatin1String("omarchy") || requestedMode == QLatin1String("milkdrop"))) {
                command = QByteArray("controls ") + requestedMode.toUtf8() + '\n';
            }
            socket.write(command);
            socket.flush();
            socket.waitForBytesWritten(500);
            return 0;
        }
        fprintf(stderr, "omadrop-ui: the running instance could not be reached\n");
        return 1;
    }
    // We own the process lock, so any socket left by a crash is stale.
    QLocalServer::removeServer(name);
    if (!server.listen(name)) {
        fprintf(stderr, "omadrop-ui: could not start the session server: %s\n",
                qPrintable(server.errorString()));
        return 1;
    }

    Backend backend;
    Theme theme;
    bool quitRequested = quit;
    QObject::connect(&backend, &Backend::stopCompleted, &application, [&] {
        if (quitRequested) QCoreApplication::quit();
    });
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &backend, &Backend::shutdown);

    if (quit) {
        QTimer::singleShot(0, &backend, &Backend::stop);
        return application.exec();
    }

    QObject::connect(&server, &QLocalServer::newConnection, &application, [&] {
        while (QLocalSocket* socket = server.nextPendingConnection()) {
            auto receive = [&, socket] {
                if (!socket->canReadLine()) return;
                const QString command = QString::fromUtf8(socket->readLine()).trimmed();
                if (command == QLatin1String("quit")) {
                    quitRequested = true;
                } else if (command.startsWith(QLatin1String("controls "))) {
                    backend.setMode(command.mid(9));
                }
                backend.stop();
            };
            QObject::connect(socket, &QLocalSocket::readyRead, socket, receive);
            QTimer::singleShot(0, socket, receive);
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        fprintf(stderr, "omadrop-ui: could not load the controls interface\n");
        return 1;
    }

    if (!controls) {
        backend.play();
    }
    return application.exec();
}
