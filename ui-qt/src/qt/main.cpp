#include <QApplication>
#include <QStyleFactory>
#include "qt/BrowserWindow.h"
#include <QWebEngineSettings>
#include <QLoggingCategory>
#include <QSurfaceFormat>
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    // Forcer le rendu logiciel si GPU non disponible
    // Ces variables d'environnement doivent être définies AVANT QApplication
    if (!std::getenv("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", "software");
    }
    if (!std::getenv("QT_OPENGL")) {
        qputenv("QT_OPENGL", "software");
    }
    // Désactiver Vulkan explicitement
    qputenv("QT_VULKAN_LIB", "");
    
    // Désactiver les warnings Vulkan/OpenGL si GPU non détecté
    QLoggingCategory::setFilterRules("qt.qpa.gl.debug=false\nqt.qpa.vulkan.debug=false");
    
    // Configurer le format de surface OpenGL pour le rendu logiciel
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setVersion(2, 1);
    QSurfaceFormat::setDefaultFormat(format);
    
    QApplication app(argc, argv);
    
    // Configuration de l'application
    app.setApplicationName("WeedlyWeb");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("WeedlyWeb");
    
    // Configurer WebEngine pour utiliser le rendu logiciel si nécessaire
    QWebEngineSettings::defaultSettings()->setAttribute(
        QWebEngineSettings::Accelerated2dCanvasEnabled, false);
    QWebEngineSettings::defaultSettings()->setAttribute(
        QWebEngineSettings::WebGLEnabled, false);
    
    // Créer et afficher la fenêtre principale
    BrowserWindow window;
    window.show();
    
    return app.exec();
}

