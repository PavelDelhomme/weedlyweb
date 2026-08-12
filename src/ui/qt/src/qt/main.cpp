#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QLoggingCategory>
#include <QSurfaceFormat>
#include "qt/BrowserWindow.h"
#include <cstdlib>

int main(int argc, char *argv[])
{
    if (!std::getenv("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", "software");
    }
    if (!std::getenv("QT_OPENGL")) {
        qputenv("QT_OPENGL", "software");
    }
    qputenv("QT_VULKAN_LIB", "");

    QLoggingCategory::setFilterRules("qt.qpa.gl.debug=false\nqt.qpa.vulkan.debug=false");

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setVersion(2, 1);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("WeedlyWeb"));
    app.setApplicationVersion(QStringLiteral("1.0"));
    app.setOrganizationName(QStringLiteral("WeedlyWeb"));

    if (QWebEngineSettings* settings = QWebEngineProfile::defaultProfile()->settings()) {
        settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
        settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
    }

    BrowserWindow window;
    window.show();
    return app.exec();
}
