#include "rendering/RenderingEngine.h"
#include <iostream>
#include <memory>

// Fonction statique pour gérer "notify::title"
static void on_notify_title(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (data->first && WEBKIT_IS_WEB_VIEW(data->first)) {
        const gchar* title = webkit_web_view_get_title(data->first);
        if (title) {
            data->second(std::string(title));
        }
    }
}


// Fonction statique pour gérer "notify::uri"
static void on_notify_uri(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) {
        std::cerr << "Erreur : WebView invalide dans on_notify_uri." << std::endl;
        return;
    }

    const gchar* uri = webkit_web_view_get_uri(data->first);
    if (uri) {
        data->second(std::string(uri));
    }
}

// Fonction statique pour gérer "destroy"
static void on_destroy_callback(GObject *object, gpointer user_data) {
    auto *data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>*>*>(user_data);
    delete data; // Libère la mémoire allouée dynamiquement
}

// Définition de la fonction statique helper (à mettre dans RenderingEngine.cpp)
static void on_notify_title_helper(GObject* object, GParamSpec*, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) return;
    const gchar* title = webkit_web_view_get_title(data->first);
    if (title) data->second(std::string(title));
}

static void on_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        const gchar* title = webkit_web_view_get_title(web_view);
        auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
        if (title && data) {
            data->second(std::string(title));
        }
    }
}


RenderingEngine::RenderingEngine()
    : webView(nullptr) {
    // Ne pas créer la vue Web immédiatement - elle sera créée dans initializeRendering
    // après que GTK soit complètement initialisé
}


RenderingEngine::~RenderingEngine() {
    cleanupSignals();
    if (webView) {
        g_clear_object(&webView);
    }
}


void RenderingEngine::initializeRendering(GtkWidget *mainContainer) {
    std::cerr << "[DEBUG RenderingEngine] Début de initializeRendering()" << std::endl;
    if (!mainContainer) {
        std::cerr << "Erreur : mainContainer est null dans initializeRendering" << std::endl;
        return;
    }
    
    // Créer la vue Web seulement maintenant, après que GTK soit complètement initialisé
    std::cerr << "[DEBUG RenderingEngine] Création de WebView..." << std::endl;
    if (!webView) {
        webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
        std::cerr << "[DEBUG RenderingEngine] webkit_web_view_new() appelé" << std::endl;
        if (!webView) {
            std::cerr << "Erreur : Impossible de créer WebView" << std::endl;
            return;
        }
        std::cerr << "[DEBUG RenderingEngine] WebView créé avec succès" << std::endl;
    }
    
    // Vérifier que webView n'est pas déjà dans un container
    if (gtk_widget_get_parent(GTK_WIDGET(webView))) {
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(webView))), GTK_WIDGET(webView));
    }
    
    // Créer un container pour la webView (expandable)
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    if (!container) {
        std::cerr << "Erreur : Impossible de créer le container" << std::endl;
        return;
    }
    gtk_widget_set_name(container, "container-web");
    
    // Vérifier que webView n'a pas déjà un parent avant de l'ajouter
    if (!gtk_widget_get_parent(GTK_WIDGET(webView))) {
        gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(webView), TRUE, TRUE, 0);
        std::cerr << "[DEBUG RenderingEngine] webView ajoutée au container" << std::endl;
    }
    
    // Vérifier que le container n'est pas déjà dans le container principal
    std::cerr << "[DEBUG RenderingEngine] Ajout du container au container principal..." << std::endl;
    if (!gtk_widget_get_parent(container)) {
        // Ajouter à la fin avec expansion pour prendre tout l'espace restant
        gtk_box_pack_end(GTK_BOX(mainContainer), container, TRUE, TRUE, 0);
        // S'assurer que le container est expansible
        gtk_widget_set_vexpand(container, TRUE);
        gtk_widget_set_hexpand(container, TRUE);
        std::cerr << "[DEBUG RenderingEngine] Conteneur ajouté au container principal" << std::endl;
    }
    
    // S'assurer que la webView et le container sont visibles
    if (webView) {
        gtk_widget_set_vexpand(GTK_WIDGET(webView), TRUE);
        gtk_widget_set_hexpand(GTK_WIDGET(webView), TRUE);
        // S'assurer que le container est expansible
        gtk_widget_set_vexpand(container, TRUE);
        gtk_widget_set_hexpand(container, TRUE);
        // Afficher tous les widgets
        gtk_widget_show_all(GTK_WIDGET(webView));
        gtk_widget_show_all(container);
        std::cerr << "[DEBUG RenderingEngine] webView et container rendus visibles et expansibles" << std::endl;
    }
    
    std::cerr << "[DEBUG RenderingEngine] Fin de initializeRendering()" << std::endl;
}

void RenderingEngine::displayPage(const std::string& url) {
    std::cerr << "[DEBUG RenderingEngine] Début de displayPage(" << url << ")" << std::endl;
    if (!webView) {
        std::cerr << "Erreur : WebView non initialisé dans displayPage." << std::endl;
        return;
    }
    std::cout << "Chargement de l'URL : " << url << std::endl;
    std::cerr << "[DEBUG RenderingEngine] Appel de webkit_web_view_load_uri()..." << std::endl;
    try {
        webkit_web_view_load_uri(webView, url.c_str());
        std::cerr << "[DEBUG RenderingEngine] webkit_web_view_load_uri() terminé" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception lors du chargement de l'URL : " << e.what() << std::endl;
    }
    std::cerr << "[DEBUG RenderingEngine] Fin de displayPage()" << std::endl;
}


std::string RenderingEngine::getCurrentURL() const {
    const gchar* uri = webkit_web_view_get_uri(webView);
    return uri ? std::string(uri) : "";
}

std::string RenderingEngine::getCurrentTitle() const {
    const gchar* title = webkit_web_view_get_title(webView);
    return title ? std::string(title) : "Titre inconnu";
}


void RenderingEngine::navigateBack() {
    if (webkit_web_view_can_go_back(webView)) {
        webkit_web_view_go_back(webView);
    }
}

void RenderingEngine::navigateForward() {
    if (webkit_web_view_can_go_forward(webView)) {
        webkit_web_view_go_forward(webView);
    }
}

GtkWidget* RenderingEngine::createButton(const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *image = gtk_image_new_from_icon_name(iconName.c_str(), GTK_ICON_SIZE_BUTTON);
    GtkWidget *button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(button), image);
    // Connecter le signal seulement si un callback est fourni
    if (callback) {
        g_signal_connect(button, "clicked", callback, data);
    }
    return button;
}


GtkWidget* RenderingEngine::createTextEntry(GCallback callback, gpointer data) {
    GtkWidget *champ = gtk_entry_new();
    g_signal_connect(champ, "activate", callback, data);
    return champ;
}


// Mise à jour de la connexion au signal
void RenderingEngine::connectPageLoadedSignal(std::function<void(const std::string&)> callback) {
    if (!webView || !callback) {
        std::cerr << "Erreur : WebView non initialisé dans connectPageLoadedSignal." << std::endl;
        return;
    };

    // Utilisation de std::make_shared pour une gestion propre de la mémoire
    auto* data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(webView, callback);

    // Connexion du signal avec gestion sécurisée
    g_signal_connect_data(
        webView,
        "notify::title",
        G_CALLBACK(on_notify_title),
        data,
        [](gpointer user_data, GClosure*) { 
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data); 
        },
        G_CONNECT_AFTER
    );
}


void RenderingEngine::connectLoadCompleteSignal(std::function<void(const std::string&)> callback) {
    if (!webView) return;

    auto data = std::make_shared<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>(webView, callback);
    g_signal_connect_data(
        webView, "notify::title",
        G_CALLBACK(on_notify_title),
        data.get(),
        [](gpointer user_data, GClosure*) { 
            delete static_cast<std::shared_ptr<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>*>(user_data);
        },
        G_CONNECT_AFTER
    );

}

// Mise a jour de la connexion du signal de favicon
void RenderingEngine::connectFaviconChangedSignal(std::function<void(cairo_surface_t*)> callback) {
    if (!webView) return;

    g_signal_connect(webView, "notify::favicon", G_CALLBACK(+[](WebKitWebView* web_view, GParamSpec*, gpointer user_data) {
        auto* callback = static_cast<std::function<void(cairo_surface_t*)>*>(user_data);
        cairo_surface_t* icon = webkit_web_view_get_favicon(web_view);
        if (icon) {
            (*callback)(icon);
        }
    }), new std::function<void(cairo_surface_t*)>(callback));

}


void RenderingEngine::refreshPage() {
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        webkit_web_view_reload(webView);
    }
}

void RenderingEngine::connectURLChangedSignal(std::function<void(const std::string&)> callback) {
    if (!webView || !WEBKIT_IS_WEB_VIEW(webView)) {
        std::cerr << "Erreur : WebView invalide dans connectURLChangedSignal." << std::endl;
        return;
    }

    auto data = std::make_shared<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>(webView, callback);

    // Utilisation d'un std::shared_ptr sans `new`
    g_signal_connect_data(
        webView,
        "notify::uri",
        G_CALLBACK(on_notify_uri),
        new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(*data),
        [](gpointer user_data, GClosure *) {
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
        },
        static_cast<GConnectFlags>(0)
    );

}


void RenderingEngine::onNotifyUri(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) return;

    const gchar* uri = webkit_web_view_get_uri(data->first);
    if (uri) data->second(std::string(uri));
}


void RenderingEngine::cleanupSignals() {
    if (webView) {
        g_signal_handlers_disconnect_by_data(webView, this);
        // Libération manuelle des données utilisées dans les signaux
        callbackTitreChange = nullptr;
    }
}


void RenderingEngine::connectTitleChangedSignal(std::function<void(const std::string&)> callback) {
    callbackTitreChange = callback;

    // Créer un pointeur persistant pour stocker les données
    auto* data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(webView, callbackTitreChange);
    
    // Utilisation correcte de `g_signal_connect`
    g_signal_connect(webView, "notify::title", G_CALLBACK(on_notify_title), data);
}
