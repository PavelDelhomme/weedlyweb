#include "MoteurRendu.h"
#include <iostream>
#include <memory>

// Fonction statique pour gérer "notify::title"
static void on_notify_title(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    //auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    //auto data = std::make_shared<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>(vueWeb, callback);
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (data.first && WEBKIT_IS_WEB_VIEW(data.first)) {
        const gchar* title = webkit_web_view_get_title(data.first);
        if (title) {
            data.second(std::string(title));
        }
    }
    // if (vueWeb && WEBKIT_IS_WEB_VIEW(vueWeb)) {
    //     if (!data || !data->first || !WEBKIT_IS_WEB_VIEW(data->first)) {
    //         std::cerr << "Erreur : WebView invalide dans on_notify_title." << std::endl;
    //         return;
    //     }

    //     const gchar* title = webkit_web_view_get_title(data->first);
    //     if (title) {
    //         data->second(std::string(title));
    //     }
    //     g_signal_connect(vueWeb, "notify::title", G_CALLBACK(on_notify_title), data.get());

    //     // Libération de la mémoire si le signal est unique
    //     //delete data;
    // } else {
    //     std::cerr << "Erreur : WebVeiw non initialisé." << std::endl;
    // }
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

// Définition de la fonction statique helper (à mettre dans MoteurRendu.cpp)
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


MoteurRendu::MoteurRendu() {
    vueWeb = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!vueWeb) {    
        std::cerr << "Erreur : WebView non initialisé." << std::endl;
        return;
    }
}

MoteurRendu::~MoteurRendu() {
    nettoyerSignaux();
    if (vueWeb) {
        g_clear_object(&vueWeb);
    }
}


void MoteurRendu::initialiserRendu(GtkWidget *conteneurPrincipal) {
    GtkWidget *conteneur = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneur), GTK_WIDGET(vueWeb), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), conteneur, TRUE, TRUE, 0);
}

void MoteurRendu::afficherPage(const std::string& url) {
    if (!vueWeb) {
        std::cerr << "Erreur : WebView non initialisé dans afficherPage." << std::endl;
        return;
    }
    std::cout << "Chargement de l'URL : " << url << std::endl;
    try {
        webkit_web_view_load_uri(vueWeb, url.c_str());
    } catch (const std::exception& e) {
        std::cerr << "Exception lors du chargement de l'URL : " << e.what() << std::endl;
    }
}


std::string MoteurRendu::obtenirURLActuelle() const {
    const gchar* uri = webkit_web_view_get_uri(vueWeb);
    return uri ? std::string(uri) : "";
}

std::string MoteurRendu::obtenirTitreActuel() const {
    const gchar* title = webkit_web_view_get_title(vueWeb);
    return title ? std::string(title) : "Titre inconnu";
}


void MoteurRendu::naviguerRetour() {
    if (webkit_web_view_can_go_back(vueWeb)) {
        webkit_web_view_go_back(vueWeb);
    }
}

void MoteurRendu::naviguerSuivant() {
    if (webkit_web_view_can_go_forward(vueWeb)) {
        webkit_web_view_go_forward(vueWeb);
    }
}

GtkWidget* MoteurRendu::creerBouton(const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *image = gtk_image_new_from_icon_name(iconName.c_str(), GTK_ICON_SIZE_BUTTON);
    GtkWidget *bouton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(bouton), image);
    g_signal_connect(bouton, "clicked", callback, data);
    return bouton;
}


GtkWidget* MoteurRendu::creerChampTexte(GCallback callback, gpointer data) {
    GtkWidget *champ = gtk_entry_new();
    g_signal_connect(champ, "activate", callback, data);
    return champ;
}


// Mise à jour de la connexion au signal
void MoteurRendu::connecterSignalPageChargee(std::function<void(const std::string&)> callback) {
    if (!vueWeb || !callback) {
        std::cerr << "Erreur : WebView non initialisé dans connecterSignalPageChargee." << std::endl;
        return;
    };

    // Utilisation de std::make_shared pour une gestion propre de la mémoire
    auto* data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(vueWeb, callback);

    // Connexion du signal avec gestion sécurisée
    g_signal_connect_data(
        vueWeb,
        "notify::title",
        G_CALLBACK(on_notify_title),
        data,
        [](gpointer user_data, GClosure*) { 
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data); 
        },
        G_CONNECT_AFTER
    );
}


void MoteurRendu::connecterSignalChargementComplet(std::function<void(const std::string&)> callback) {
    if (!vueWeb) return;

    // auto* data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(vueWeb, callback);

    // g_signal_connect_data(
    //     vueWeb,
    //     "load-changed",
    //     G_CALLBACK(on_load_changed),
    //     data,
    //     [](gpointer user_data, GClosure*) { 
    //         delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data); 
    //     },
    //     G_CONNECT_AFTER
    // );
    auto data = std::make_shared<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>(vueWeb, callback);
    g_signal_connect_data(
        vueWeb, "notify::title",
        G_CALLBACK(on_notify_title),
        data.get(),
        [](gpointer user_data, GClosure*) { 
            delete static_cast<std::shared_ptr<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>*>(user_data);
        },
        G_CONNECT_AFTER
    );

}

// Mise a jour de la connexion du signal de favicon
void MoteurRendu::connecterSignalFaviconChange(std::function<void(cairo_surface_t*)> callback) {
    if (!vueWeb) return;

    g_signal_connect(vueWeb, "notify::favicon", G_CALLBACK(+[](WebKitWebView* web_view, GParamSpec*, gpointer user_data) {
        auto* callback = static_cast<std::function<void(cairo_surface_t*)>*>(user_data);
        cairo_surface_t* icon = webkit_web_view_get_favicon(web_view);
        if (icon) {
            (*callback)(icon);
        }
    }), new std::function<void(cairo_surface_t*)>(callback));

}


void MoteurRendu::rafraichirPage() {
    if (vueWeb && WEBKIT_IS_WEB_VIEW(vueWeb)) {
        webkit_web_view_reload(vueWeb);
    }
}

void MoteurRendu::connecterSignalURLChangee(std::function<void(const std::string&)> callback) {
    if (!vueWeb || !WEBKIT_IS_WEB_VIEW(vueWeb)) {
        std::cerr << "Erreur : WebView invalide dans connecterSignalURLChangee." << std::endl;
        return;
    }

    auto data = std::make_shared<std::pair<WebKitWebView*, std::function<void(const std::string&)>>>(vueWeb, callback);

    // Utilisation d'un std::shared_ptr sans `new`
    g_signal_connect_data(
        vueWeb,
        "notify::uri",
        G_CALLBACK(on_notify_uri),
        new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(*data),
        [](gpointer user_data, GClosure *) {
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
        },
        static_cast<GConnectFlags>(0)
    );

}


void MoteurRendu::onNotifyUri(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) return;

    const gchar* uri = webkit_web_view_get_uri(data->first);
    if (uri) data->second(std::string(uri));
}


void MoteurRendu::nettoyerSignaux() {
    if (vueWeb) {
        g_signal_handlers_disconnect_by_data(vueWeb, this);
        // Libération manuelle des données utilisées dans les signaux
        callbackTitreChange = nullptr;
    }
}


void MoteurRendu::connecterSignalTitreChange(std::function<void(const std::string&)> callback) {
    callbackTitreChange = callback;

    // Créer un pointeur persistant pour stocker les données
    auto* data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(vueWeb, callbackTitreChange);
    
    // Utilisation correcte de `g_signal_connect`
    g_signal_connect(vueWeb, "notify::title", G_CALLBACK(on_notify_title), data);
}
