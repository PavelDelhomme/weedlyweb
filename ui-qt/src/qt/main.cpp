#include <QApplication>
#include <QStyleFactory>
#include "qt/BrowserWindow.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Configuration de l'application
    app.setApplicationName("WeedlyWeb");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("WeedlyWeb");
    
    // Créer et afficher la fenêtre principale
    BrowserWindow window;
    window.show();
    
    return app.exec();
}

