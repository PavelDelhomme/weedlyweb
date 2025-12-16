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
    : webView(nullptr), loadingStateCallback(nullptr) {
    // Ne pas créer la vue Web immédiatement - elle sera créée dans initializeRendering
    // après que GTK soit complètement initialisé
}


RenderingEngine::~RenderingEngine() {
    cleanupSignals();
    if (webView) {
        g_clear_object(&webView);
    }
}


// Variable statique pour stocker le container (pour l'indicateur de chargement)
static GtkWidget* loadingIndicator = nullptr;
static GtkWidget* webContainer = nullptr;

void RenderingEngine::initializeRendering(GtkWidget *mainContainer) {
    if (!mainContainer) {
        std::cerr << "Erreur : mainContainer est null dans initializeRendering" << std::endl;
        return;
    }
    
    // Créer la vue Web seulement maintenant, après que GTK soit complètement initialisé
    if (!webView) {
        webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
        if (!webView) {
            std::cerr << "Erreur : Impossible de créer WebView" << std::endl;
            return;
        }
    }
    
    // Vérifier que webView n'est pas déjà dans un container
    if (gtk_widget_get_parent(GTK_WIDGET(webView))) {
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(webView))), GTK_WIDGET(webView));
    }
    
    // Créer un container pour la webView (expandable)
    // NOTE: On utilise directement le mainContainer passé en paramètre, pas besoin de créer un nouveau container
    // Le mainContainer est déjà le webContainer de Browser
    webContainer = mainContainer;
    
    // Ne PAS créer d'indicateur de chargement ici - on utilise uniquement le spinner dans la barre d'URL
    // Cela évite les conflits et les problèmes d'affichage
    loadingIndicator = nullptr;
    
    // S'assurer que la webView est correctement configurée
    if (webView) {
        GtkWidget* webWidget = GTK_WIDGET(webView);
        
        // S'assurer que la WebView est expansible
        gtk_widget_set_vexpand(webWidget, TRUE);
        gtk_widget_set_hexpand(webWidget, TRUE);
        
        // Ne pas définir de taille minimale fixe - laisser GTK gérer automatiquement
        // Cela permet un meilleur redimensionnement
        
        // Afficher la WebView immédiatement (elle sera gérée par les onglets)
        // Ne pas l'afficher ici car elle sera gérée par Browser::addNewTab
    }
}

// Fonction helper pour gérer l'état de chargement (définie dans Browser.cpp)
// Déclaration externe pour éviter la dépendance circulaire
extern void browser_set_loading_state(bool loading);

// Callback pour masquer l'indicateur de chargement quand la page est chargée
static void on_load_finished(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    const gchar* uri = webkit_web_view_get_uri(web_view);
    std::string url = uri ? std::string(uri) : "unknown";
    
    if (load_event == WEBKIT_LOAD_STARTED) {
        std::cerr << "[DEBUG] WEBKIT_LOAD_STARTED pour URL: " << url << std::endl;
        
        // Afficher uniquement l'indicateur de chargement dans la barre d'URL
        browser_set_loading_state(true);
        
        // NE PAS masquer la WebView - la laisser visible pour un rendu immédiat
    } else if (load_event == WEBKIT_LOAD_FINISHED) {
        std::cerr << "[DEBUG] WEBKIT_LOAD_FINISHED pour URL: " << url << std::endl;
        
        // Masquer l'indicateur de chargement dans la barre d'URL
        browser_set_loading_state(false);
        
        // S'assurer que la WebView est visible et affichée
        if (web_view && GTK_IS_WIDGET(web_view)) {
            GtkWidget* webWidget = GTK_WIDGET(web_view);
            
            // Afficher la WebView et tous ses parents
            GtkWidget* parent = gtk_widget_get_parent(webWidget);
            while (parent) {
                gtk_widget_show_all(parent);
                gtk_widget_set_visible(parent, TRUE);
                parent = gtk_widget_get_parent(parent);
            }
            
            // Afficher la WebView
            gtk_widget_show_all(webWidget);
            gtk_widget_set_visible(webWidget, TRUE);
            
            // Forcer le redessinage
            gtk_widget_queue_draw(webWidget);
            gtk_widget_queue_resize(webWidget);
            
            std::cerr << "[DEBUG] WebView rendue visible après chargement" << std::endl;
        }
    } else if (load_event == WEBKIT_LOAD_COMMITTED) {
        std::cerr << "[DEBUG] WEBKIT_LOAD_COMMITTED pour URL: " << url << std::endl;
    } else if (load_event == WEBKIT_LOAD_REDIRECTED) {
        std::cerr << "[DEBUG] WEBKIT_LOAD_REDIRECTED pour URL: " << url << std::endl;
    } else {
        std::cerr << "[DEBUG] Événement de chargement inconnu: " << load_event << " pour URL: " << url << std::endl;
    }
}

void RenderingEngine::displayPage(const std::string& url) {
    std::cerr << "[DEBUG] displayPage() appelé avec URL: '" << url << "'" << std::endl;
    
    if (!webView || !WEBKIT_IS_WEB_VIEW(webView)) {
        std::cerr << "[ERREUR] WebView non initialisé dans displayPage." << std::endl;
        return;
    }
    
    std::cerr << "[DEBUG] WebView est valide" << std::endl;
    
    // Normaliser l'URL si nécessaire
    std::string normalizedUrl = url;
    if (normalizedUrl.empty()) {
        normalizedUrl = "https://www.duckduckgo.com";
        std::cerr << "[DEBUG] URL vide, utilisation de la valeur par défaut: " << normalizedUrl << std::endl;
    } else if (normalizedUrl.find("://") == std::string::npos) {
        // Si pas de protocole, ajouter https://
        normalizedUrl = "https://" + normalizedUrl;
        std::cerr << "[DEBUG] Protocole manquant, URL normalisée: " << normalizedUrl << std::endl;
    }
    
    std::cerr << "[DEBUG] Chargement de l'URL normalisée : " << normalizedUrl << std::endl;
    
    // Connecter le signal load-changed pour gérer l'indicateur de chargement
    static bool signalConnected = false;
    if (!signalConnected) {
        g_signal_connect(webView, "load-changed", G_CALLBACK(on_load_finished), nullptr);
        signalConnected = true;
    }
    
    // S'assurer que le WebView est visible avant de charger
    GtkWidget* webWidget = GTK_WIDGET(webView);
    if (webWidget) {
        // S'assurer que tous les parents sont visibles
        GtkWidget* parent = gtk_widget_get_parent(webWidget);
        while (parent) {
            gtk_widget_show_all(parent);
            gtk_widget_set_visible(parent, TRUE);
            parent = gtk_widget_get_parent(parent);
        }
        
        // S'assurer que la WebView est visible et expansible
        gtk_widget_show_all(webWidget);
        gtk_widget_set_visible(webWidget, TRUE);
        gtk_widget_set_vexpand(webWidget, TRUE);
        gtk_widget_set_hexpand(webWidget, TRUE);
    }
    
    // Charger l'URL
    webkit_web_view_load_uri(webView, normalizedUrl.c_str());
    
    // Forcer le rafraîchissement de l'affichage
    if (webWidget) {
        gtk_widget_queue_draw(webWidget);
        gtk_widget_queue_resize(webWidget);
    }
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
