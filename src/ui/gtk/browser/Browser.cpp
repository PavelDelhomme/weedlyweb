#include "browser/Browser.h"
#include "managers/FileManager.h"
#include "managers/MemoryManager.h"
#include "managers/FavoritesManager.h"
#include "utils/Utils.h"
#include "utils/CommandPalette.h"
#include "utils/RequestInterceptor.h"
#include "database/Database.h"
#include "core/PageEnhancements.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>
#include <unistd.h>
#include <curl/curl.h>
#include <cairo/cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

namespace {
bool isDebugLoggingEnabled() {
    const char* value = std::getenv("WEEDLYWEB_DEBUG");
    if (!value) {
        return false;
    }

    std::string normalized(value);
    return normalized != "0" && normalized != "false" && normalized != "FALSE" &&
           normalized != "off" && normalized != "OFF";
}

WebKitWebContext* getPersistentWebContext() {
    static WebKitWebContext* context = nullptr;
    if (context) {
        return context;
    }

    const std::string dataDir = FileManager::webkitDataDirectory();
    const std::string cacheDir = FileManager::webkitCacheDirectory();
    const std::string cookiesPath = FileManager::cookiesDatabasePath();

    WebKitWebsiteDataManager* manager = webkit_website_data_manager_new(
        "base-data-directory", dataDir.c_str(),
        "base-cache-directory", cacheDir.c_str(),
        nullptr);
    context = webkit_web_context_new_with_website_data_manager(manager);

    WebKitCookieManager* cookieManager = webkit_web_context_get_cookie_manager(context);
    webkit_cookie_manager_set_persistent_storage(
        cookieManager,
        cookiesPath.c_str(),
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    // Nécessaire pour rester connecté à Google et sites similaires
    webkit_cookie_manager_set_accept_policy(cookieManager, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    return context;
}

WebKitWebView* createPersistentWebView() {
    return WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(getPersistentWebContext()));
}
}

#define WEEDLYWEB_DEBUG_LOG(message) \
    do { \
        if (isDebugLoggingEnabled()) { \
            std::cerr << message << std::endl; \
        } \
    } while (false)

// Variable globale pour le callback de chargement
Browser* g_browser_instance = nullptr;

// Fonction helper pour gérer l'état de chargement dans la barre d'URL
void browser_set_loading_state(bool loading) {
    if (g_browser_instance) {
        g_browser_instance->setLoadingState(loading);
    }
}

// Méthode publique pour gérer l'état de chargement
void Browser::setLoadingState(bool loading) {
    if (loadingSpinner) {
        if (loading) {
            gtk_widget_show(loadingSpinner);
            gtk_spinner_start(GTK_SPINNER(loadingSpinner));
        } else {
            gtk_spinner_stop(GTK_SPINNER(loadingSpinner));
            gtk_widget_hide(loadingSpinner);
        }
    }
}

// Fonction utilitaire pour obtenir le chemin absolu d'un fichier
// Conforme aux standards C++17 : const-correctness et référence const
std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
}
static void delete_user_data(gpointer user_data, GClosure*) {
    delete static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
}

// Déclarations pour Utils.cpp (non-static pour être accessibles depuis Utils.cpp)
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
void on_modifier_favori(GtkWidget*, gpointer user_data);
void on_supprimer_favori(GtkWidget*, gpointer user_data);

static gboolean on_favori_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto* navigateur = static_cast<Browser*>(user_data);
        GtkWidget* menu = creerMenuContextuelFavoris(navigateur, widget);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}



// Définition pour Utils.cpp
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    if (!data) return;
    if (data->first) {
        data->first->addNewTab(data->second);
    }
    delete data;
}

// Définition pour Utils.cpp
void on_supprimer_favori(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->removeFavorite(data->second);
    }
    delete data;
}


static void on_menu_item_activate(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    data->first->loadURL(data->second);
    delete data;
}

static void free_browser_url_pair(gpointer data, GClosure*) {
    delete static_cast<std::pair<Browser*, std::string>*>(data);
}

static std::string truncate_favorite_label(const std::string& s) {
    constexpr std::size_t kMax = 12;
    if (s.size() <= kMax) {
        return s;
    }
    return s.substr(0, kMax - 3) + "...";
}

static std::string extract_host_from_url(const std::string& url) {
    std::string host = url;
    const auto proto = host.find("://");
    if (proto != std::string::npos) {
        host = host.substr(proto + 3);
    }
    const auto slash = host.find('/');
    if (slash != std::string::npos) {
        host = host.substr(0, slash);
    }
    const auto at = host.find('@');
    if (at != std::string::npos) {
        host = host.substr(at + 1);
    }
    const auto colon = host.find(':');
    if (colon != std::string::npos) {
        host = host.substr(0, colon);
    }
    return host;
}

static GtkWidget* create_letter_icon(const std::string& text, int size = 16) {
    GtkWidget* image = gtk_image_new();
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 0.30, 0.45, 0.75);
    cairo_arc(cr, size / 2.0, size / 2.0, size / 2.0 - 0.5, 0, 2 * G_PI);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, size * 0.55);
    char letter[8] = "?";
    if (!text.empty()) {
        letter[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
        letter[1] = '\0';
    }
    cairo_text_extents_t extents;
    cairo_text_extents(cr, letter, &extents);
    cairo_move_to(cr, (size - extents.width) / 2.0 - extents.x_bearing,
                  (size - extents.height) / 2.0 - extents.y_bearing);
    cairo_show_text(cr, letter);
    cairo_destroy(cr);
    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    if (pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
        g_object_unref(pixbuf);
    }
    return image;
}

static GtkWidget* create_folder_icon_widget(int size = 16) {
    GtkWidget* image = gtk_image_new_from_icon_name("folder", GTK_ICON_SIZE_MENU);
    gtk_image_set_pixel_size(GTK_IMAGE(image), size);
    return image;
}

struct FaviconLoadData {
    GtkWidget* image;
    std::string url;
};

static std::string favicon_cache_dir() {
    const char* home = g_get_user_cache_dir();
    std::string dir = std::string(home ? home : "/tmp") + "/weedlyweb/favicons";
    g_mkdir_with_parents(dir.c_str(), 0755);
    return dir;
}

static std::string favicon_cache_path_for_host(const std::string& host) {
    std::string safe;
    safe.reserve(host.size());
    for (char c : host) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-') {
            safe.push_back(c);
        } else {
            safe.push_back('_');
        }
    }
    return favicon_cache_dir() + "/" + safe + ".png";
}

static gboolean apply_downloaded_favicon(gpointer user_data) {
    auto* data = static_cast<FaviconLoadData*>(user_data);
    if (!data) {
        return FALSE;
    }
    if (data->image && GTK_IS_IMAGE(data->image) && !gtk_widget_in_destruction(data->image)) {
        GError* error = nullptr;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(data->url.c_str(), 16, 16, &error);
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(data->image), pixbuf);
            g_object_unref(pixbuf);
        }
        if (error) {
            g_error_free(error);
        }
        // Ne pas unlink : chemin = cache disque
    }
    delete data;
    return FALSE;
}

static void* download_favicon_thread(void* user_data) {
    auto* data = static_cast<FaviconLoadData*>(user_data);
    if (!data) {
        return nullptr;
    }
    const std::string host = extract_host_from_url(data->url);
    if (host.empty()) {
        g_idle_add(+[](gpointer p) -> gboolean {
            delete static_cast<FaviconLoadData*>(p);
            return FALSE;
        }, data);
        return nullptr;
    }

    const std::string cachePath = favicon_cache_path_for_host(host);
    if (g_file_test(cachePath.c_str(), G_FILE_TEST_IS_REGULAR)) {
        data->url = cachePath;
        g_idle_add(apply_downloaded_favicon, data);
        return nullptr;
    }

    const std::string iconUrl = "https://www.google.com/s2/favicons?sz=32&domain=" + host;
    CURL* curl = curl_easy_init();
    bool ok = false;
    if (curl) {
        FILE* fp = fopen(cachePath.c_str(), "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, iconUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "WeedlyWeb/1.0");
            const CURLcode res = curl_easy_perform(curl);
            fclose(fp);
            ok = (res == CURLE_OK);
        }
        curl_easy_cleanup(curl);
    }

    if (ok) {
        data->url = cachePath;
        g_idle_add(apply_downloaded_favicon, data);
    } else {
        unlink(cachePath.c_str());
        g_idle_add(+[](gpointer p) -> gboolean {
            delete static_cast<FaviconLoadData*>(p);
            return FALSE;
        }, data);
    }
    return nullptr;
}

static void start_favicon_load(GtkWidget* image, const std::string& pageUrl) {
    if (!image || pageUrl.empty()) {
        return;
    }
    // Appliquer immédiatement depuis le cache si dispo
    const std::string host = extract_host_from_url(pageUrl);
    if (!host.empty()) {
        const std::string cachePath = favicon_cache_path_for_host(host);
        if (g_file_test(cachePath.c_str(), G_FILE_TEST_IS_REGULAR)) {
            GError* error = nullptr;
            GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(cachePath.c_str(), 16, 16, &error);
            if (pixbuf) {
                gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
                g_object_unref(pixbuf);
                if (error) g_error_free(error);
                return;
            }
            if (error) g_error_free(error);
        }
    }
    auto* data = new FaviconLoadData{image, pageUrl};
    GThread* thread = g_thread_new("favicon-load", download_favicon_thread, data);
    if (thread) {
        g_thread_unref(thread);
    }
}

static void destroy_json_ptr(gpointer p);
static gboolean on_bookmark_menu_item_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
static gboolean on_folder_menu_item_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
static void populate_favorites_menu_from_children(GtkMenuShell* shell, Browser* browser, const nlohmann::json& children);

static GtkWidget* create_bookmark_menu_item(Browser* browser, const std::string& name, const std::string& url) {
    GtkWidget* mi = gtk_menu_item_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* icon = create_letter_icon(name, 14);
    GtkWidget* label = gtk_label_new(name.c_str());
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(mi), box);
    auto* data = new std::pair<Browser*, std::string>(browser, url);
    g_signal_connect(mi, "activate", G_CALLBACK(on_menu_item_activate), data);
    g_object_set_data_full(G_OBJECT(mi), "favorite-name", g_strdup(name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(mi), "favorite-url", g_strdup(url.c_str()), g_free);
    g_signal_connect(mi, "button-press-event", G_CALLBACK(on_bookmark_menu_item_button_press), browser);
    start_favicon_load(icon, url);
    return mi;
}

static GtkWidget* create_folder_menu_item(Browser* browser, const nlohmann::json& folder) {
    const std::string folderName = folder.value("name", "Dossier");
    GtkWidget* mi = gtk_menu_item_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* icon = create_folder_icon_widget(14);
    GtkWidget* label = gtk_label_new(folderName.c_str());
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(mi), box);

    GtkWidget* sub = gtk_menu_new();
    nlohmann::json subChildren = nlohmann::json::array();
    if (folder.contains("children") && folder["children"].is_array()) {
        subChildren = folder["children"];
    }
    populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), browser, subChildren);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub);
    auto* owned = new nlohmann::json(folder);
    g_object_set_data_full(G_OBJECT(mi), "folder-name", g_strdup(folderName.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(mi), "folder-json", owned, destroy_json_ptr);
    g_signal_connect(mi, "button-press-event", G_CALLBACK(on_folder_menu_item_button_press), browser);
    return mi;
}

static gboolean on_group_color_dot_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    std::string colorHex = navigateur->getTabsManager()->getCouleurGroupe(navigateur->getTabsManager()->getGroupeActif());
    double r = 0.3, g = 0.33, b = 0.82;
    if (colorHex.size() == 7 && colorHex[0] == '#') {
        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        r = (hex(colorHex[1]) * 16 + hex(colorHex[2])) / 255.0;
        g = (hex(colorHex[3]) * 16 + hex(colorHex[4])) / 255.0;
        b = (hex(colorHex[5]) * 16 + hex(colorHex[6])) / 255.0;
    }
    const int w = gtk_widget_get_allocated_width(widget);
    const int h = gtk_widget_get_allocated_height(widget);
    const double radius = (w < h ? w : h) / 2.0 - 0.5;
    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, w / 2.0, h / 2.0, radius, 0, 2 * G_PI);
    cairo_fill(cr);
    return FALSE;
}

static void free_move_tab_payload(gpointer data, GClosure*) {
    delete static_cast<std::pair<Browser*, std::pair<GtkWidget*, std::string>>*>(data);
}

static void on_move_tab_to_group_activate(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, std::pair<GtkWidget*, std::string>>*>(user_data);
    if (p && p->first) {
        p->first->moveTabToGroup(p->second.first, p->second.second);
    }
}

static void free_tab_widget_payload(gpointer data, GClosure*) {
    delete static_cast<std::pair<Browser*, GtkWidget*>*>(data);
}

static void on_tab_menu_close(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->removeTab(p->second);
    }
}

static void on_tab_menu_duplicate(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->duplicateTab(p->second);
    }
}

static void on_tab_menu_rename(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->renameTab(p->second);
    }
}

static void on_tab_menu_reset_title(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->resetTabTitle(p->second);
    }
}

static void on_tab_menu_toggle_pin(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->togglePinTab(p->second);
    }
}

static void on_tab_menu_add_favorite(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->addTabToFavorites(p->second);
    }
}

static void on_tab_menu_edit_favorite(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->editTabFavorite(p->second);
    }
}

static void on_tab_menu_remove_favorite(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (p && p->first && p->second) {
        p->first->removeTabFavorite(p->second);
    }
}

static void on_open_url_current_or_tab(GtkWidget*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, std::pair<std::string, bool>>*>(user_data);
    if (!p || !p->first) return;
    if (p->second.second) {
        p->first->addNewTab(p->second.first);
    } else {
        p->first->loadURL(p->second.first);
    }
}

static void free_open_url_payload(gpointer data, GClosure*) {
    delete static_cast<std::pair<Browser*, std::pair<std::string, bool>>*>(data);
}

static void destroy_json_ptr(gpointer p) {
    delete static_cast<nlohmann::json*>(p);
}

static void on_delete_favorite_activate(GtkWidget* w, gpointer ud) {
    auto* browser = static_cast<Browser*>(ud);
    const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "favorite-name"));
    if (!browser || !n) return;
    if (!browser->confirmAction("Supprimer le favori", std::string("Supprimer « ") + n + " » ?")) {
        return;
    }
    if (auto favs = browser->getFavoris()) {
        FavoritesJson::removeByNameRecursive(*favs, n);
        FileManager::writeJSON(FileManager::favoritesJSONPath(), *favs);
        browser->refreshFavoritesBar();
        browser->updateStarButton();
    }
}

static void on_edit_favorite_activate(GtkWidget*, gpointer ud) {
    auto* browser = static_cast<Browser*>(ud);
    if (browser) {
        browser->showFavoritesManager();
    }
}

static void on_open_all_folder_activate(GtkWidget* w, gpointer ud) {
    auto* browser = static_cast<Browser*>(ud);
    auto* folderJson = static_cast<nlohmann::json*>(g_object_get_data(G_OBJECT(w), "folder-json"));
    if (browser && folderJson) {
        browser->openAllBookmarksInFolder(*folderJson);
    }
}

static void on_delete_folder_activate(GtkWidget* w, gpointer ud) {
    auto* browser = static_cast<Browser*>(ud);
    const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "folder-name"));
    if (!browser || !n) return;
    if (!browser->confirmAction("Supprimer le dossier",
            std::string("Supprimer le dossier « ") + n + " » et son contenu ?")) {
        return;
    }
    if (auto favs = browser->getFavoris()) {
        FavoritesJson::removeByNameRecursive(*favs, n);
        FileManager::writeJSON(FileManager::favoritesJSONPath(), *favs);
        browser->refreshFavoritesBar();
        browser->updateStarButton();
    }
}

static gboolean on_bookmark_menu_item_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    if (event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_SECONDARY) {
        return FALSE;
    }
    auto* browser = static_cast<Browser*>(user_data);
    const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-name"));
    const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-url"));
    if (!browser || !url) {
        return TRUE;
    }

    GtkWidget* menu = gtk_menu_new();

    GtkWidget* openCurrent = gtk_menu_item_new_with_label("Ouvrir dans la page actuelle");
    auto* d1 = new std::pair<Browser*, std::pair<std::string, bool>>(browser, {url, false});
    g_signal_connect_data(openCurrent, "activate", G_CALLBACK(on_open_url_current_or_tab), d1, free_open_url_payload, (GConnectFlags)0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openCurrent);

    GtkWidget* openNew = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    auto* d2 = new std::pair<Browser*, std::pair<std::string, bool>>(browser, {url, true});
    g_signal_connect_data(openNew, "activate", G_CALLBACK(on_open_url_current_or_tab), d2, free_open_url_payload, (GConnectFlags)0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openNew);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* modifier = gtk_menu_item_new_with_label("Modifier…");
    g_signal_connect(modifier, "activate", G_CALLBACK(on_edit_favorite_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifier);

    GtkWidget* supprimer = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data_full(G_OBJECT(supprimer), "favorite-name", g_strdup(name ? name : ""), g_free);
    g_signal_connect(supprimer, "activate", G_CALLBACK(on_delete_favorite_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimer);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
    return TRUE;
}

static gboolean on_folder_menu_item_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    if (event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_SECONDARY) {
        return FALSE;
    }
    auto* browser = static_cast<Browser*>(user_data);
    const char* folderName = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "folder-name"));
    auto* folderJson = static_cast<nlohmann::json*>(g_object_get_data(G_OBJECT(widget), "folder-json"));
    if (!browser || !folderName) {
        return TRUE;
    }

    GtkWidget* menu = gtk_menu_new();

    GtkWidget* openAll = gtk_menu_item_new_with_label("Ouvrir tous les favoris");
    g_object_set_data(G_OBJECT(openAll), "folder-json", folderJson);
    g_signal_connect(openAll, "activate", G_CALLBACK(on_open_all_folder_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openAll);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* supprimer = gtk_menu_item_new_with_label("Supprimer le dossier…");
    g_object_set_data_full(G_OBJECT(supprimer), "folder-name", g_strdup(folderName), g_free);
    g_signal_connect(supprimer, "activate", G_CALLBACK(on_delete_folder_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimer);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
    return TRUE;
}

static void populate_favorites_menu_from_children(GtkMenuShell* shell, Browser* browser, const nlohmann::json& children) {
    if (!children.is_array() || children.empty()) {
        GtkWidget* emptyItem = gtk_menu_item_new_with_label("(vide)");
        gtk_widget_set_sensitive(emptyItem, FALSE);
        gtk_menu_shell_append(shell, emptyItem);
        return;
    }
    for (const auto& child : children) {
        if (FavoritesJson::isFolder(child)) {
            gtk_menu_shell_append(shell, create_folder_menu_item(browser, child));
        } else if (child.contains("url") && child.contains("name") && child["url"].is_string()) {
            gtk_menu_shell_append(shell, create_bookmark_menu_item(
                browser,
                child["name"].get<std::string>(),
                child["url"].get<std::string>()));
        }
    }
}

static GtkWidget* create_folder_menu_button(Browser* browser, const nlohmann::json& folderItem) {
    GtkWidget* mb = gtk_menu_button_new();
    const std::string baseName = folderItem.value("name", "Dossier");
    const std::string label = truncate_favorite_label(baseName);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(mb), GTK_ARROW_DOWN);
    GtkWidget* folderIcon = create_folder_icon_widget(14);
    gtk_button_set_always_show_image(GTK_BUTTON(mb), TRUE);
    gtk_button_set_image(GTK_BUTTON(mb), folderIcon);
    gtk_button_set_label(GTK_BUTTON(mb), label.c_str());
    gtk_widget_set_tooltip_text(mb, ("Dossier : " + baseName + " — clic droit pour plus d'actions").c_str());
    gtk_widget_set_name(mb, "button-favori-dossier");
    GtkWidget* menu = gtk_menu_new();
    nlohmann::json children = nlohmann::json::array();
    if (folderItem.contains("children") && folderItem["children"].is_array()) {
        children = folderItem["children"];
    }
    populate_favorites_menu_from_children(GTK_MENU_SHELL(menu), browser, children);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    GtkWidget* openAll = gtk_menu_item_new_with_label("Ouvrir tous les favoris");
    auto* ownedFolder = new nlohmann::json(folderItem);
    g_object_set_data_full(G_OBJECT(openAll), "folder-json", ownedFolder, destroy_json_ptr);
    g_signal_connect(openAll, "activate", G_CALLBACK(on_open_all_folder_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openAll);

    gtk_widget_show_all(menu);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(mb), menu);

    auto* ownedForBtn = new nlohmann::json(folderItem);
    g_object_set_data_full(G_OBJECT(mb), "folder-name", g_strdup(baseName.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(mb), "folder-json", ownedForBtn, destroy_json_ptr);
    g_signal_connect(mb, "button-press-event", G_CALLBACK(on_folder_menu_item_button_press), browser);

    gtk_widget_set_margin_start(mb, 1);
    gtk_widget_set_margin_end(mb, 1);
    gtk_widget_show_all(mb);
    return mb;
}

static void on_bookmark_load_url(GtkButton*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, std::string>*>(user_data);
    if (p && p->first) {
        p->first->loadURL(p->second);
    }
}

static void connect_bookmark_button_clicked(GtkWidget* boutonFavori, Browser* browser, const std::string& url) {
    auto* data = new std::pair<Browser*, std::string>(browser, url);
    g_signal_connect_data(
        boutonFavori,
        "clicked",
        G_CALLBACK(on_bookmark_load_url),
        data,
        free_browser_url_pair,
        (GConnectFlags)0);
}


static void on_favori_destroy(GtkWidget* widget, gpointer user_data) {
    g_free(user_data);
}

static void on_destroy_callback(GtkWidget* widget, gpointer user_data) {
    delete static_cast<std::pair<Browser*, std::string>*>(user_data);
}


static void on_ajouter_onglet(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        navigateur->addNewTab(navigateur->getHomepage());
    }
}

static void on_page_chargee(GtkLabel* label, const std::string& titre) {
    gtk_label_set_text(label, titre.c_str());
}


static void on_naviguer_retour(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateBack(button, navigateur);
}

static void on_naviguer_suivant(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateForward(button, navigateur);
}

static void on_aller_accueil(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onGoHome(button, navigateur);
}

static void on_rafraichir_page(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onRefreshPage(button, navigateur);
}
// Fonction statique pour gérer l'ouverture dans un nouvel onglet
static void on_ouvrir_nouvel_onglet(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    data->first->addNewTab(data->second);
    delete data;
}
// Définition pour Utils.cpp
void on_modifier_favori(GtkWidget*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->showFavoritesManager();
}


static void on_ajouter_favori_menu(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::pair<GtkWidget*, GtkWidget*>>*>(user_data);
    auto* navigateur = data->first;
    GtkWidget* entryNom = data->second.first;
    GtkWidget* entryURL = data->second.second;

    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(entryNom));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(entryURL));

    navigateur->addFavorite(nom, url, "Général");
    delete data;
}


static void on_bouton_favoris_clicked(GtkButton*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        navigateur->toggleCurrentPageFavorite();
    }
}

static void on_ajouter_groupe(GtkWidget* widget, gpointer data) {
    auto* info = static_cast<std::pair<Browser*, GtkWidget*>*>(data);
    if (info && info->first) {
        const gchar* groupName = gtk_entry_get_text(GTK_ENTRY(info->second));
        info->first->getTabsManager()->ajouterGroupe(groupName);
        info->first->changeTabGroup(groupName);
        delete info;
    }
}

static void on_changer_groupe(GtkWidget* item, gpointer data) {
    auto* navigateur = static_cast<Browser*>(data);
    if (navigateur) {
        const char* groupName = gtk_menu_item_get_label(GTK_MENU_ITEM(item));
        navigateur->changeTabGroup(groupName);
    }
}

static gboolean on_delete_event(GtkWidget*, GdkEvent*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->saveConfiguration();
    gtk_main_quit();
    return FALSE;
}

static gint weedly_key_snooper(GtkWidget*, GdkEventKey* event, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (!navigateur || !event || event->type != GDK_KEY_PRESS) {
        return FALSE;
    }
    return navigateur->handleKeyboardShortcut(event) ? TRUE : FALSE;
}


// ✅ Gestion du clic droit (menu contextuel)
static gboolean on_favoris_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_SECONDARY) {
        return FALSE;
    }
    auto* browser = static_cast<Browser*>(user_data);
    const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-name"));
    const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-url"));
    if (!browser || !url) {
        return TRUE;
    }

    GtkWidget* menu = gtk_menu_new();

    GtkWidget* openCurrent = gtk_menu_item_new_with_label("Ouvrir dans la page actuelle");
    auto* d1 = new std::pair<Browser*, std::pair<std::string, bool>>(browser, {url, false});
    g_signal_connect_data(openCurrent, "activate", G_CALLBACK(on_open_url_current_or_tab), d1, free_open_url_payload, (GConnectFlags)0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openCurrent);

    GtkWidget* openNew = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    auto* d2 = new std::pair<Browser*, std::pair<std::string, bool>>(browser, {url, true});
    g_signal_connect_data(openNew, "activate", G_CALLBACK(on_open_url_current_or_tab), d2, free_open_url_payload, (GConnectFlags)0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), openNew);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* modifier = gtk_menu_item_new_with_label("Modifier…");
    g_signal_connect(modifier, "activate", G_CALLBACK(on_edit_favorite_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifier);

    GtkWidget* supprimer = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data_full(G_OBJECT(supprimer), "favorite-name", g_strdup(name ? name : ""), g_free);
    g_signal_connect(supprimer, "activate", G_CALLBACK(on_delete_favorite_activate), browser);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimer);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
    return TRUE;
}

void on_button_ajouter_clicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);

    // Récupérer les valeurs des champs
    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryNomFavori()));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryURLFavori()));

    if (nom && url && *nom && *url) {
        // Ajout du favori dans la liste
        navigateur->addFavorite(nom, url, "Général");
        navigateur->refreshFavoritesBar();

        // Cacher le popover via la méthode publique
        gtk_widget_hide(navigateur->getPopoverFavoris());
    } else {
        std::cerr << "Veuillez remplir les deux champs." << std::endl;
    }
}




Browser::Browser() 
    : renderingEngine(std::make_unique<RenderingEngine>()),
      httpManager(std::make_unique<HTTPManager>()),
      memoryManager(std::make_unique<MemoryManager>()),
      tabsManager(std::make_unique<TabsManager>()),
      scriptEngine(std::make_unique<ScriptEngine>()),
      favorites(std::make_shared<nlohmann::json>()),
      window(nullptr),
      mainContainer(nullptr),
      navigationBar(nullptr),
      favoritesBar(nullptr),
      tabsBar(nullptr),
      groupChip(nullptr),
      groupChipLabel(nullptr),
      urlBar(nullptr),
      starButton(nullptr),
      darkModeButton(nullptr),
      readerModeButton(nullptr),
      loadingSpinner(nullptr),
      favoriteNameEntry(nullptr),
      favoriteUrlEntry(nullptr),
      favoritesPopover(nullptr),
      activeTab(nullptr),
      webContainer(nullptr),
      preloadHomeView(nullptr),
      forceDarkMode(false),
      readerMode(false)
{
    // Initialiser la base de données
    database = std::make_unique<Database>();
    if (!database->initDatabase()) {
        std::cerr << "Erreur lors de l'initialisation de la base de données" << std::endl;
    }
    
    // Initialiser l'intercepteur de requêtes
    requestInterceptor = std::make_unique<RequestInterceptor>();
    // Temporairement désactivé pour déboguer le crash
    // requestInterceptor->enable();
    
    // Initialiser la palette de commandes
    commandPalette = std::make_unique<CommandPalette>();
    commandPalette->setRequestInterceptor(requestInterceptor.get());
    
    favoritesManager = std::make_unique<FavoritesManager>(favorites, [this]() {
        refreshFavoritesBar();
        updateStarButton();
        refreshUrlCompletionModel();
    });
    loadBrowsingHistory();
    loadConfiguration();
    buildInterface();
    configureKeyboardShortcuts();
}


Browser::~Browser() {
    if (keySnooperId != 0) {
        gtk_key_snooper_remove(keySnooperId);
        keySnooperId = 0;
    }

    // Sauvegarder la configuration avant de fermer
    saveConfiguration();

    if (preloadHomeView) {
        g_object_unref(preloadHomeView);
        preloadHomeView = nullptr;
    }
    
    // Nettoyer les WebViews et leurs signaux avant de détruire les widgets
    // Les WebViews sont gérées par GTK mais on doit nettoyer les références
    for (auto& tab : tabs) {
        if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
            // Déconnecter les signaux de la WebView
            g_signal_handlers_disconnect_matched(tab.webView, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            // Note: Ne pas appeler g_object_unref car GTK gère la durée de vie des widgets
        }
        tab.webView = nullptr;
        tab.tabWidget = nullptr;
        tab.label = nullptr;
        tab.faviconImage = nullptr;
    }
    tabs.clear();
    activeTab = nullptr;
    
    // Nettoyer le moteur de rendu en premier (qui nettoie ses propres signaux)
    if (renderingEngine) {
        renderingEngine.reset();
    }
    
    // Nettoyer les signaux avant de détruire les widgets
    if (window && GTK_IS_WIDGET(window)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(window)) {
            // Déconnecter tous les signaux de la fenêtre
            g_signal_handlers_disconnect_matched(window, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            
            // Détruire la fenêtre (cela détruira automatiquement tous les children)
            gtk_widget_destroy(window);
        }
        window = nullptr;
    }
    
    // Réinitialiser les pointeurs pour éviter les accès après destruction
    mainContainer = nullptr;
    navigationBar = nullptr;
    favoritesBar = nullptr;
    tabsBar = nullptr;
    urlBar = nullptr;
    starButton = nullptr;
    darkModeButton = nullptr;
    readerModeButton = nullptr;
    loadingSpinner = nullptr;
    favoriteNameEntry = nullptr;
    favoriteUrlEntry = nullptr;
    favoritesPopover = nullptr;
    webContainer = nullptr;
}

std::shared_ptr<nlohmann::json> Browser::getFavoris() {
    return favorites;
}

void Browser::buildInterface() {
    // Debug messages removed for cleaner output
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "WeedlyWeb");
    
    // Forcer le mode sombre sur la fenêtre (utilise le thème système)
    GtkSettings* settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    
    // Activer le redimensionnement de la fenêtre
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    fitWindowToMonitor();
    
    // F11 / raccourcis : gérés via key snooper (configureKeyboardShortcuts)
    // pour fonctionner même quand le focus est dans WebKit.
    
    // Définir l'icône de la fenêtre
    std::string iconPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.png");
    if (std::filesystem::exists(iconPath)) {
        GdkPixbuf *icon = gdk_pixbuf_new_from_file(iconPath.c_str(), nullptr);
        if (icon) {
            gtk_window_set_icon(GTK_WINDOW(window), icon);
            g_object_unref(icon);
        }
    } else {
        // Essayer avec le SVG si PNG n'existe pas
        std::string svgPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.svg");
        if (std::filesystem::exists(svgPath)) {
            GdkPixbuf *icon = gdk_pixbuf_new_from_file(svgPath.c_str(), nullptr);
            if (icon) {
                gtk_window_set_icon(GTK_WINDOW(window), icon);
                g_object_unref(icon);
            }
        }
    }
    
    // Configuration pour que la fenêtre apparaisse dans la barre des tâches
    // Définir le nom de classe X11 pour l'identification par le gestionnaire de fenêtres
    gtk_widget_set_name(window, "weedlyweb");
    
    // Définir le rôle de la fenêtre (pour le gestionnaire de fenêtres)
    gtk_window_set_role(GTK_WINDOW(window), "weedlyweb-browser");
    
    // S'assurer que la fenêtre n'est pas ignorée par le gestionnaire de fenêtres
    // (par défaut, GTK_WINDOW_TOPLEVEL devrait déjà être visible, mais on s'en assure)
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), FALSE);
    
    // Définir le type de fenêtre (normal, pas un splash ou un popup)
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);
    

    // Connexion sécurisée du signal de fermeture avec lambda sécurisée
    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->saveConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  
    }), this);

    // Conteneur principal
    mainContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), mainContainer);

    // CORRECT ORDER: Bars at top, web view at bottom (expandable)
    // 1. Tabs bar (at top)
    initializeTabsBar();
    
    // 2. Navigation bar with URL (under tabs)
    initializeNavigationBar();
    
    // 3. Favorites bar (under URL bar)
    refreshFavoritesBar();
    
    // 4. Web container (at bottom, expandable) - MUST be last to take remaining space
    // NOUVELLE APPROCHE : Container simple et direct
    webContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(webContainer, "container-web");
    gtk_widget_set_vexpand(webContainer, TRUE);
    gtk_widget_set_hexpand(webContainer, TRUE);
    
    // S'assurer que le container est visible dès le début
    gtk_widget_show_all(webContainer);
    
    if (mainContainer && !gtk_widget_get_parent(webContainer)) {
        // Utiliser pack_end pour que le container web prenne tout l'espace restant
        gtk_box_pack_end(GTK_BOX(mainContainer), webContainer, TRUE, TRUE, 0);
    }
    
    // Initialiser le moteur de rendu (passe le webContainer)
    renderingEngine->initializeRendering(webContainer);
    
    // Charger les styles CSS pour un design minimaliste
    loadStyles();
    
    // Initialiser l'instance globale pour le callback de chargement
    g_browser_instance = this;
    
    // Afficher la fenêtre
    gtk_widget_show_all(window);
    
    // Présenter la fenêtre au gestionnaire de fenêtres (pour qu'elle apparaisse dans la barre des tâches)
    gtk_window_present(GTK_WINDOW(window));
    // Ne pas maximiser/forcer un moniteur : laisse la fenêtre sur l'écran où elle a été ouverte
    
    // Forcer le traitement des événements GTK pour s'assurer que la fenêtre est rendue
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    // Précharger la page d'accueil en arrière-plan pendant que l'UI se stabilise
    preloadHomepage();
    
    // Ajouter le premier onglet : restauration de session (style Chrome/Brave) ou accueil
    restoreSessionTabs();

    // Filet de sécurité : si le signal load-finished a été manqué, retenter le focus
    // sur le champ de recherche de la page d'accueil après chargement typique.
    g_timeout_add(1200, +[](gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        if (!navigateur || !navigateur->consumeHomepageSearchFocus()) {
            return FALSE;
        }
        if (navigateur->activeTab && navigateur->activeTab->webView &&
            WEBKIT_IS_WEB_VIEW(navigateur->activeTab->webView)) {
            navigateur->focusHomepageSearchBox(navigateur->activeTab->webView);
        }
        return FALSE;
    }, this);

    renderingEngine->connectURLChangedSignal([this](const std::string& url) {
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), url.c_str());
        }
        // Mettre à jour le bouton étoile selon si l'URL est en favoris
        updateStarButton();
        applyPageEnhancements();
    });

    initializeFavoritesPopover();
    
    // Mettre à jour le bouton étoile initial
    updateStarButton();
    updateEnhancementButtons();
    // gtk_widget_show_all sera appelé après l'ajout de l'onglet
}

void Browser::addButton(GtkWidget* container, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *button = renderingEngine->createButton(iconName, callback, data);
    if (button && GTK_IS_WIDGET(button) && !gtk_widget_get_parent(button)) {
        gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
    }
}

std::string Browser::getCurrentTitle() const {
    if (activeTab && activeTab->webView) {
        const gchar* title = webkit_web_view_get_title(activeTab->webView);
        return title ? std::string(title) : "Titre inconnu";
    }
    return "Titre inconnu";
}



GtkWidget* obtenirDernierEnfant(GtkWidget* parent) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(parent));
    return g_list_last(children) ? GTK_WIDGET(g_list_last(children)->data) : nullptr;
}

GtkWidget* obtenirPremierEnfant(GtkWidget* parent) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(parent));
    return children ? GTK_WIDGET(children->data) : nullptr;
}

void Browser::initializeNavigationBar() {
    navigationBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(navigationBar, 5);
    gtk_widget_set_margin_end(navigationBar, 5);
    gtk_widget_set_margin_top(navigationBar, 5);
    gtk_widget_set_margin_bottom(navigationBar, 5);
    gtk_widget_set_name(navigationBar, "barre-navigation");

    // Boutons de navigation (retour, suivant, rafraîchir, accueil)
    addButton(navigationBar, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    addButton(navigationBar, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    addButton(navigationBar, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    addButton(navigationBar, "go-home", G_CALLBACK(on_aller_accueil), this);
    
    // Container pour la barre d'URL avec indicateur de chargement
    GtkWidget* urlContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_name(urlContainer, "url-container");
    
    // Indicateur de chargement (spinner) dans la barre d'URL
    loadingSpinner = gtk_spinner_new();
    gtk_widget_set_size_request(loadingSpinner, 16, 16);
    gtk_widget_set_margin_start(loadingSpinner, 5);
    gtk_widget_set_margin_end(loadingSpinner, 5);
    gtk_widget_hide(loadingSpinner); // Masqué par défaut
    gtk_box_pack_start(GTK_BOX(urlContainer), loadingSpinner, FALSE, FALSE, 0);
    
    // Barre d'URL (expandable)
    urlBar = renderingEngine->createTextEntry(G_CALLBACK(&Browser::onUrlBarActivate), this);
    gtk_box_pack_start(GTK_BOX(urlContainer), urlBar, TRUE, TRUE, 0);
    setupUrlBarCompletion();
    
    // Ajouter le container URL à la barre de navigation
    gtk_box_pack_start(GTK_BOX(navigationBar), urlContainer, TRUE, TRUE, 0);

    // Bouton favorites (étoile) — une seule icône, pas de label doublon
    starButton = gtk_button_new();
    GtkWidget* starImg = gtk_image_new_from_icon_name("non-starred-symbolic", GTK_ICON_SIZE_BUTTON);
    if (!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "non-starred-symbolic")) {
        starImg = gtk_image_new_from_icon_name("non-starred", GTK_ICON_SIZE_BUTTON);
    }
    gtk_button_set_image(GTK_BUTTON(starButton), starImg);
    gtk_button_set_label(GTK_BUTTON(starButton), nullptr);
    gtk_button_set_always_show_image(GTK_BUTTON(starButton), TRUE);
    gtk_widget_set_name(starButton, "button-etoile");
    gtk_widget_set_tooltip_text(starButton, "Ajouter aux favoris");
    gtk_widget_set_margin_start(starButton, 3);
    gtk_widget_set_margin_end(starButton, 3);
    g_signal_connect(starButton, "clicked", G_CALLBACK(on_bouton_favoris_clicked), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), starButton, FALSE, FALSE, 0);

    // Mode sombre forcé (type Dark Reader) — à côté de l'étoile
    darkModeButton = gtk_toggle_button_new();
    {
        GtkIconTheme* theme = gtk_icon_theme_get_default();
        const char* darkIcon = "weather-clear-night-symbolic";
        if (!gtk_icon_theme_has_icon(theme, darkIcon)) {
            darkIcon = gtk_icon_theme_has_icon(theme, "weather-clear-night")
                           ? "weather-clear-night"
                           : "preferences-desktop-display-symbolic";
        }
        if (!gtk_icon_theme_has_icon(theme, darkIcon)) {
            darkIcon = "dialog-information";
        }
        GtkWidget* darkImg = gtk_image_new_from_icon_name(darkIcon, GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(darkModeButton), darkImg);
        gtk_button_set_always_show_image(GTK_BUTTON(darkModeButton), TRUE);
    }
    gtk_widget_set_name(darkModeButton, "button-dark-mode");
    gtk_widget_set_tooltip_text(darkModeButton, "Mode sombre des pages (comme Dark Reader)");
    gtk_widget_set_margin_start(darkModeButton, 2);
    g_signal_connect(darkModeButton, "toggled", G_CALLBACK(+[](GtkToggleButton*, gpointer user_data) {
        static_cast<Browser*>(user_data)->toggleForceDarkMode();
    }), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), darkModeButton, FALSE, FALSE, 0);

    // Mode lecture (type Chrome mobile)
    readerModeButton = gtk_toggle_button_new();
    {
        GtkIconTheme* theme = gtk_icon_theme_get_default();
        const char* readerIcon = "view-paged-symbolic";
        if (!gtk_icon_theme_has_icon(theme, readerIcon)) {
            readerIcon = gtk_icon_theme_has_icon(theme, "view-paged")
                             ? "view-paged"
                             : (gtk_icon_theme_has_icon(theme, "format-justify-left")
                                    ? "format-justify-left"
                                    : "text-x-generic");
        }
        GtkWidget* readerImg = gtk_image_new_from_icon_name(readerIcon, GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(readerModeButton), readerImg);
        gtk_button_set_always_show_image(GTK_BUTTON(readerModeButton), TRUE);
    }
    gtk_widget_set_name(readerModeButton, "button-reader-mode");
    gtk_widget_set_tooltip_text(readerModeButton, "Mode lecture (texte épuré)");
    gtk_widget_set_margin_start(readerModeButton, 2);
    gtk_widget_set_margin_end(readerModeButton, 2);
    g_signal_connect(readerModeButton, "toggled", G_CALLBACK(+[](GtkToggleButton*, gpointer user_data) {
        static_cast<Browser*>(user_data)->toggleReaderMode();
    }), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), readerModeButton, FALSE, FALSE, 0);

    // Menu hamburger (trois barres horizontales) pour les options
    GtkWidget* boutonMenu = renderingEngine->createButton("open-menu", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showOptionsMenu();
    }), this);
    if (boutonMenu && GTK_IS_WIDGET(boutonMenu)) {
        gtk_box_pack_start(GTK_BOX(navigationBar), boutonMenu, FALSE, FALSE, 0);
    }

    // Ajouter la barre de navigation au container principal
    if (mainContainer && !gtk_widget_get_parent(navigationBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), navigationBar, FALSE, FALSE, 0);
    }
    updateEnhancementButtons();
}


std::string Browser::getCurrentURL() const {
    if (activeTab && activeTab->webView) {
        const gchar* uri = webkit_web_view_get_uri(activeTab->webView);
        return uri ? std::string(uri) : "";
    }
    return "";
}


void Browser::initializeFavoritesBar() {
    refreshFavoritesBar();
}

void Browser::showFavoritesMenu() {
    GtkWidget* menu = gtk_menu_new();
    for (const auto& favori : *favorites) {
        if (FavoritesJson::isFolder(favori)) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori.value("name", "Dossier").c_str());
            GtkWidget* sub = gtk_menu_new();
            nlohmann::json ch = nlohmann::json::array();
            if (favori.contains("children") && favori["children"].is_array()) {
                ch = favori["children"];
            }
            populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), this, ch);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        } else if (favori.contains("name") && favori.contains("url")) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"].get<std::string>());
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
    gtk_widget_show_all(menu);
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showRemainingFavoritesMenu() {
    constexpr int kMaxTopLevelSlots = 14;
    GtkWidget* menu = gtk_menu_new();
    int index = 0;
    for (const auto& favori : *favorites) {
        if (index++ < kMaxTopLevelSlots) {
            continue;
        }
        if (FavoritesJson::isFolder(favori)) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori.value("name", "Dossier").c_str());
            GtkWidget* sub = gtk_menu_new();
            nlohmann::json ch = nlohmann::json::array();
            if (favori.contains("children") && favori["children"].is_array()) {
                ch = favori["children"];
            }
            populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), this, ch);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        } else if (favori.contains("name") && favori.contains("url")) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"].get<std::string>());
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
    GList* kids = gtk_container_get_children(GTK_CONTAINER(menu));
    const bool empty = (kids == nullptr);
    if (kids) {
        g_list_free(kids);
    }
    if (empty) {
        GtkWidget* item = gtk_menu_item_new_with_label("(aucun)");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    gtk_widget_show_all(menu);
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}


void Browser::showFavoritesManager() {
    if (!favoritesManager) {
        favoritesManager = std::make_unique<FavoritesManager>(favorites, [this]() { refreshFavoritesBar(); });
    }
    favoritesManager->setPageContext(getCurrentTitle(), getCurrentURL());
    favoritesManager->showWindow();
}


void Browser::refreshFavoritesBar() {
    constexpr int kMaxTopLevelSlots = 14;

    if (favoritesBar) {
        if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
            GtkWidget* parent = gtk_widget_get_parent(favoritesBar);
            if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
                    gtk_container_remove(GTK_CONTAINER(parent), favoritesBar);
                }
            }
            if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
                gtk_widget_destroy(favoritesBar);
            }
        }
        favoritesBar = nullptr;
        groupChip = nullptr;
        groupChipLabel = nullptr;
    }

    favoritesBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_name(favoritesBar, "barre-favorites");
    gtk_widget_set_margin_start(favoritesBar, 6);
    gtk_widget_set_margin_end(favoritesBar, 6);
    gtk_widget_set_margin_top(favoritesBar, 2);
    gtk_widget_set_margin_bottom(favoritesBar, 2);

    // Groupes en début de barre (style Chrome/Brave)
    groupChip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_name(groupChip, "group-chip");
    GtkWidget* colorDot = gtk_drawing_area_new();
    gtk_widget_set_size_request(colorDot, 10, 10);
    g_signal_connect(colorDot, "draw", G_CALLBACK(on_group_color_dot_draw), this);
    gtk_box_pack_start(GTK_BOX(groupChip), colorDot, FALSE, FALSE, 0);
    groupChipLabel = gtk_label_new(tabsManager->getGroupeActif().c_str());
    gtk_widget_set_name(groupChipLabel, "group-chip-label");
    gtk_box_pack_start(GTK_BOX(groupChip), groupChipLabel, FALSE, FALSE, 0);
    GtkWidget* boutonGroupes = gtk_button_new_with_label("▾");
    gtk_widget_set_name(boutonGroupes, "button-groupes");
    gtk_widget_set_tooltip_text(boutonGroupes, "Gérer les groupes d'onglets");
    g_signal_connect(boutonGroupes, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        static_cast<Browser*>(user_data)->showGroupsMenu();
    }), this);
    gtk_box_pack_start(GTK_BOX(groupChip), boutonGroupes, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(favoritesBar), groupChip, FALSE, FALSE, 0);

    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(sep, 4);
    gtk_widget_set_margin_end(sep, 4);
    gtk_box_pack_start(GTK_BOX(favoritesBar), sep, FALSE, FALSE, 0);

    if (favorites && !favorites->empty()) {
        int compteur = 0;
        const int totalRoot = static_cast<int>(favorites->size());
        for (const auto& favori : *favorites) {
            if (compteur >= kMaxTopLevelSlots) {
                break;
            }
            if (FavoritesJson::isFolder(favori)) {
                GtkWidget* folderBtn = create_folder_menu_button(this, favori);
                gtk_box_pack_start(GTK_BOX(favoritesBar), folderBtn, FALSE, FALSE, 0);
            } else if (favori.contains("url") && favori.contains("name") && favori["url"].is_string()) {
                const std::string rawName = favori["name"].get<std::string>();
                const std::string rawUrl = favori["url"].get<std::string>();
                const std::string label = truncate_favorite_label(rawName);
                GtkWidget* boutonFavori = gtk_button_new();
                gtk_button_set_always_show_image(GTK_BUTTON(boutonFavori), TRUE);
                GtkWidget* icon = create_letter_icon(rawName, 14);
                gtk_button_set_image(GTK_BUTTON(boutonFavori), icon);
                gtk_button_set_label(GTK_BUTTON(boutonFavori), label.c_str());
                gtk_widget_set_tooltip_text(boutonFavori, rawUrl.c_str());
                gtk_widget_set_name(boutonFavori, "button-favori");
                g_object_set_data_full(G_OBJECT(boutonFavori), "favorite-url", g_strdup(rawUrl.c_str()), g_free);
                g_object_set_data_full(G_OBJECT(boutonFavori), "favorite-name", g_strdup(rawName.c_str()), g_free);
                connect_bookmark_button_clicked(boutonFavori, this, rawUrl);
                g_signal_connect(boutonFavori, "button-press-event", G_CALLBACK(on_favoris_button_press), this);
                start_favicon_load(icon, rawUrl);
                gtk_box_pack_start(GTK_BOX(favoritesBar), boutonFavori, FALSE, FALSE, 0);
            }
            compteur++;
        }

        // ⋯ uniquement s'il reste des favoris racine non affichés
        if (totalRoot > kMaxTopLevelSlots) {
            GtkWidget* overflowBtn = gtk_button_new_with_label("⋯");
            gtk_widget_set_name(overflowBtn, "button-favori-more");
            gtk_widget_set_tooltip_text(overflowBtn, "Autres favoris et gestionnaire");
            g_signal_connect(overflowBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                static_cast<Browser*>(user_data)->showFavoritesOverflowMenu();
            }), this);
            gtk_box_pack_end(GTK_BOX(favoritesBar), overflowBtn, FALSE, FALSE, 0);
        }
    }

    if (mainContainer && !gtk_widget_get_parent(favoritesBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), favoritesBar, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(mainContainer), favoritesBar, 2);
    }
    gtk_widget_show_all(favoritesBar);
    updateGroupChip();
    updateStarButton();
    refreshUrlCompletionModel();
}


void Browser::updateStarButton() {
    if (!starButton || !GTK_IS_BUTTON(starButton)) {
        return;
    }
    std::string urlActuelle = getCurrentURL();
    const bool estDejaFavori = !urlActuelle.empty() &&
        favorites && FavoritesJson::containsUrlRecursive(*favorites, urlActuelle);

    const char* iconName = estDejaFavori ? "starred-symbolic" : "non-starred-symbolic";
    const char* fallback = estDejaFavori ? "starred" : "non-starred";
    if (!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), iconName)) {
        iconName = fallback;
    }
    GtkWidget* img = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(starButton), img);
    gtk_button_set_label(GTK_BUTTON(starButton), nullptr);
    gtk_button_set_always_show_image(GTK_BUTTON(starButton), TRUE);
    gtk_widget_set_tooltip_text(starButton,
        estDejaFavori ? "Retirer des favoris" : "Ajouter aux favoris");
}

void Browser::updateEnhancementButtons() {
    suppressEnhancementSignals = true;
    if (darkModeButton && GTK_IS_TOGGLE_BUTTON(darkModeButton)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(darkModeButton), forceDarkMode);
        gtk_widget_set_tooltip_text(darkModeButton,
            forceDarkMode ? "Désactiver le mode sombre des pages"
                          : "Mode sombre des pages (comme Dark Reader)");
    }
    if (readerModeButton && GTK_IS_TOGGLE_BUTTON(readerModeButton)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(readerModeButton), readerMode);
        gtk_widget_set_tooltip_text(readerModeButton,
            readerMode ? "Quitter le mode lecture" : "Mode lecture (texte épuré)");
    }
    suppressEnhancementSignals = false;
}

void Browser::applyPageEnhancements(WebKitWebView* webView) {
    WebKitWebView* view = webView;
    if (!view && activeTab) {
        view = activeTab->webView;
    }
    if (!view || !WEBKIT_IS_WEB_VIEW(view) || !scriptEngine) {
        return;
    }

    const gchar* uri = webkit_web_view_get_uri(view);
    if (!uri || uri[0] == '\0' || g_str_has_prefix(uri, "about:") ||
        g_str_has_prefix(uri, "weedly:") || g_str_has_prefix(uri, "data:")) {
        return;
    }

    if (readerMode) {
        scriptEngine->executerScript(view, PageEnhancements::readerModeEnableScript());
    }
    if (forceDarkMode) {
        scriptEngine->executerScript(view, PageEnhancements::darkModeEnableScript());
    } else {
        scriptEngine->executerScript(view, PageEnhancements::darkModeDisableScript());
    }
}

void Browser::toggleForceDarkMode() {
    if (suppressEnhancementSignals) {
        return;
    }
    const bool on = darkModeButton && GTK_IS_TOGGLE_BUTTON(darkModeButton) &&
                    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(darkModeButton));
    forceDarkMode = on;
    updateEnhancementButtons();
    applyPageEnhancements();
    persistSession();
}

void Browser::toggleReaderMode() {
    if (suppressEnhancementSignals) {
        return;
    }
    const bool on = readerModeButton && GTK_IS_TOGGLE_BUTTON(readerModeButton) &&
                    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(readerModeButton));
    const bool wasOn = readerMode;
    readerMode = on;
    updateEnhancementButtons();

    if (readerMode) {
        applyPageEnhancements();
    } else if (wasOn && activeTab && activeTab->webView && WEBKIT_IS_WEB_VIEW(activeTab->webView)) {
        // Sortir du mode lecture en rechargeant la page d'origine
        webkit_web_view_reload(activeTab->webView);
    }
    persistSession();
}

void Browser::toggleCurrentPageFavorite() {
    const std::string url = getCurrentURL();
    if (url.empty() || !favorites) {
        return;
    }
    if (FavoritesJson::containsUrlRecursive(*favorites, url)) {
        FavoritesJson::removeByUrlRecursive(*favorites, url);
        FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
    } else {
        std::string nom = getCurrentTitle();
        if (nom.empty() || nom == "Titre inconnu" || nom == "Nouvel onglet") {
            nom = "Favori";
        }
        addFavorite(nom, url, "Général");
        updateStarButton();
        return;
    }
    refreshFavoritesBar();
    updateStarButton();
}

void Browser::focusUrlBar() {
    if (!urlBar || !GTK_IS_WIDGET(urlBar)) {
        return;
    }
    gtk_widget_grab_focus(urlBar);
    if (GTK_IS_EDITABLE(urlBar)) {
        gtk_editable_select_region(GTK_EDITABLE(urlBar), 0, -1);
    }
}

void Browser::requestHomepageSearchFocus() {
    pendingHomepageSearchFocus = true;
}

bool Browser::consumeHomepageSearchFocus() {
    if (!pendingHomepageSearchFocus) {
        return false;
    }
    pendingHomepageSearchFocus = false;
    return true;
}

void Browser::focusHomepageSearchBox(WebKitWebView* webView) {
    if (!webView || !WEBKIT_IS_WEB_VIEW(webView) || !GTK_IS_WIDGET(webView)) {
        return;
    }

    // Donner le focus clavier au WebView (pas à la barre d'URL)
    gtk_widget_grab_focus(GTK_WIDGET(webView));

    // Cibler le champ de recherche de la page (Google, DuckDuckGo, Bing, etc.)
    static const char* kFocusSearchJs =
        "(function(){"
        "  function visible(el){"
        "    if(!el) return false;"
        "    var s=window.getComputedStyle(el);"
        "    return s.display!=='none' && s.visibility!=='hidden' && el.offsetParent!==null;"
        "  }"
        "  function tryFocus(){"
        "    var selectors=["
        "      '#search_form_input_homepage',"
        "      '#searchbox_input',"
        "      'input[name=\"q\"]',"
        "      'textarea[name=\"q\"]',"
        "      'input[type=\"search\"]',"
        "      'form[role=\"search\"] input:not([type=\"hidden\"])',"
        "      'form[action*=\"search\"] input:not([type=\"hidden\"])',"
        "      '[role=\"combobox\"]',"
        "      'input[id*=\"search\" i]',"
        "      'input[aria-label*=\"Search\" i]',"
        "      'input[aria-label*=\"Recherche\" i]',"
        "      'input[placeholder*=\"Search\" i]',"
        "      'input[placeholder*=\"Recherche\" i]',"
        "      'input[placeholder*=\"search\" i]'"
        "    ];"
        "    for(var i=0;i<selectors.length;i++){"
        "      try{"
        "        var el=document.querySelector(selectors[i]);"
        "        if(el && visible(el)){"
        "          el.focus({preventScroll:false});"
        "          try{el.click();}catch(e){}"
        "          try{el.select&&el.select();}catch(e){}"
        "          return true;"
        "        }"
        "      }catch(e){}"
        "    }"
        "    var inputs=document.querySelectorAll('input[type=\"text\"],input:not([type]),textarea');"
        "    var best=null,bestArea=0;"
        "    for(var j=0;j<inputs.length;j++){"
        "      var n=inputs[j];"
        "      if(!visible(n) || n.disabled || n.readOnly) continue;"
        "      var r=n.getBoundingClientRect();"
        "      var area=r.width*r.height;"
        "      if(area>bestArea && r.width>80){best=n;bestArea=area;}"
        "    }"
        "    if(best){"
        "      best.focus({preventScroll:false});"
        "      try{best.click();}catch(e){}"
        "      try{best.select&&best.select();}catch(e){}"
        "      return true;"
        "    }"
        "    return false;"
        "  }"
        "  if(tryFocus()) return;"
        "  setTimeout(tryFocus,250);"
        "  setTimeout(tryFocus,700);"
        "  setTimeout(tryFocus,1400);"
        "})();";

    webkit_web_view_evaluate_javascript(webView, kFocusSearchJs, -1, nullptr, nullptr, nullptr, nullptr, nullptr);

    // Re-focus WebView après un court délai (certains sites volent le focus au chargement)
    g_timeout_add(400, +[](gpointer data) -> gboolean {
        auto* view = WEBKIT_WEB_VIEW(data);
        if (view && WEBKIT_IS_WEB_VIEW(view) && GTK_IS_WIDGET(view)) {
            gtk_widget_grab_focus(GTK_WIDGET(view));
            webkit_web_view_evaluate_javascript(view,
                "(function(){var e=document.activeElement;"
                "if(!e||(e.tagName!=='INPUT'&&e.tagName!=='TEXTAREA'&&e.getAttribute('role')!=='combobox')){"
                "var s=document.querySelector('#searchbox_input,#search_form_input_homepage,input[name=\"q\"],textarea[name=\"q\"],input[type=\"search\"]');"
                "if(s){s.focus();try{s.select&&s.select();}catch(x){}}"
                "}})();",
                -1, nullptr, nullptr, nullptr, nullptr, nullptr);
        }
        return FALSE;
    }, webView);
}


void Browser::initializeFavoritesPopover() {
    //favoritesPopover = gtk_popover_new(favoritesBar); // Attaché à la barre de favorites
    favoritesPopover = gtk_popover_new(navigationBar); // Attaché à la barre
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Champs de saisie
    favoriteNameEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(favoriteNameEntry), "Nom du favori");

    favoriteUrlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(favoriteUrlEntry), "URL du favori");

    // Boutton de validation
    GtkWidget* boutonAjouter = gtk_button_new_with_label("Ajouter Favori");
    g_signal_connect(boutonAjouter, "clicked", G_CALLBACK(on_button_ajouter_clicked), this);

    // Ajout des éléments dans la boîte
    gtk_box_pack_start(GTK_BOX(box), favoriteNameEntry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), favoriteUrlEntry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), boutonAjouter, FALSE, FALSE, 5);
    
    // Ajouter le contenu au popover
    gtk_container_add(GTK_CONTAINER(favoritesPopover), box);
    // Ne pas afficher le popover automatiquement - il sera affiché uniquement quand l'utilisateur le demande
    // gtk_widget_show_all(favoritesPopover); // Retiré pour éviter l'affichage au démarrage
}


void Browser::initializeTabsBar() {
    // Nettoyer l'ancienne barre d'tabs si elle existe
    // CRITIQUE : Vérifier que tabsBar est valide avant toute manipulation
    if (tabsBar) {
        // Vérifier que c'est un widget GTK valide et qu'il n'est pas déjà en destruction
        if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
            // Retirer du container avant de détruire
            GtkWidget* parent = gtk_widget_get_parent(tabsBar);
            if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                // Vérifier à nouveau que le widget est toujours valide
                if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
                    gtk_container_remove(GTK_CONTAINER(parent), tabsBar);
                }
            }
            // Vérifier une dernière fois avant destruction
            if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
                gtk_widget_destroy(tabsBar);
            }
        }
        // Toujours réinitialiser le pointeur après tentative de destruction
        tabsBar = nullptr;
    }

    // Créer un ScrolledWindow pour rendre les onglets scrollables
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_NONE);
    gtk_widget_set_name(scrolledWindow, "scrolled-tabs");
    
    // Créer une nouvelle barre d'tabs avec container visible
    tabsBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_start(tabsBar, 5);
    gtk_widget_set_margin_end(tabsBar, 5);
    gtk_widget_set_margin_top(tabsBar, 5);
    gtk_widget_set_margin_bottom(tabsBar, 2);
    gtk_widget_set_name(tabsBar, "barre-tabs");
    
    // Ajouter la barre d'onglets au ScrolledWindow
    gtk_container_add(GTK_CONTAINER(scrolledWindow), tabsBar);
    
    // Bouton + pour nouvel onglet (les groupes sont dans la barre de favoris)
    GtkWidget* boutonAjouterOngletInit = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(boutonAjouterOngletInit, "Nouvel onglet");
    gtk_widget_set_name(boutonAjouterOngletInit, "button-ajouter-onglet");
    g_signal_connect(boutonAjouterOngletInit, "clicked", G_CALLBACK(on_ajouter_onglet), this);
    gtk_box_pack_start(GTK_BOX(tabsBar), boutonAjouterOngletInit, FALSE, FALSE, 0);
    gtk_widget_show_all(boutonAjouterOngletInit);
    
    // Les tabs seront ajoutés ici (au milieu)
    // Le bouton "+" sera ajouté dynamiquement après chaque onglet dans addNewTab()
    
    // Ajouter le ScrolledWindow au container principal (en haut)
    if (mainContainer && !gtk_widget_get_parent(scrolledWindow)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), scrolledWindow, FALSE, FALSE, 0);
    }
    
    // Connecter le signal scroll-event pour permettre le changement d'onglet avec la molette
    g_signal_connect(scrolledWindow, "scroll-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventScroll* event, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        if (navigateur->tabs.empty() || !navigateur->activeTab) return FALSE;
        
        // Trouver l'onglet actif
        int currentIndex = -1;
        for (size_t i = 0; i < navigateur->tabs.size(); ++i) {
            if (navigateur->tabs[i].tabWidget == navigateur->activeTab->tabWidget) {
                currentIndex = i;
                break;
            }
        }
        
        if (currentIndex == -1) return FALSE;
        
        // Changer d'onglet selon la direction du scroll
        if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_SMOOTH) {
            if (event->delta_y > 0 && currentIndex < static_cast<int>(navigateur->tabs.size()) - 1) {
                navigateur->changeActiveTab(navigateur->tabs[currentIndex + 1].tabWidget);
                return TRUE;
            }
        } else if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_SMOOTH) {
            if (event->delta_y < 0 && currentIndex > 0) {
                navigateur->changeActiveTab(navigateur->tabs[currentIndex - 1].tabWidget);
                return TRUE;
            }
        }
        
        return FALSE;
    }), this);
}

void Browser::changeTabGroup(const std::string& groupName) {
    if (!tabsManager->groupeExiste(groupName)) {
        tabsManager->ajouterGroupe(groupName);
    }
    tabsManager->changerGroupeActif(groupName);
    updateGroupChip();
    refreshTabsBarVisibility();

    // Activer le premier onglet visible du groupe, ou créer un nouvel onglet
    TabData* firstVisible = nullptr;
    for (auto& tab : tabs) {
        if (tab.groupName == groupName && tab.tabWidget) {
            firstVisible = &tab;
            break;
        }
    }
    if (firstVisible) {
        changeActiveTab(firstVisible->tabWidget);
    } else {
        addNewTab(homepage.empty() ? "https://www.duckduckgo.com" : homepage);
    }
}

void Browser::updateGroupChip() {
    if (groupChipLabel && GTK_IS_LABEL(groupChipLabel)) {
        gtk_label_set_text(GTK_LABEL(groupChipLabel), tabsManager->getGroupeActif().c_str());
    }
    if (groupChip && GTK_IS_WIDGET(groupChip)) {
        gtk_widget_queue_draw(groupChip);
    }
}

void Browser::refreshTabsBarVisibility() {
    const std::string actif = tabsManager->getGroupeActif();
    for (auto& tab : tabs) {
        if (!tab.tabWidget || !GTK_IS_WIDGET(tab.tabWidget)) {
            continue;
        }
        if (tab.groupName == actif) {
            gtk_widget_show(tab.tabWidget);
        } else {
            gtk_widget_hide(tab.tabWidget);
        }
    }
}

void Browser::moveTabToGroup(GtkWidget* tabWidget, const std::string& groupName) {
    for (auto& tab : tabs) {
        if (tab.tabWidget != tabWidget) {
            continue;
        }
        if (!tabsManager->groupeExiste(groupName)) {
            tabsManager->ajouterGroupe(groupName);
        }
        tabsManager->removeTab(tab.groupName, tab.url);
        tab.groupName = groupName;
        tabsManager->ajouterOnglet(groupName, tab.url);
        refreshTabsBarVisibility();
        if (groupName == tabsManager->getGroupeActif()) {
            changeActiveTab(tab.tabWidget);
        }
        return;
    }
}

void Browser::deleteTabGroup(const std::string& groupName) {
    if (groupName == "Par défaut") {
        return;
    }
    for (auto& tab : tabs) {
        if (tab.groupName == groupName) {
            tab.groupName = "Par défaut";
            tabsManager->ajouterOnglet("Par défaut", tab.url);
        }
    }
    tabsManager->supprimerGroupe(groupName);
    changeTabGroup("Par défaut");
}

void Browser::showTabContextMenu(GtkWidget* tabWidget, GdkEventButton* event) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab) {
        return;
    }

    const std::string tabUrl = getTabUrl(*tab);
    const bool inFavorites = !tabUrl.empty() && favorites &&
                             FavoritesJson::containsUrlRecursive(*favorites, tabUrl);

    GtkWidget* menu = gtk_menu_new();
    auto appendItem = [&](const char* label, GCallback cb) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        auto* payload = new std::pair<Browser*, GtkWidget*>(this, tabWidget);
        g_signal_connect_data(item, "activate", cb, payload, free_tab_widget_payload, (GConnectFlags)0);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    appendItem("Fermer l'onglet", G_CALLBACK(on_tab_menu_close));
    appendItem("Dupliquer l'onglet", G_CALLBACK(on_tab_menu_duplicate));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    appendItem("Renommer l'onglet…", G_CALLBACK(on_tab_menu_rename));
    if (tab->titleLocked) {
        appendItem("Utiliser le titre de la page", G_CALLBACK(on_tab_menu_reset_title));
    }
    appendItem(tab->pinned ? "Désépingler l'onglet" : "Épingler l'onglet",
               G_CALLBACK(on_tab_menu_toggle_pin));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    if (inFavorites) {
        appendItem("Modifier le favori…", G_CALLBACK(on_tab_menu_edit_favorite));
        appendItem("Supprimer le favori", G_CALLBACK(on_tab_menu_remove_favorite));
    } else if (!tabUrl.empty()) {
        appendItem("Ajouter aux favoris", G_CALLBACK(on_tab_menu_add_favorite));
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* moveHeader = gtk_menu_item_new_with_label("Déplacer vers le groupe");
    gtk_widget_set_sensitive(moveHeader, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), moveHeader);

    for (const auto& groupe : tabsManager->getGroupes()) {
        GtkWidget* item = gtk_menu_item_new_with_label(groupe.c_str());
        auto* payload = new std::pair<Browser*, std::pair<GtkWidget*, std::string>>(this, {tabWidget, groupe});
        g_signal_connect_data(item, "activate", G_CALLBACK(on_move_tab_to_group_activate), payload,
                              free_move_tab_payload, (GConnectFlags)0);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    GtkWidget* newGroup = gtk_menu_item_new_with_label("Nouveau groupe…");
    g_signal_connect(newGroup, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        static_cast<Browser*>(user_data)->showGroupsMenu();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), newGroup);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
}

TabData* Browser::findTabByWidget(GtkWidget* tabWidget) {
    for (auto& tab : tabs) {
        if (tab.tabWidget == tabWidget) {
            return &tab;
        }
    }
    return nullptr;
}

std::string Browser::getTabUrl(const TabData& tab) const {
    if (!tab.url.empty()) {
        return tab.url;
    }
    if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
        const gchar* uri = webkit_web_view_get_uri(tab.webView);
        if (uri && uri[0] != '\0') {
            return uri;
        }
    }
    return "";
}

std::string Browser::getTabDisplayTitle(const TabData& tab) const {
    if (tab.titleLocked && !tab.customTitle.empty()) {
        return tab.customTitle;
    }
    if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
        const gchar* title = webkit_web_view_get_title(tab.webView);
        if (title && title[0] != '\0') {
            return title;
        }
    }
    return "Nouvel onglet";
}

void Browser::updateTabLabel(TabData& tab) {
    if (!tab.label || !GTK_IS_LABEL(tab.label)) {
        return;
    }
    std::string titre = getTabDisplayTitle(tab);
    if (titre.length() > 24) {
        titre = titre.substr(0, 21) + "...";
    }
    gtk_label_set_text(GTK_LABEL(tab.label), titre.c_str());
    if (tab.tabWidget && GTK_IS_WIDGET(tab.tabWidget)) {
        gtk_widget_set_tooltip_text(tab.tabWidget, getTabDisplayTitle(tab).c_str());
    }
}

void Browser::reorderTabsBar() {
    if (!tabsBar) {
        return;
    }
    int pos = 0;
    for (auto& t : tabs) {
        if (t.pinned && t.tabWidget && GTK_IS_WIDGET(t.tabWidget)) {
            gtk_box_reorder_child(GTK_BOX(tabsBar), t.tabWidget, pos++);
        }
    }
    for (auto& t : tabs) {
        if (!t.pinned && t.tabWidget && GTK_IS_WIDGET(t.tabWidget)) {
            gtk_box_reorder_child(GTK_BOX(tabsBar), t.tabWidget, pos++);
        }
    }
}

void Browser::duplicateTab(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab) {
        return;
    }
    const std::string url = getTabUrl(*tab);
    if (!url.empty()) {
        addNewTab(url);
    }
}

void Browser::renameTab(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab || !window) {
        return;
    }

    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "Renommer l'onglet",
        GTK_WINDOW(window),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "Annuler", GTK_RESPONSE_CANCEL,
        "Enregistrer", GTK_RESPONSE_ACCEPT,
        nullptr);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 6);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), getTabDisplayTitle(*tab).c_str());
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), box);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        const gchar* text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && *text) {
            tab->customTitle = text;
            tab->titleLocked = true;
            updateTabLabel(*tab);
        }
    }
    gtk_widget_destroy(dlg);
}

void Browser::resetTabTitle(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab) {
        return;
    }
    tab->customTitle.clear();
    tab->titleLocked = false;
    updateTabLabel(*tab);
}

void Browser::togglePinTab(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab || !tab->tabWidget) {
        return;
    }
    tab->pinned = !tab->pinned;
    GtkStyleContext* ctx = gtk_widget_get_style_context(tab->tabWidget);
    if (tab->pinned) {
        gtk_style_context_add_class(ctx, "onglet-epingle");
    } else {
        gtk_style_context_remove_class(ctx, "onglet-epingle");
    }
    reorderTabsBar();
}

void Browser::addTabToFavorites(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab || !favorites) {
        return;
    }
    const std::string url = getTabUrl(*tab);
    if (url.empty()) {
        return;
    }
    if (FavoritesJson::containsUrlRecursive(*favorites, url)) {
        editTabFavorite(tabWidget);
        return;
    }
    std::string nom = getTabDisplayTitle(*tab);
    if (nom.empty() || nom == "Nouvel onglet") {
        nom = url;
    }
    addFavorite(nom, url, "Général");
    updateStarButton();
}

void Browser::editTabFavorite(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab || !favorites || !favoritesManager) {
        return;
    }
    const std::string url = getTabUrl(*tab);
    if (url.empty()) {
        return;
    }
    std::string favName;
    std::string favUrl;
    if (!FavoritesJson::findByUrlRecursive(*favorites, url, favName, &favUrl)) {
        return;
    }
    favoritesManager->promptRename(favName, favUrl.empty() ? url : favUrl, false);
    updateStarButton();
}

void Browser::removeTabFavorite(GtkWidget* tabWidget) {
    TabData* tab = findTabByWidget(tabWidget);
    if (!tab || !favorites) {
        return;
    }
    const std::string url = getTabUrl(*tab);
    if (url.empty()) {
        return;
    }
    std::string favName;
    if (!FavoritesJson::findByUrlRecursive(*favorites, url, favName, nullptr)) {
        return;
    }
    if (!confirmAction("Supprimer le favori", "Supprimer « " + favName + " » des favoris ?")) {
        return;
    }
    FavoritesJson::removeByUrlRecursive(*favorites, url);
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
    refreshFavoritesBar();
    updateStarButton();
}

void Browser::addNewTab(const std::string &url) {
    const std::string groupeCourant = tabsManager->getGroupeActif();
    tabsManager->ajouterOnglet(groupeCourant, url);

    // Créer un container visible pour l'onglet avec style
    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(hboxOnglet, 2);
    gtk_widget_set_margin_end(hboxOnglet, 2);
    gtk_widget_set_margin_top(hboxOnglet, 2);
    gtk_widget_set_margin_bottom(hboxOnglet, 2);
    gtk_widget_set_name(hboxOnglet, "onglet");

    // Favicon (placeholder lettre, remplacé via notify::favicon)
    GtkWidget *faviconImage = create_letter_icon("N", 14);
    gtk_widget_set_margin_start(faviconImage, 4);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), faviconImage, FALSE, FALSE, 0);
    
    // Label avec le titre de l'onglet (ou "Nouvel onglet" par défaut)
    GtkWidget *labelTitre = gtk_label_new("Nouvel onglet");
    gtk_label_set_ellipsize(GTK_LABEL(labelTitre), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(labelTitre, 4);
    gtk_widget_set_margin_end(labelTitre, 4);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, TRUE, TRUE, 0);

    // Bouton fermer (×) à droite
    GtkWidget *boutonFermer = gtk_button_new_with_label("×");
    gtk_widget_set_tooltip_text(boutonFermer, "Fermer l'onglet");
    gtk_widget_set_margin_start(boutonFermer, 2);
    gtk_widget_set_margin_end(boutonFermer, 2);
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer user_data) -> gboolean {
        auto* n = static_cast<Browser*>(user_data);
        // Trouver le container parent (hboxOnglet) - remonter de 1 niveau
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        if (parent) {
            n->removeTab(parent);
        }
        return TRUE; // Empêcher la propagation
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    // Rendre l'onglet cliquable pour changer d'onglet actif
    gtk_widget_set_events(hboxOnglet, GDK_BUTTON_PRESS_MASK);
    // Définir le curseur pointer en code (GTK CSS ne supporte pas cursor)
    // Attendre que le widget soit réalisé pour définir le curseur
    g_signal_connect(hboxOnglet, "realize", G_CALLBACK(+[](GtkWidget* widget, gpointer) {
        GdkWindow* window = gtk_widget_get_window(widget);
        if (window) {
            GdkDisplay* display = gtk_widget_get_display(widget);
            if (display) {
                GdkCursor* cursor = gdk_cursor_new_from_name(display, "pointer");
                if (cursor) {
                    gdk_window_set_cursor(window, cursor);
                    g_object_unref(cursor);
                }
            }
        }
    }), nullptr);
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, gpointer user_data) -> gboolean {
        auto* n = static_cast<Browser*>(user_data);
        if (!n) {
            return FALSE;
        }
        if (event->button == GDK_BUTTON_MIDDLE) {
            TabData* tab = n->findTabByWidget(widget);
            if (tab && tab->pinned) {
                return TRUE;
            }
            n->removeTab(widget);
            return TRUE;
        }
        if (event->button == GDK_BUTTON_SECONDARY) {
            n->changeActiveTab(widget);
            n->showTabContextMenu(widget, event);
            return TRUE;
        }
        if (event->button == GDK_BUTTON_PRIMARY) {
            n->changeActiveTab(widget);
            return TRUE;
        }
        return FALSE;
    }), this);

    // Ajouter l'onglet à la barre d'tabs (après le button groupes)
    if (tabsBar && !gtk_widget_get_parent(hboxOnglet)) {
        // Retirer le bouton "+" s'il existe déjà pour le réinsérer après le nouvel onglet
        GList* children = gtk_container_get_children(GTK_CONTAINER(tabsBar));
        GtkWidget* boutonAjouterOnglet = nullptr;
        
        // Chercher le bouton "+" dans les enfants
        for (GList* iter = children; iter != nullptr; iter = iter->next) {
            GtkWidget* widget = GTK_WIDGET(iter->data);
            const gchar* name = gtk_widget_get_name(widget);
            if (name && strcmp(name, "button-ajouter-onglet") == 0) {
                boutonAjouterOnglet = widget;
                g_object_ref(boutonAjouterOnglet);
                gtk_container_remove(GTK_CONTAINER(tabsBar), widget);
                break;
            }
        }
        g_list_free(children);
        
        // Ajouter le nouvel onglet
        gtk_box_pack_start(GTK_BOX(tabsBar), hboxOnglet, FALSE, FALSE, 0);
        
        // Réinsérer le bouton "+" juste après le nouvel onglet (pas à l'extrême droite)
        if (boutonAjouterOnglet && GTK_IS_WIDGET(boutonAjouterOnglet)) {
            gtk_box_pack_start(GTK_BOX(tabsBar), boutonAjouterOnglet, FALSE, FALSE, 0);
            g_object_unref(boutonAjouterOnglet);
        } else {
            // Créer le bouton "+" s'il n'existe pas encore
            boutonAjouterOnglet = gtk_button_new_with_label("+");
            gtk_widget_set_tooltip_text(boutonAjouterOnglet, "Ajouter un nouvel onglet");
            gtk_widget_set_margin_start(boutonAjouterOnglet, 2);
            gtk_widget_set_margin_end(boutonAjouterOnglet, 2);
            gtk_widget_set_name(boutonAjouterOnglet, "button-ajouter-onglet");
            g_signal_connect(boutonAjouterOnglet, "clicked", G_CALLBACK(on_ajouter_onglet), this);
            gtk_box_pack_start(GTK_BOX(tabsBar), boutonAjouterOnglet, FALSE, FALSE, 0);
            gtk_widget_show_all(boutonAjouterOnglet);
        }
    }
    // Normaliser l'URL (ne pas préfixer data:/about:/file:)
    std::string urlNormalisee = normalizeNavigationUrl(url);
    
    // Créer la WebView (réutilise le préchargement homepage si possible)
    WebKitWebView* newWebView = takePreloadedHomeView(urlNormalisee);
    if (!newWebView) {
        newWebView = createPersistentWebView();
    }
    
    // Vérifier que la WebView est correctement créée
    if (!newWebView) {
        std::cerr << "[ERREUR] webkit_web_view_new() a retourné NULL" << std::endl;
        return;
    }
    
    // Vérifier que c'est bien une WebView valide
    if (!WEBKIT_IS_WEB_VIEW(newWebView)) {
        std::cerr << "[ERREUR] L'objet créé n'est pas une WebView valide" << std::endl;
        g_object_unref(newWebView);
        return;
    }
    
    // Vérifier que c'est un widget GTK valide
    if (!GTK_IS_WIDGET(newWebView)) {
        std::cerr << "[ERREUR] WebView n'est pas un widget GTK valide" << std::endl;
        g_object_unref(newWebView);
        return;
    }
    
    GtkWidget* webWidget = GTK_WIDGET(newWebView);
    
    // Garder la console des pages silencieuse par défaut : les warnings CSP/preload
    // viennent souvent du site ou de WebKit, pas du shell du navigateur.
    WebKitSettings* settings = webkit_web_view_get_settings(newWebView);
    if (settings) {
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, isDebugLoggingEnabled());
        // Activer JavaScript (devrait être activé par défaut)
        webkit_settings_set_enable_javascript(settings, TRUE);
        // Note: webkit_settings_set_enable_plugins est déprécié et ne fait rien
    }
    
    // Donner un nom CSS à la WebView pour le debug visuel
    gtk_widget_set_name(webWidget, "webkit-webview");
    
    // Configuration de base de la WebView
    gtk_widget_set_vexpand(webWidget, TRUE);
    gtk_widget_set_hexpand(webWidget, TRUE);
    
    // CRITIQUE : S'assurer que la WebView a une taille minimale valide
    // WebKit nécessite une taille valide pour initialiser le contexte de rendu
    // Ne pas fixer de taille minimale pour permettre le redimensionnement complet
    gtk_widget_set_size_request(webWidget, 1, 1);
    
    // Créer la structure TabData AVANT d'ajouter au container
    TabData tabData;
    tabData.url = urlNormalisee;
    tabData.tabWidget = hboxOnglet;
    tabData.webView = newWebView;
    tabData.label = labelTitre;
    tabData.faviconImage = faviconImage;
    tabData.groupName = groupeCourant;
    
    // Ajouter l'onglet à la liste
    tabs.push_back(tabData);
    
    // IMPORTANT : Ne PAS ajouter la WebView au container ici
    // Laisser changeActiveTab gérer l'ajout/retrait des WebViews
    // Cela évite d'invalider la WebView active qui pourrait être en cours d'utilisation
    
    // Juste s'assurer que la WebView n'est pas déjà dans un autre container
    GtkWidget* oldParent = gtk_widget_get_parent(webWidget);
    if (oldParent && oldParent != webContainer) {
        // Ne pas retirer ici, changeActiveTab le fera de manière sécurisée
        WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView a un parent différent, sera géré par changeActiveTab");
    }
    
    // Afficher l'onglet
    gtk_widget_show_all(hboxOnglet);
    
    // Forcer l'affichage de la WebView et du container
    if (webContainer) {
        gtk_widget_show_all(webContainer);
        gtk_widget_queue_draw(webContainer);
        
        // CRITIQUE : Attendre que le container soit réalisé et obtenir sa taille
        // Utiliser un timeout pour forcer le redimensionnement après que tout soit affiché
        g_timeout_add(100, [](gpointer user_data) -> gboolean {
            auto* browser = static_cast<Browser*>(user_data);
            if (browser && browser->webContainer) {
                int containerWidth = gtk_widget_get_allocated_width(browser->webContainer);
                int containerHeight = gtk_widget_get_allocated_height(browser->webContainer);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Container allocated size: " << containerWidth << "x" << containerHeight);
                
                // Si le container a une taille valide, redimensionner toutes les WebViews
                if (containerWidth > 100 && containerHeight > 100) {
                    for (auto& tab : browser->tabs) {
                        if (tab.webView && GTK_IS_WIDGET(tab.webView)) {
                            GtkWidget* w = GTK_WIDGET(tab.webView);
                            gtk_widget_set_size_request(w, -1, -1); // Réinitialiser
                            gtk_widget_queue_resize(w);
                            gtk_widget_queue_draw(w);
                        }
                    }
                }
            }
            return FALSE; // Ne pas répéter
        }, this);
    }
    gtk_widget_show_all(webWidget);
    gtk_widget_queue_draw(webWidget);
    
    // Connecter le signal pour mettre à jour le titre de l'onglet quand la page se charge
    // IMPORTANT: Utiliser un pointeur stable (l'index dans le vector) au lieu de &tabs.back()
    // car &tabs.back() peut devenir invalide si le vector est réalloué
    if (WEBKIT_IS_WEB_VIEW(newWebView) && G_IS_OBJECT(newWebView)) {
        // Stocker l'index de l'onglet dans le vector
        size_t tabIndex = tabs.size() - 1;
        
        g_signal_connect(newWebView, "notify::title", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer user_data) {
            auto* browser = static_cast<Browser*>(user_data);
            if (!browser) return;
            
            WebKitWebView* webView = WEBKIT_WEB_VIEW(obj);
            for (auto& tab : browser->tabs) {
                if (tab.webView == webView && tab.label && GTK_IS_LABEL(tab.label)) {
                    const gchar* title = webkit_web_view_get_title(tab.webView);
                    const gchar* uri = webkit_web_view_get_uri(tab.webView);
                    if (!tab.titleLocked) {
                        browser->updateTabLabel(tab);
                    }
                    if (uri) {
                        tab.url = uri;
                        browser->recordHistoryVisit(uri, title ? title : "");
                        if (&tab == browser->activeTab) {
                            browser->updateStarButton();
                            browser->refreshUrlCompletionModel();
                        }
                    }
                    break;
                }
            }
        }), this);

        g_signal_connect(newWebView, "notify::favicon", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer user_data) {
            auto* browser = static_cast<Browser*>(user_data);
            if (!browser) return;
            WebKitWebView* webView = WEBKIT_WEB_VIEW(obj);
            cairo_surface_t* surface = webkit_web_view_get_favicon(webView);
            if (!surface) return;
            for (auto& tab : browser->tabs) {
                if (tab.webView != webView || !tab.faviconImage || !GTK_IS_IMAGE(tab.faviconImage)) {
                    continue;
                }
                const int sw = cairo_image_surface_get_width(surface);
                const int sh = cairo_image_surface_get_height(surface);
                if (sw <= 0 || sh <= 0) break;
                GdkPixbuf* full = gdk_pixbuf_get_from_surface(surface, 0, 0, sw, sh);
                if (!full) break;
                GdkPixbuf* scaled = gdk_pixbuf_scale_simple(full, 14, 14, GDK_INTERP_BILINEAR);
                g_object_unref(full);
                if (scaled) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(tab.faviconImage), scaled);
                    g_object_unref(scaled);
                }
                break;
            }
        }), this);

        start_favicon_load(faviconImage, urlNormalisee);
    } else {
        std::cerr << "[ERREUR] Impossible de connecter le signal notify::title - WebView invalide" << std::endl;
    }
    
    // Changer l'onglet actif vers celui-ci (cela affichera la WebView et chargera l'URL)
    changeActiveTab(hboxOnglet);
    refreshTabsBarVisibility();
    updateGroupChip();
    persistSession();

    gtk_widget_show_all(tabsBar);
}

void Browser::changeActiveTab(GtkWidget* tabWidget) {
    for (auto &tab : tabs) {
        if (tab.tabWidget == tabWidget) {
            // Mettre à jour l'onglet actif
            activeTab = &tab;
            
            // Vérifier que la WebView est valide AVANT toute manipulation
            if (!tab.webView || !WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide dans changeActiveTab" << std::endl;
                return;
            }
            
            GtkWidget* webWidget = GTK_WIDGET(tab.webView);
            
            // CRITIQUE : Cacher toutes les autres WebViews AVANT de manipuler le container
            // Cela évite les problèmes de rendu pendant le changement d'onglet
            for (auto &otherTab : tabs) {
                if (otherTab.webView && GTK_IS_WIDGET(otherTab.webView) && &otherTab != &tab) {
                    gtk_widget_hide(GTK_WIDGET(otherTab.webView));
                }
            }
            
            // Retirer uniquement les autres WebViews du container, PAS celle qu'on veut afficher
            // Faire cela APRÈS avoir caché les autres pour éviter les problèmes de rendu
            if (webContainer) {
                GList* children = gtk_container_get_children(GTK_CONTAINER(webContainer));
                GList* toRemove = nullptr;
                
                // D'abord, collecter les WebViews à retirer (ne pas les retirer pendant l'itération)
                for (GList* iter = children; iter != nullptr; iter = iter->next) {
                    GtkWidget* child = GTK_WIDGET(iter->data);
                    // Ne retirer que les WebViews qui ne sont pas celle qu'on veut afficher
                    if (GTK_IS_WIDGET(child) && child != webWidget && WEBKIT_IS_WEB_VIEW(child)) {
                        // Vérifier que ce n'est pas une WebView active en cours d'utilisation
                        bool isActive = false;
                        for (auto &otherTab : tabs) {
                            if (otherTab.webView && GTK_WIDGET(otherTab.webView) == child && &otherTab == activeTab) {
                                isActive = true;
                                break;
                            }
                        }
                        if (!isActive) {
                            toRemove = g_list_prepend(toRemove, child);
                        }
                    }
                }
                g_list_free(children);
                
                // Maintenant retirer les WebViews collectées
                // CRITIQUE : Vérifier que chaque widget est valide avant de le retirer
                for (GList* iter = toRemove; iter != nullptr; iter = iter->next) {
                    GtkWidget* child = GTK_WIDGET(iter->data);
                    // Vérifier que le widget est valide, qu'il a le bon parent, et qu'il n'est pas en destruction
                    if (GTK_IS_WIDGET(child) && 
                        !gtk_widget_in_destruction(child) &&
                        gtk_widget_get_parent(child) == webContainer) {
                        // Vérifier à nouveau après avoir obtenu le parent
                        if (GTK_IS_WIDGET(child) && !gtk_widget_in_destruction(child)) {
                            gtk_container_remove(GTK_CONTAINER(webContainer), child);
                        }
                    }
                }
                g_list_free(toRemove);
            }
            
            // Afficher la WebView de l'onglet actif
            // Vérifier à nouveau que la WebView est toujours valide après les manipulations
            if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView)) {
                std::cerr << "[ERREUR] WebView devenue invalide après manipulation du container" << std::endl;
                return;
            }
            
            // 1. S'assurer que le container web est visible et expansible
            if (webContainer) {
                gtk_widget_show_all(webContainer);
                gtk_widget_set_visible(webContainer, TRUE);
                gtk_widget_set_vexpand(webContainer, TRUE);
                gtk_widget_set_hexpand(webContainer, TRUE);
            }
            
            // 2. CRITIQUE : Ajouter UNIQUEMENT la WebView active au container
            // (on a déjà retiré toutes les autres ci-dessus)
            if (webContainer) {
                // Vérifier où se trouve la WebView actuellement
                GtkWidget* currentParent = gtk_widget_get_parent(webWidget);
                
                if (!currentParent) {
                    // WebView n'a pas de parent, l'ajouter au container
                    gtk_box_pack_start(GTK_BOX(webContainer), webWidget, TRUE, TRUE, 0);
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView ajoutée au container lors du changement d'onglet");
                } else if (currentParent == webContainer) {
                    // WebView est déjà dans le bon container, juste s'assurer qu'elle est visible
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView déjà dans le container, forcer l'affichage...");
                    // NE PAS retirer/réajouter car cela peut invalider la WebView
                    // Juste forcer l'affichage et le redessinage
                } else {
                    // WebView est dans un autre container, la déplacer
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView dans un autre container, déplacement...");
                    // Vérifier que le parent et le widget sont valides avant retrait
                    if (GTK_IS_CONTAINER(currentParent) && GTK_IS_WIDGET(webWidget) && 
                        !gtk_widget_in_destruction(webWidget) && !gtk_widget_in_destruction(currentParent)) {
                        gtk_container_remove(GTK_CONTAINER(currentParent), webWidget);
                        
                        // Vérifier que la WebView est toujours valide après retrait
                        if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView) || 
                            gtk_widget_in_destruction(webWidget)) {
                            std::cerr << "[ERREUR] WebView devenue invalide après retrait du parent" << std::endl;
                            return;
                        }
                        
                        // Vérifier à nouveau avant d'ajouter au nouveau container
                        if (GTK_IS_WIDGET(webWidget) && !gtk_widget_in_destruction(webWidget)) {
                            gtk_box_pack_start(GTK_BOX(webContainer), webWidget, TRUE, TRUE, 0);
                        } else {
                            std::cerr << "[ERREUR] WebView invalide avant ajout au container" << std::endl;
                            return;
                        }
                    } else {
                        std::cerr << "[ERREUR] Parent ou WebView invalide avant déplacement" << std::endl;
                        return;
                    }
                }
            }
            
            // 3. Afficher la WebView IMMÉDIATEMENT et forcer sa visibilité
            // CRITIQUE : Toujours forcer l'affichage même si la WebView était déjà visible
            gtk_widget_show_all(webWidget);
            gtk_widget_set_visible(webWidget, TRUE);
            gtk_widget_set_vexpand(webWidget, TRUE);
            gtk_widget_set_hexpand(webWidget, TRUE);
            
            // Forcer le redessinage immédiat
            gtk_widget_queue_resize(webWidget);
            gtk_widget_queue_draw(webWidget);
            
            // Forcer la réalisation si nécessaire
            if (!gtk_widget_get_realized(webWidget)) {
                gtk_widget_realize(webWidget);
            }
            
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView shown: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO"));
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView realized: " << (gtk_widget_get_realized(webWidget) ? "YES" : "NO"));
            
            // 4. Charger l'URL immédiatement
            // Vérifier que la WebView est valide avant d'utiliser
            if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide avant chargement URL" << std::endl;
                return;
            }
            const gchar* currentUri = webkit_web_view_get_uri(tab.webView);
            std::string currentUrl = currentUri ? std::string(currentUri) : "";
            
            // Pages internes (HTML stub) : ne pas recharger via load_uri
            const bool internalPage = tab.url.rfind("weedly://", 0) == 0;
            const bool alreadyLoading = webkit_web_view_is_loading(tab.webView);
            const bool samePage = !currentUrl.empty() && FavoritesJson::urlsMatch(currentUrl, tab.url);
            bool needsReload = !internalPage && !alreadyLoading && !samePage &&
                (currentUrl.empty() || !FavoritesJson::urlsMatch(currentUrl, tab.url));
            
            if (needsReload) {
                WEEDLYWEB_DEBUG_LOG("[DEBUG] ========== LOADING URL ==========");
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Target URL: " << tab.url);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Current URL: " << currentUrl);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView realized: " << (gtk_widget_get_realized(webWidget) ? "YES" : "NO"));
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView visible: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO"));
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
                
                // Obtenir la taille allouée
                int width = 0, height = 0;
                if (gtk_widget_get_realized(webWidget)) {
                    width = gtk_widget_get_allocated_width(webWidget);
                    height = gtk_widget_get_allocated_height(webWidget);
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView size: " << width << "x" << height);
                }
                
                // Vérifier une dernière fois que la WebView est valide avant de charger
                if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                    std::cerr << "[ERREUR] WebView invalide avant webkit_web_view_load_uri" << std::endl;
                    return;
                }
                
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Calling webkit_web_view_load_uri...");
                webkit_web_view_load_uri(tab.webView, tab.url.c_str());
                
                // Vérifier l'URI après chargement
                const gchar* loadedUri = webkit_web_view_get_uri(tab.webView);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] URI after load_uri: " << (loadedUri ? loadedUri : "NULL"));
                
                // Forcer le redessinage après le chargement
                gtk_widget_queue_draw(webWidget);
                if (webContainer) {
                    gtk_widget_queue_draw(webContainer);
                }
                WEEDLYWEB_DEBUG_LOG("[DEBUG] ==================================");
            } else {
                WEEDLYWEB_DEBUG_LOG("[DEBUG] URL already loaded: " << currentUrl);
                // Même si l'URL est déjà chargée, forcer le redessinage et le rechargement visuel
                // Cela garantit que la page s'affiche correctement quand on revient à l'onglet
                gtk_widget_queue_resize(webWidget);
                gtk_widget_queue_draw(webWidget);
                if (webContainer) {
                    gtk_widget_queue_resize(webContainer);
                    gtk_widget_queue_draw(webContainer);
                }
                
                // Forcer un rechargement visuel même si l'URL est la même
                // Cela évite les problèmes d'affichage quand on revient à un onglet
                gtk_widget_show_all(webWidget);
                gtk_widget_set_visible(webWidget, TRUE);
            }
            
            // Forcer le traitement des événements pour s'assurer que tout est affiché
            int iterations = 0;
            while (gtk_events_pending() && iterations < 10) {
                gtk_main_iteration_do(FALSE);
                iterations++;
            }
            
            // 3. Connecter les signaux de chargement UNE SEULE FOIS
            // Vérifier que la WebView est valide avant de connecter les signaux
            if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !G_IS_OBJECT(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide, impossible de connecter les signaux" << std::endl;
                return;
            }
            
            // Utiliser un set statique pour éviter de connecter les signaux plusieurs fois
            // Note: Les WebViews sont gérées par GTK et ne doivent pas être supprimées manuellement
            // Le set est nettoyé automatiquement quand les WebViews sont détruites par GTK
            static std::set<WebKitWebView*> connectedViews;
            
            // Vérifier que la WebView est toujours valide avant de l'ajouter au set
            if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide, impossible de connecter les signaux" << std::endl;
                return;
            }
            
            if (connectedViews.find(tab.webView) == connectedViews.end()) {
                connectedViews.insert(tab.webView);
                    
                    // Handler pour load-changed
                    g_signal_connect(tab.webView, "load-changed", G_CALLBACK(+[](WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer) {
                        const gchar* uri = webkit_web_view_get_uri(web_view);
                        WEEDLYWEB_DEBUG_LOG("[DEBUG] Load event: " << load_event << " for URI: " << (uri ? uri : "NULL"));
                        
                        if (load_event == WEBKIT_LOAD_STARTED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load started, showing spinner");
                            browser_set_loading_state(true);
                        } else if (load_event == WEBKIT_LOAD_COMMITTED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load committed");
                        } else if (load_event == WEBKIT_LOAD_FINISHED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load finished, hiding spinner");
                            browser_set_loading_state(false);
                            
                            // CRITIQUE : Forcer l'affichage après chargement
                            GtkWidget* w = GTK_WIDGET(web_view);
                            gtk_widget_show_all(w);
                            gtk_widget_set_visible(w, TRUE);
                            gtk_widget_queue_draw(w);

                            // Au premier chargement de la page d'accueil : focus sur
                            // le champ de recherche du moteur (dans la page), pas la barre d'URL.
                            if (g_browser_instance && g_browser_instance->consumeHomepageSearchFocus()) {
                                g_browser_instance->focusHomepageSearchBox(web_view);
                            }
                            if (g_browser_instance) {
                                g_browser_instance->applyPageEnhancements(web_view);
                            }
                            
                            if (isDebugLoggingEnabled()) {
                                const gchar* js =
                                    "console.log('=== DIAGNOSTIC WEBVIEW ===');"
                                    "console.log('Document ready: ' + document.readyState);"
                                    "console.log('URL: ' + window.location.href);"
                                    "console.log('Title: ' + document.title);"
                                    "console.log('Body exists: ' + (document.body !== null));"
                                    "console.log('Body innerHTML length: ' + (document.body ? document.body.innerHTML.length : 0));"
                                    "console.log('Window width: ' + window.innerWidth);"
                                    "console.log('Window height: ' + window.innerHeight);";

                                webkit_web_view_evaluate_javascript(web_view, js, -1, nullptr, nullptr, nullptr,
                                    [](GObject*, GAsyncResult*, gpointer) {
                                        WEEDLYWEB_DEBUG_LOG("[DEBUG] JavaScript diagnostic executed");
                                    }, nullptr);
                            }
                            
                            // CRITIQUE : Forcer le redimensionnement et le rendu après chargement
                            // (w est déjà déclaré plus haut)
                            
                            // Obtenir la taille du container parent (qui devrait être webContainer)
                            GtkWidget* container = gtk_widget_get_parent(w);
                            if (container) {
                                int containerWidth = gtk_widget_get_allocated_width(container);
                                int containerHeight = gtk_widget_get_allocated_height(container);
                                WEEDLYWEB_DEBUG_LOG("[DEBUG] Container size after load: " << containerWidth << "x" << containerHeight);
                                
                                // Réinitialiser la taille de la WebView pour permettre l'expansion
                                if (containerWidth > 100 && containerHeight > 100) {
                                    gtk_widget_set_size_request(w, -1, -1); // -1 = utiliser la taille naturelle
                                    WEEDLYWEB_DEBUG_LOG("[DEBUG] Reset WebView size request to natural size");
                                }
                                
                                // Forcer le redessinage du container aussi
                                gtk_widget_queue_resize(container);
                                gtk_widget_queue_draw(container);
                            }
                            
                            // Forcer le redessinage complet de la WebView
                            gtk_widget_queue_resize(w);
                            gtk_widget_queue_draw(w);
                            
                            // Forcer le traitement des événements pour le rendu
                            while (gtk_events_pending()) {
                                gtk_main_iteration_do(FALSE);
                            }
                            
                            // Vérifier la taille finale
                            int finalWidth = gtk_widget_get_allocated_width(w);
                            int finalHeight = gtk_widget_get_allocated_height(w);
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Final WebView size: " << finalWidth << "x" << finalHeight);
                            
                            // Si la taille est toujours 1x1, forcer un redimensionnement avec un délai
                            if (finalWidth <= 1 || finalHeight <= 1) {
                                std::cerr << "[WARNING] WebView size is still too small, scheduling resize..." << std::endl;
                                g_timeout_add(200, [](gpointer user_data) -> gboolean {
                                    GtkWidget* widget = static_cast<GtkWidget*>(user_data);
                                    if (widget && GTK_IS_WIDGET(widget)) {
                                        gtk_widget_queue_resize(widget);
                                        gtk_widget_queue_draw(widget);
                                        WEEDLYWEB_DEBUG_LOG("[DEBUG] Forced resize after timeout");
                                    }
                                    return FALSE;
                                }, w);
                            }
                        }
                    }), nullptr);
                    
                    // Handler pour load-failed (erreurs de chargement)
                    g_signal_connect(tab.webView, "load-failed", G_CALLBACK(+[](WebKitWebView*, WebKitLoadEvent load_event, const gchar* failing_uri, GError* error, gpointer) -> gboolean {
                        // 302 / CANCELLED = navigation remplacée (onglet, stop, nouvelle URL) — pas une vraie erreur
                        if (error && error->domain == WEBKIT_NETWORK_ERROR &&
                            error->code == WEBKIT_NETWORK_ERROR_CANCELLED) {
                            browser_set_loading_state(false);
                            return TRUE;
                        }
                        std::cerr << "[ERROR] ========== LOAD FAILED ==========" << std::endl;
                        std::cerr << "[ERROR] Event: " << load_event << std::endl;
                        std::cerr << "[ERROR] URI: " << (failing_uri ? failing_uri : "NULL") << std::endl;
                        if (error) {
                            std::cerr << "[ERROR] Error code: " << error->code << std::endl;
                            std::cerr << "[ERROR] Error domain: " << g_quark_to_string(error->domain) << std::endl;
                            std::cerr << "[ERROR] Error message: " << error->message << std::endl;
                        }
                        browser_set_loading_state(false);
                        std::cerr << "[ERROR] ==================================" << std::endl;
                        return FALSE;
                    }), nullptr);
            }
            
            // 5. Forcer le rendu et l'affichage final
            gtk_widget_queue_resize(webWidget);
            gtk_widget_queue_draw(webWidget);
            if (webContainer) {
                gtk_widget_queue_resize(webContainer);
                gtk_widget_queue_draw(webContainer);
            }
            
            // S'assurer que la WebView est visible une dernière fois
            gtk_widget_show_all(webWidget);
            gtk_widget_set_visible(webWidget, TRUE);
            
            // Mettre à jour la barre d'URL
            if (urlBar) {
                gtk_entry_set_text(GTK_ENTRY(urlBar), tab.url.c_str());
            }
            
            // Mettre à jour le bouton étoile
            updateStarButton();
            
            // Mettre en surbrillance l'onglet actif
            highlight(tab.tabWidget);
            
            // Forcer un dernier traitement des événements
            iterations = 0;
            while (gtk_events_pending() && iterations < 5) {
                gtk_main_iteration_do(FALSE);
                iterations++;
            }
            
            WEEDLYWEB_DEBUG_LOG("[DEBUG] Changement d'onglet terminé - WebView visible: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO") << ", parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
            return;
        }
    }
}



void Browser::executeScriptInActiveTab(const std::string& script) {
    if (activeTab && activeTab->webView) {
        scriptEngine->executerScript(activeTab->webView, script);
    }
}

void Browser::removeFavorite(GtkWidget* widget) {
    const char* storedName = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-name"));
    const gchar* favoriteName = storedName ? storedName : gtk_button_get_label(GTK_BUTTON(widget));
    favoritesManager->removeFavorite(favoriteName ? favoriteName : "");
    refreshFavoritesBar();
    updateStarButton();
}


void Browser::removeTab(GtkWidget *tabWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(tabs.begin(), tabs.end(), [tabWidget](const auto &tab) {
        return tab.tabWidget == tabWidget;
    });

    if (it != tabs.end()) {
        TabData& tab = *it;
        tabsManager->removeTab(tab.groupName, tab.url);
        
        // Nettoyer la WebView
        if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
            // Déconnecter les signaux avant de retirer
            g_signal_handlers_disconnect_matched(tab.webView, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            
            // Retirer du container
            GtkWidget* webWidget = GTK_WIDGET(tab.webView);
            GtkWidget* parent = gtk_widget_get_parent(webWidget);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), webWidget);
            }
            
            // Nettoyer le set statique des WebViews connectées
            // Accéder au même set statique utilisé dans changeActiveTab
            {
                static std::set<WebKitWebView*> connectedViews;
                connectedViews.erase(tab.webView);
            }
            
            // Libérer la référence (GTK gère la durée de vie des widgets)
            // Note: Ne pas utiliser g_object_unref ou g_clear_object car GTK gère automatiquement
            // la durée de vie des widgets quand leur parent est détruit
            tab.webView = nullptr;
        }

        // Supprimer le widget de l'onglet
        // CRITIQUE : Vérifier plusieurs fois que le widget est valide avant destruction
        if (tab.tabWidget) {
            // Vérifier que c'est bien un widget GTK valide
            if (GTK_IS_WIDGET(tab.tabWidget)) {
                // Vérifier qu'il n'est pas déjà en cours de destruction
                if (!gtk_widget_in_destruction(tab.tabWidget)) {
                    // Retirer du parent AVANT de détruire
                    GtkWidget* parent = gtk_widget_get_parent(tab.tabWidget);
                    if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                        // Vérifier à nouveau que le widget est toujours valide
                        if (GTK_IS_WIDGET(tab.tabWidget) && !gtk_widget_in_destruction(tab.tabWidget)) {
                            gtk_container_remove(GTK_CONTAINER(parent), tab.tabWidget);
                        }
                    }
                    
                    // Vérifier une dernière fois avant destruction
                    if (GTK_IS_WIDGET(tab.tabWidget) && !gtk_widget_in_destruction(tab.tabWidget)) {
                        gtk_widget_destroy(tab.tabWidget);
                    }
                }
            }
            // Réinitialiser le pointeur après destruction
            tab.tabWidget = nullptr;
        }
        
        // Mettre à jour l'onglet actif avant de supprimer
        bool wasActive = (activeTab == &(*it));
        
        // Sauvegarder le pointeur vers le widget de l'onglet suivant (si disponible)
        GtkWidget* nextTabWidget = nullptr;
        if (wasActive && tabs.size() > 1) {
            // Trouver l'onglet suivant ou précédent
            auto nextIt = std::next(it);
            if (nextIt != tabs.end()) {
                nextTabWidget = nextIt->tabWidget;
            } else {
                // Prendre l'onglet précédent
                if (it != tabs.begin()) {
                    auto prevIt = std::prev(it);
                    nextTabWidget = prevIt->tabWidget;
                }
            }
        }
        
        // Réinitialiser activeTab AVANT de supprimer de la liste
        if (wasActive) {
            activeTab = nullptr;
        }
        
        // Supprimer l'onglet de la liste
        tabs.erase(it);
        
        // Si aucun onglet n'est présent, fermer l'application
        if (tabs.empty()) {
            closeApplication();
        } else {
            // Activer un onglet restant de manière sécurisée
            if (nextTabWidget && GTK_IS_WIDGET(nextTabWidget) && !gtk_widget_in_destruction(nextTabWidget)) {
                // Vérifier que l'onglet existe toujours dans la liste
                bool tabExists = false;
                for (const auto& remainingTab : tabs) {
                    if (remainingTab.tabWidget == nextTabWidget) {
                        tabExists = true;
                        break;
                    }
                }
                if (tabExists) {
                    changeActiveTab(nextTabWidget);
                } else if (!tabs.empty()) {
                    // Fallback : prendre le premier onglet disponible
                    changeActiveTab(tabs.front().tabWidget);
                }
            } else if (!tabs.empty()) {
                // Fallback : prendre le premier onglet disponible
                changeActiveTab(tabs.front().tabWidget);
            }
        }
        persistSession();
    }
}

void Browser::highlight(GtkWidget *tabWidget) {
    if (!tabWidget) return;
    
    // Parcourir tous les tabs et gérer les classes CSS
    for (auto &tab : tabs) {
        if (tab.tabWidget && GTK_IS_WIDGET(tab.tabWidget)) {
            GtkStyleContext* context = gtk_widget_get_style_context(tab.tabWidget);
            if (tab.tabWidget == tabWidget) {
                // Ajouter la classe "onglet-actif" à l'onglet sélectionné
                gtk_style_context_add_class(context, "onglet-actif");
            } else {
                // Retirer la classe "onglet-actif" des autres tabs
                gtk_style_context_remove_class(context, "onglet-actif");
            }
        }
    }
}

void Browser::loadURL(const std::string& url) {
    if (!activeTab || !activeTab->webView || !WEBKIT_IS_WEB_VIEW(activeTab->webView)) {
        std::cerr << "[ERREUR] loadURL: WebView invalide" << std::endl;
        return;
    }

    std::string urlNormalisee = normalizeNavigationUrl(url);
    webkit_web_view_load_uri(activeTab->webView, urlNormalisee.c_str());
    activeTab->url = urlNormalisee;
    history.push_back(urlNormalisee);
    recordHistoryVisit(urlNormalisee);

    if (urlBar) {
        gtk_entry_set_text(GTK_ENTRY(urlBar), urlNormalisee.c_str());
    }
    
    updateStarButton();
    persistSession();
}

void Browser::showMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}


void Browser::loadConfiguration() {
    std::string chemin = FileManager::configJSONPath();
    nlohmann::json config = FileManager::readJSON(chemin);
    
    std::string cheminFavoris = FileManager::favoritesJSONPath();
    nlohmann::json favorisJson = FileManager::readJSON(cheminFavoris);

    if (favorisJson.is_null() || favorisJson.empty()) {
        std::cerr << "Aucun favori trouvé, initialisation avec un favori par défaut." << std::endl;
        favorisJson = nlohmann::json::array({
            {{"name", "DuckDuckGo"}, {"url", "https://duckduckgo.com/"}, {"tag", "Recherche"}}
        });
        FileManager::writeJSON(cheminFavoris, favorisJson);
    }
    
    *favorites = favorisJson;
    
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://duckduckgo.com/";
        FileManager::writeJSON(chemin, config);
    }

    homepage = config.value("homepage", "https://duckduckgo.com/");
    forceDarkMode = config.value("force_dark_mode", false);
    // Mode lecture = par page / session UI, pas restauré au démarrage
    readerMode = false;
    pendingSessionTabs.clear();
    restorePreviousSession = false;

    if (config.contains("tabs") && config["tabs"].is_array() && !config["tabs"].empty()) {
        for (const auto& onglet : config["tabs"]) {
            if (onglet.contains("url") && onglet["url"].is_string()) {
                const std::string u = onglet["url"].get<std::string>();
                if (!u.empty()) {
                    pendingSessionTabs.push_back(u);
                }
            }
        }
        // Restaurer si session sale (crash) ou si l'utilisateur a des onglets sauvegardés
        restorePreviousSession = config.value("session_dirty", true) || !pendingSessionTabs.empty();
    }

    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
    if (restorePreviousSession && !pendingSessionTabs.empty()) {
        std::cout << "Session précédente : " << pendingSessionTabs.size() << " onglet(s) à restaurer." << std::endl;
    }
    updateEnhancementButtons();
}


void Browser::saveConfiguration() {
    persistSession();
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
}

void Browser::persistSession() {
    nlohmann::json config = FileManager::readJSON(FileManager::configJSONPath());
    if (!config.is_object()) {
        config = nlohmann::json::object();
    }
    config["homepage"] = homepage;
    config["force_dark_mode"] = forceDarkMode;

    nlohmann::json ongletsJson = nlohmann::json::array();
    for (const auto& tab : tabs) {
        if (tab.url.empty()) {
            continue;
        }
        // Ne pas persister les pages HTML internes (stubs)
        if (tab.url.rfind("weedly://", 0) == 0 || tab.url == "about:blank") {
            continue;
        }
        ongletsJson.push_back({{"url", tab.url}, {"group", tab.groupName}});
    }
    config["tabs"] = ongletsJson;
    config["session_dirty"] = true;
    FileManager::writeJSON(FileManager::configJSONPath(), config);
}

void Browser::fitWindowToMonitor() {
    if (!window) {
        return;
    }

    GdkDisplay* display = gdk_display_get_default();
    if (!display) {
        gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
        return;
    }

    // Moniteur sous le pointeur = écran où l'utilisateur a la souris
    GdkMonitor* monitor = nullptr;
    gint pointerX = 0;
    gint pointerY = 0;
    GdkSeat* seat = gdk_display_get_default_seat(display);
    if (seat) {
        GdkDevice* pointer = gdk_seat_get_pointer(seat);
        if (pointer) {
            gdk_device_get_position(pointer, nullptr, &pointerX, &pointerY);
            monitor = gdk_display_get_monitor_at_point(display, pointerX, pointerY);
        }
    }
    if (!monitor) {
        monitor = gdk_display_get_primary_monitor(display);
    }
    if (!monitor && gdk_display_get_n_monitors(display) > 0) {
        monitor = gdk_display_get_monitor(display, 0);
    }

    if (!monitor) {
        gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
        return;
    }

    GdkRectangle workarea = {};
    gdk_monitor_get_workarea(monitor, &workarea);

    // Taille = zone utile du moniteur actif uniquement (pas le bureau virtuel 3 écrans)
    gtk_window_set_default_size(GTK_WINDOW(window), workarea.width, workarea.height);
    // Ne pas utiliser GTK_WIN_POS_CENTER : ça centre sur tout le desktop étendu
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

    // Stocker la géométrie pour l'appliquer au map (avant maximize)
    auto* geom = new GdkRectangle(workarea);
    g_object_set_data_full(G_OBJECT(window), "weedly-target-workarea", geom,
                           [](gpointer p) { delete static_cast<GdkRectangle*>(p); });

    // Au premier affichage : placer sur ce moniteur puis maximiser dessus
    g_signal_connect(window, "map-event", G_CALLBACK(+[](GtkWidget* widget, GdkEvent*, gpointer) -> gboolean {
        auto* wa = static_cast<GdkRectangle*>(g_object_get_data(G_OBJECT(widget), "weedly-target-workarea"));
        if (!wa || !GTK_IS_WINDOW(widget)) {
            return FALSE;
        }

        // Positionner d'abord sur le moniteur de la souris (sinon maximize
        // tombe sur le moniteur primary / bureau virtuel)
        gtk_window_move(GTK_WINDOW(widget), wa->x + 8, wa->y + 8);
        gtk_window_resize(GTK_WINDOW(widget),
                          std::max(800, wa->width - 16),
                          std::max(600, wa->height - 16));

        // Maximiser = grand écran sur CE moniteur (barre de titre + chrome visibles)
        gtk_window_maximize(GTK_WINDOW(widget));

        // Une seule fois
        g_object_set_data(G_OBJECT(widget), "weedly-target-workarea", nullptr);
        return FALSE;
    }), nullptr);
}

void Browser::preloadHomepage() {
    const std::string url = homepage.empty() ? "https://duckduckgo.com/" : homepage;
    if (preloadHomeView) {
        return;
    }
    preloadHomeView = createPersistentWebView();
    if (!preloadHomeView || !WEBKIT_IS_WEB_VIEW(preloadHomeView)) {
        preloadHomeView = nullptr;
        return;
    }
    // Garder une référence : le widget n'a pas encore de parent
    g_object_ref_sink(preloadHomeView);
    WebKitSettings* settings = webkit_web_view_get_settings(preloadHomeView);
    if (settings) {
        webkit_settings_set_enable_javascript(settings, TRUE);
    }
    webkit_web_view_load_uri(preloadHomeView, url.c_str());
}

WebKitWebView* Browser::takePreloadedHomeView(const std::string& url) {
    if (!preloadHomeView || !WEBKIT_IS_WEB_VIEW(preloadHomeView)) {
        return nullptr;
    }
    const std::string target = normalizeNavigationUrl(url);
    const std::string home = normalizeNavigationUrl(homepage.empty() ? "https://duckduckgo.com/" : homepage);
    if (!FavoritesJson::urlsMatch(target, home)) {
        return nullptr;
    }
    WebKitWebView* view = preloadHomeView;
    preloadHomeView = nullptr;
    return view;
}

void Browser::restoreSessionTabs() {
    if (restorePreviousSession && !pendingSessionTabs.empty()) {
        for (const auto& url : pendingSessionTabs) {
            addNewTab(url);
        }
        pendingSessionTabs.clear();
    } else {
        requestHomepageSearchFocus();
        addNewTab(homepage.empty() ? "https://duckduckgo.com/" : homepage);
    }
}

std::string Browser::normalizeNavigationUrl(const std::string& url) {
    if (url.empty()) {
        return "https://duckduckgo.com/";
    }
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
        url.rfind("file://", 0) == 0 || url.rfind("data:", 0) == 0 ||
        url.rfind("about:", 0) == 0 || url.rfind("webkit://", 0) == 0 ||
        url.rfind("weedly://", 0) == 0) {
        return url;
    }
    if (url.find("://") != std::string::npos) {
        return url;
    }
    return "https://" + url;
}

void Browser::addFavorite(const std::string& nom, const std::string& url, const std::string& tag) {
    favoritesManager->addFavorite(nom, url, tag);
    refreshFavoritesBar();
    updateStarButton();
}



void Browser::showFavoritesOverflowMenu() {
    GtkWidget* menu = gtk_menu_new();
    constexpr int kMaxTopLevelSlots = 14;
    int index = 0;
    bool hasOverflow = false;

    if (favorites) {
        for (const auto& favori : *favorites) {
            if (index++ < kMaxTopLevelSlots) {
                continue;
            }
            hasOverflow = true;
            if (FavoritesJson::isFolder(favori)) {
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), create_folder_menu_item(this, favori));
            } else if (favori.contains("name") && favori.contains("url")) {
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), create_bookmark_menu_item(
                    this,
                    favori["name"].get<std::string>(),
                    favori["url"].get<std::string>()));
            }
        }
    }

    if (hasOverflow) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    }

    GtkWidget* manage = gtk_menu_item_new_with_label("Gestionnaire de favoris…");
    g_signal_connect(manage, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showFavoritesManager();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), manage);

    gtk_widget_show_all(menu);
    if (favoritesBar) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_NORTH_EAST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

bool Browser::confirmAction(const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(
        window ? GTK_WINDOW(window) : nullptr,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s",
        message.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Annuler", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Supprimer", GTK_RESPONSE_ACCEPT);
    const int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response == GTK_RESPONSE_ACCEPT;
}

void Browser::openAllBookmarksInFolder(const nlohmann::json& folder) {
    std::function<void(const nlohmann::json&)> walk = [&](const nlohmann::json& node) {
        if (FavoritesJson::isFolder(node) && node.contains("children")) {
            for (const auto& child : node["children"]) {
                walk(child);
            }
        } else if (node.contains("url") && node["url"].is_string()) {
            addNewTab(node["url"].get<std::string>());
        }
    };
    walk(folder);
}

void Browser::showHistoryPage() {
    openHtmlTab("Historique",
        "<!DOCTYPE html><html><head><meta charset=utf-8><title>Historique</title>"
        "<style>body{font-family:system-ui;background:#1e1e1e;color:#eee;padding:2rem}"
        "h1{color:#4a90e2}li{margin:.4rem 0;opacity:.85}</style></head><body>"
        "<h1>Historique</h1><p>Page inspirée Brave — historique local à brancher.</p>"
        "<ul><li>Aujourd'hui</li><li>Hier</li><li>Autres appareils (sync) — à venir</li></ul>"
        "</body></html>");
}

void Browser::showPasswordsPage() {
    openHtmlTab("Mots de passe",
        "<!DOCTYPE html><html><head><meta charset=utf-8><title>Mots de passe</title>"
        "<style>body{font-family:system-ui;background:#1e1e1e;color:#eee;padding:2rem}"
        "h1{color:#4a90e2}</style></head><body>"
        "<h1>Mots de passe et saisie automatique</h1>"
        "<p>Section paramètres dédiée (style Brave) — coffre local à brancher.</p>"
        "</body></html>");
}

void Browser::showDevToolsPage(const std::string& tool) {
    openHtmlTab(tool,
        std::string("<!DOCTYPE html><html><head><meta charset=utf-8><title>") + tool + "</title>"
        "<style>body{font-family:monospace;background:#121212;color:#9f9;padding:2rem}"
        "h1{color:#6cf}</style></head><body><h1>" + tool + "</h1>"
        "<p>Outil développeur WeedlyWeb — stub UI (inspiré Brave / Chromium DevTools).</p>"
        "<p>WebKit Inspector peut être branché ensuite via WebKitWebInspector.</p>"
        "</body></html>");
}

void Browser::openHtmlTab(const std::string& title, const std::string& html) {
    addNewTab("about:blank");
    if (!activeTab || !activeTab->webView || !WEBKIT_IS_WEB_VIEW(activeTab->webView)) {
        return;
    }
    webkit_web_view_load_html(activeTab->webView, html.c_str(), nullptr);
    activeTab->url = "weedly://" + title;
    if (activeTab->label && GTK_IS_LABEL(activeTab->label)) {
        gtk_label_set_text(GTK_LABEL(activeTab->label), title.c_str());
    }
    if (urlBar) {
        gtk_entry_set_text(GTK_ENTRY(urlBar), activeTab->url.c_str());
    }
    updateStarButton();
}

void Browser::showOptionsMenu() {
    GtkWidget* menu = gtk_menu_new();

    // Navigation / fenêtres
    GtkWidget* newTab = gtk_menu_item_new_with_label("Nouvel onglet");
    g_signal_connect(newTab, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        auto* b = static_cast<Browser*>(ud);
        b->addNewTab(b->getHomepage());
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), newTab);

    GtkWidget* newWin = gtk_menu_item_new_with_label("Nouvelle fenêtre");
    g_signal_connect(newWin, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        auto* b = static_cast<Browser*>(ud);
        b->getTabsManager()->ajouterGroupe("Fenêtre", "#A0A5EB");
        b->changeTabGroup("Fenêtre");
        b->addNewTab(b->getHomepage());
    }), this);
    gtk_widget_set_tooltip_text(newWin, "Ouvre un groupe « Fenêtre » (stub multi-fenêtre Brave)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), newWin);

    GtkWidget* privateWin = gtk_menu_item_new_with_label("Nouvelle fenêtre de navigation privée");
    g_signal_connect(privateWin, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        auto* b = static_cast<Browser*>(ud);
        b->getTabsManager()->ajouterGroupe("Privé", "#9E1F63");
        b->changeTabGroup("Privé");
        b->addNewTab(b->getHomepage());
    }), this);
    gtk_widget_set_tooltip_text(privateWin, "Groupe isolé « Privé » — session WebKit séparée à brancher");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), privateWin);

    GtkWidget* torWin = gtk_menu_item_new_with_label("Nouvelle fenêtre privée avec Tor");
    g_signal_connect(torWin, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        auto* b = static_cast<Browser*>(ud);
        b->getTabsManager()->ajouterGroupe("Tor", "#E22172");
        b->changeTabGroup("Tor");
        b->addNewTab("https://duckduckgo.com/");
    }), this);
    gtk_widget_set_tooltip_text(torWin, "Inspiré Brave Tor window — proxy Tor à brancher");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), torWin);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Historique (split-like submenu)
    GtkWidget* hist = gtk_menu_item_new_with_label("Historique");
    GtkWidget* histSub = gtk_menu_new();
    GtkWidget* histLocal = gtk_menu_item_new_with_label("Historique de cet appareil");
    g_signal_connect(histLocal, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showHistoryPage();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(histSub), histLocal);
    GtkWidget* histSync = gtk_menu_item_new_with_label("Autres appareils (synchronisation)");
    gtk_widget_set_tooltip_text(histSync, "Sync multi-appareils — à brancher");
    gtk_menu_shell_append(GTK_MENU_SHELL(histSub), histSync);
    GtkWidget* histClear = gtk_menu_item_new_with_label("Effacer les données de navigation…");
    gtk_menu_shell_append(GTK_MENU_SHELL(histSub), histClear);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(hist), histSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), hist);

    GtkWidget* passwords = gtk_menu_item_new_with_label("Mots de passe et saisie automatique");
    g_signal_connect(passwords, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showPasswordsPage();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), passwords);

    GtkWidget* bookmarks = gtk_menu_item_new_with_label("Favoris");
    GtkWidget* bmSub = gtk_menu_new();
    GtkWidget* bmManage = gtk_menu_item_new_with_label("Gestionnaire de favoris");
    g_signal_connect(bmManage, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showFavoritesManager();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(bmSub), bmManage);
    GtkWidget* bmImport = gtk_menu_item_new_with_label("Importer des favoris…");
    gtk_menu_shell_append(GTK_MENU_SHELL(bmSub), bmImport);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(bookmarks), bmSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), bookmarks);

    // Cyber & Dev Tools (liens utiles — hors barre de favoris)
    auto appendUrlItem = [&](GtkWidget* shell, const char* label, const char* url) {
        GtkWidget* it = gtk_menu_item_new_with_label(label);
        g_object_set_data_full(G_OBJECT(it), "nav-url", g_strdup(url), g_free);
        g_signal_connect(it, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer ud) {
            const char* u = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "nav-url"));
            if (u) {
                static_cast<Browser*>(ud)->addNewTab(u);
            }
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(shell), it);
    };

    GtkWidget* cyber = gtk_menu_item_new_with_label("Cyber");
    GtkWidget* cyberSub = gtk_menu_new();
    appendUrlItem(cyberSub, "CVE Details", "https://www.cvedetails.com/");
    appendUrlItem(cyberSub, "NVD", "https://nvd.nist.gov/");
    appendUrlItem(cyberSub, "OWASP", "https://owasp.org/");
    {
        GtkWidget* cveStub = gtk_menu_item_new_with_label("CVE Analyzer (outil local)");
        g_signal_connect(cveStub, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
            static_cast<Browser*>(ud)->showDevToolsPage("CVE Analyzer");
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(cyberSub), cveStub);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(cyber), cyberSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), cyber);

    GtkWidget* devToolsLinks = gtk_menu_item_new_with_label("Dev Tools");
    GtkWidget* dtlSub = gtk_menu_new();
    appendUrlItem(dtlSub, "caniuse", "https://caniuse.com/");
    appendUrlItem(dtlSub, "regex101", "https://regex101.com/");
    appendUrlItem(dtlSub, "MDN", "https://developer.mozilla.org/");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(devToolsLinks), dtlSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), devToolsLinks);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Outils développeur
    GtkWidget* dev = gtk_menu_item_new_with_label("Outils de développement");
    GtkWidget* devSub = gtk_menu_new();
    struct DevItem { const char* label; const char* tool; };
    const DevItem tools[] = {
        {"Inspecteur d'éléments", "Inspecteur"},
        {"Débogueur JavaScript", "Débogueur JS"},
        {"Console", "Console"},
        {"Réseau", "Réseau"},
        {"Performance", "Performance"},
        {"Sécurité / Cyber", "Sécurité"},
        {"Stockage (cookies, localStorage)", "Stockage"},
        {"CVE / analyse", "CVE Analyzer"},
    };
    for (const auto& t : tools) {
        GtkWidget* it = gtk_menu_item_new_with_label(t.label);
        g_object_set_data_full(G_OBJECT(it), "tool", g_strdup(t.tool), g_free);
        g_signal_connect(it, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer ud) {
            const char* tool = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "tool"));
            static_cast<Browser*>(ud)->showDevToolsPage(tool ? tool : "DevTools");
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(devSub), it);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(dev), devSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), dev);

    // Extensions / privacy inspired by Brave
    GtkWidget* shields = gtk_menu_item_new_with_label("Boucliers & confidentialité");
    GtkWidget* shieldsSub = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(shieldsSub), gtk_menu_item_new_with_label("Bloqueur de pubs (à brancher)"));
    gtk_menu_shell_append(GTK_MENU_SHELL(shieldsSub), gtk_menu_item_new_with_label("Fingerprinting (à brancher)"));
    gtk_menu_shell_append(GTK_MENU_SHELL(shieldsSub), gtk_menu_item_new_with_label("HTTPS Everywhere (à brancher)"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(shields), shieldsSub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), shields);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* settings = gtk_menu_item_new_with_label("Paramètres");
    g_signal_connect(settings, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showSettings();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings);

    GtkWidget* help = gtk_menu_item_new_with_label("Aide");
    g_signal_connect(help, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->showHelp();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), help);

    GtkWidget* about = gtk_menu_item_new_with_label("À propos de WeedlyWeb");
    g_signal_connect(about, "activate", G_CALLBACK(+[](GtkWidget*, gpointer) {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "WeedlyWeb\n\nNavigateur WebKit2GTK\nUI inspirée de Brave Browser\nhttps://github.com/brave/brave-browser");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), about);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* quit = gtk_menu_item_new_with_label("Quitter");
    g_signal_connect(quit, "activate", G_CALLBACK(+[](GtkWidget*, gpointer ud) {
        static_cast<Browser*>(ud)->closeApplication();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);

    gtk_widget_show_all(menu);

    GList* children = gtk_container_get_children(GTK_CONTAINER(navigationBar));
    GtkWidget* boutonMenu = nullptr;
    if (children) {
        GList* last = g_list_last(children);
        if (last && GTK_IS_BUTTON(GTK_WIDGET(last->data))) {
            boutonMenu = GTK_WIDGET(last->data);
        }
    }
    g_list_free(children);

    if (boutonMenu) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), boutonMenu, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showGroupsMenu() {
    GtkWidget* menu = gtk_menu_new();

    GtkWidget* itemNouveauGroupe = gtk_menu_item_new_with_label("➕ Nouveau groupe…");
    g_signal_connect(itemNouveauGroupe, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "Nouveau groupe d'onglets",
            GTK_WINDOW(navigateur->window),
            GTK_DIALOG_MODAL,
            "Annuler", GTK_RESPONSE_CANCEL,
            "Créer", GTK_RESPONSE_ACCEPT,
            nullptr);
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nom du groupe");
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const gchar* groupName = gtk_entry_get_text(GTK_ENTRY(entry));
            if (groupName && *groupName) {
                navigateur->getTabsManager()->ajouterGroupe(groupName);
                navigateur->changeTabGroup(groupName);
            }
        }
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemNouveauGroupe);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    const std::string actif = tabsManager->getGroupeActif();
    for (const auto& groupe : tabsManager->getGroupes()) {
        const std::string color = tabsManager->getCouleurGroupe(groupe);
        std::string label = (groupe == actif ? "● " : "○ ") + groupe;
        GtkWidget* itemGroupe = gtk_menu_item_new_with_label(label.c_str());
        gtk_widget_set_tooltip_text(itemGroupe, ("Couleur " + color).c_str());
        g_object_set_data_full(G_OBJECT(itemGroupe), "group-name", g_strdup(groupe.c_str()), g_free);
        g_signal_connect(itemGroupe, "activate", G_CALLBACK(+[](GtkWidget* item, gpointer user_data) {
            auto* navigateur = static_cast<Browser*>(user_data);
            const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(item), "group-name"));
            if (name) {
                navigateur->changeTabGroup(name);
            }
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemGroupe);

        if (groupe != "Par défaut") {
            GtkWidget* delItem = gtk_menu_item_new_with_label(("    Supprimer « " + groupe + " »").c_str());
            g_object_set_data_full(G_OBJECT(delItem), "group-name", g_strdup(groupe.c_str()), g_free);
            g_signal_connect(delItem, "activate", G_CALLBACK(+[](GtkWidget* item, gpointer user_data) {
                auto* navigateur = static_cast<Browser*>(user_data);
                const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(item), "group-name"));
                if (!name) return;
                if (!navigateur->confirmAction("Supprimer le groupe",
                        std::string("Supprimer le groupe « ") + name + " » ? Les onglets iront dans « Par défaut ».")) {
                    return;
                }
                navigateur->deleteTabGroup(name);
            }), this);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), delItem);
        }
    }

    gtk_widget_show_all(menu);

    GtkWidget* anchor = groupChip ? groupChip : tabsBar;
    if (anchor && GTK_IS_WIDGET(anchor)) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), anchor, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showSettings() {
    std::string cheminParametres = FileManager::cheminParametresHTML();
    std::string urlParametres = "file://" + cheminParametres;
    
    // Vérifier que le fichier existe
    std::ifstream fichier(cheminParametres);
    if (!fichier.good()) {
        std::cerr << "[ERREUR] Le fichier de paramètres n'existe pas: " << cheminParametres << std::endl;
        // Créer un contenu HTML minimal si le fichier n'existe pas
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Page de paramètres\n\nLe fichier de paramètres sera disponible prochainement."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
        return;
    }
    fichier.close();
    
    // Charger dans l'onglet actif ou créer un nouvel onglet
    if (activeTab && activeTab->webView) {
        webkit_web_view_load_uri(activeTab->webView, urlParametres.c_str());
        activeTab->url = urlParametres;
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), urlParametres.c_str());
        }
    } else {
        addNewTab(urlParametres);
    }
}

void Browser::showHelp() {
    std::string cheminHelp = FileManager::cheminHelpHTML();
    std::string urlHelp = "file://" + cheminHelp;
    
    // Vérifier que le fichier existe
    std::ifstream fichier(cheminHelp);
    if (!fichier.good()) {
        std::cerr << "[ERREUR] Le fichier d'aide n'existe pas: " << cheminHelp << std::endl;
        // Afficher un message d'erreur si le fichier n'existe pas
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Page d'aide\n\nLe fichier d'aide n'a pas pu être chargé."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
        return;
    }
    fichier.close();
    
    // Charger dans l'onglet actif ou créer un nouvel onglet
    if (activeTab && activeTab->webView) {
        webkit_web_view_load_uri(activeTab->webView, urlHelp.c_str());
        activeTab->url = urlHelp;
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), urlHelp.c_str());
        }
    } else {
        addNewTab(urlHelp);
    }
}


void Browser::enterFullscreenOnCurrentMonitor() {
    if (!window || !GTK_IS_WINDOW(window)) {
        return;
    }

    GdkDisplay* display = gdk_display_get_default();
    GdkWindow* gdkWin = gtk_widget_get_window(window);
    if (!display || !gdkWin) {
        gtk_window_fullscreen(GTK_WINDOW(window));
        return;
    }

    // Moniteur où se trouve réellement la fenêtre (pas le bureau virtuel entier)
    GdkMonitor* monitor = gdk_display_get_monitor_at_window(display, gdkWin);
    if (!monitor) {
        gint wx = 0, wy = 0, ww = 0, wh = 0;
        gtk_window_get_position(GTK_WINDOW(window), &wx, &wy);
        gtk_window_get_size(GTK_WINDOW(window), &ww, &wh);
        monitor = gdk_display_get_monitor_at_point(display, wx + ww / 2, wy + wh / 2);
    }
    if (!monitor) {
        monitor = gdk_display_get_primary_monitor(display);
    }

    gint monitorIndex = 0;
    const gint n = gdk_display_get_n_monitors(display);
    for (gint i = 0; i < n; ++i) {
        if (gdk_display_get_monitor(display, i) == monitor) {
            monitorIndex = i;
            break;
        }
    }

    // Sortir du maximize avant plein écran pour éviter un mauvais géométrie
    if (gdk_window_get_state(gdkWin) & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_window_unmaximize(GTK_WINDOW(window));
    }

    GdkRectangle geo = {};
    gdk_monitor_get_geometry(monitor, &geo);
    // Ancrer la fenêtre sur ce moniteur avant fullscreen_on_monitor
    gtk_window_move(GTK_WINDOW(window), geo.x + 1, geo.y + 1);

    GdkScreen* screen = gdk_display_get_default_screen(display);
    if (screen) {
        gtk_window_fullscreen_on_monitor(GTK_WINDOW(window), screen, monitorIndex);
    } else {
        gtk_window_fullscreen(GTK_WINDOW(window));
    }
}

void Browser::leaveFullscreen() {
    if (window && GTK_IS_WINDOW(window)) {
        gtk_window_unfullscreen(GTK_WINDOW(window));
    }
}

bool Browser::handleKeyboardShortcut(GdkEventKey* event) {
    if (!event || event->type != GDK_KEY_PRESS) {
        return false;
    }

    const guint mods = event->state & gtk_accelerator_get_default_mod_mask();
    const bool ctrl = (mods & GDK_CONTROL_MASK) != 0;
    const bool shift = (mods & GDK_SHIFT_MASK) != 0;
    const bool alt = (mods & GDK_MOD1_MASK) != 0;
    const bool superKey = (mods & GDK_SUPER_MASK) != 0;
    if (alt || superKey) {
        return false;
    }

    const guint key = gdk_keyval_to_lower(event->keyval);

    // F11 : plein écran uniquement sur le moniteur de la fenêtre
    if (key == GDK_KEY_F11 && !ctrl && !shift) {
        GdkWindow* gdkWin = window ? gtk_widget_get_window(window) : nullptr;
        const bool isFs = gdkWin && (gdk_window_get_state(gdkWin) & GDK_WINDOW_STATE_FULLSCREEN);
        if (isFs) {
            leaveFullscreen();
        } else {
            enterFullscreenOnCurrentMonitor();
        }
        return true;
    }

    // F5 : rafraîchir
    if (key == GDK_KEY_F5 && !ctrl && !shift) {
        if (activeTab && activeTab->webView && WEBKIT_IS_WEB_VIEW(activeTab->webView)) {
            webkit_web_view_reload(activeTab->webView);
            return true;
        }
    }

    if (!ctrl) {
        return false;
    }

    // Ctrl+T ou Ctrl++ (pavé / plus) : nouvel onglet
    if (!shift && (key == GDK_KEY_t || key == GDK_KEY_plus || key == GDK_KEY_KP_Add)) {
        addNewTab(homepage.empty() ? "https://duckduckgo.com/" : homepage);
        return true;
    }
    // AZERTY / certaines dispositions : Ctrl+Shift+= produit « + »
    if (shift && (key == GDK_KEY_equal || key == GDK_KEY_plus || key == GDK_KEY_KP_Add)) {
        addNewTab(homepage.empty() ? "https://duckduckgo.com/" : homepage);
        return true;
    }

    // Ctrl+W / Ctrl+Shift+W : fermer l'onglet actif (dernier → ferme l'app)
    if (key == GDK_KEY_w) {
        if (activeTab && activeTab->tabWidget) {
            removeTab(activeTab->tabWidget);
        } else if (!tabs.empty()) {
            removeTab(tabs.back().tabWidget);
        } else {
            closeApplication();
        }
        return true;
    }

    // Ctrl+Shift+D : dupliquer
    if (shift && key == GDK_KEY_d) {
        const std::string url = getCurrentURL();
        if (!url.empty()) {
            addNewTab(url);
        }
        return true;
    }

    // Ctrl+D : favori (sans Shift)
    if (!shift && key == GDK_KEY_d) {
        if (getPopoverFavoris() && getEntryNomFavori() && getEntryURLFavori()) {
            gtk_entry_set_text(GTK_ENTRY(getEntryNomFavori()), getCurrentTitle().c_str());
            gtk_entry_set_text(GTK_ENTRY(getEntryURLFavori()), getCurrentURL().c_str());
            gtk_popover_popup(GTK_POPOVER(getPopoverFavoris()));
        } else {
            toggleCurrentPageFavorite();
        }
        return true;
    }

    // Ctrl+Shift+C : palette
    if (shift && key == GDK_KEY_c) {
        toggleCommandPalette();
        return true;
    }

    // Ctrl+L / Ctrl+E : focus barre d'URL
    if (!shift && (key == GDK_KEY_l || key == GDK_KEY_e)) {
        focusUrlBar();
        return true;
    }

    // Ctrl+H : aide
    if (!shift && key == GDK_KEY_h) {
        showHelp();
        return true;
    }

    // Ctrl+R : rafraîchir
    if (!shift && key == GDK_KEY_r) {
        if (activeTab && activeTab->webView && WEBKIT_IS_WEB_VIEW(activeTab->webView)) {
            webkit_web_view_reload(activeTab->webView);
            return true;
        }
    }

    return false;
}

void Browser::configureKeyboardShortcuts() {
    // Key snooper : reçoit les touches même quand le focus est dans WebKitWebView
    if (keySnooperId == 0) {
        keySnooperId = gtk_key_snooper_install(weedly_key_snooper, this);
    }

    // Filet de sécurité si le snooper n'est pas dispo
    g_signal_connect(window, "key-press-event",
                     G_CALLBACK(+[](GtkWidget*, GdkEvent* event, gpointer user_data) -> gboolean {
                         auto* navigateur = static_cast<Browser*>(user_data);
                         if (event && event->type == GDK_KEY_PRESS) {
                             return navigateur->handleKeyboardShortcut(&event->key) ? TRUE : FALSE;
                         }
                         return FALSE;
                     }),
                     this);
}


void Browser::loadStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    // CSS amélioré avec design moderne en mode sombre
    const gchar* css = 
        "/* Fond sombre sur les surfaces, PAS sur chaque enfant (évite carrés autour icônes/titres) */ "
        "window { "
        "  background-color: #2d2d2d; "
        "  color: #e0e0e0; "
        "} "
        "box, scrolledwindow, viewport, overlay { "
        "  background-color: transparent; "
        "  color: #e0e0e0; "
        "} "
        "label, image { "
        "  background-color: transparent; "
        "  color: inherit; "
        "} "
        "entry { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "textview { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "button { "
        "  background-color: #404040; "
        "  color: #e0e0e0; "
        "  background-image: none; "
        "} "
        "button:hover { "
        "  background-color: #505050; "
        "} "
        "/* Barre de favorites — unie avec la nav, boutons visibles */ "
        "#barre-favorites { "
        "  background-color: #282828; "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  padding: 3px 8px; "
        "  min-height: 30px; "
        "} "
        "#button-favori, #button-favori-dossier, #button-favori-more { "
        "  border: 1px solid rgba(255, 255, 255, 0.12); "
        "  border-radius: 6px; "
        "  padding: 3px 10px; "
        "  background-color: rgba(60, 60, 60, 0.55); "
        "  background-image: none; "
        "  color: #ececec; "
        "  margin: 0 2px; "
        "  font-size: 12px; "
        "  min-height: 26px; "
        "  box-shadow: none; "
        "} "
        "#button-favori:hover, #button-favori-dossier:hover, #button-favori-more:hover { "
        "  background-color: rgba(90, 120, 180, 0.55); "
        "  border-color: rgba(120, 160, 220, 0.55); "
        "  color: #ffffff; "
        "} "
        "#button-favori:active, #button-favori-dossier:active { "
        "  background-color: rgba(74, 144, 226, 0.7); "
        "} "
        "#button-favori-dossier { "
        "  color: #f5dfa0; "
        "  font-weight: 600; "
        "} "
        "#button-favori image, #button-favori-dossier image, #button-favori-more image { "
        "  background-color: transparent; "
        "} "
        "#group-chip { "
        "  background-color: rgba(55, 60, 75, 0.9); "
        "  border: 1px solid rgba(255, 255, 255, 0.12); "
        "  border-radius: 14px; "
        "  padding: 2px 8px; "
        "  margin-right: 2px; "
        "} "
        "#group-chip-label { "
        "  font-size: 11px; "
        "  color: #e8e8e8; "
        "  font-weight: 600; "
        "  background-color: transparent; "
        "} "
        "#button-groupes { "
        "  min-width: 22px; "
        "  min-height: 22px; "
        "  padding: 0 4px; "
        "  border: none; "
        "  background-color: transparent; "
        "  background-image: none; "
        "  color: #ddd; "
        "} "
        "#button-groupes:hover { "
        "  background-color: rgba(255,255,255,0.12); "
        "  border-radius: 4px; "
        "} "
        "#favorites-list row:hover { "
        "  background-color: rgba(74, 144, 226, 0.25); "
        "} "
        "#favorites-add-form { "
        "  background-color: rgba(50, 50, 50, 0.9); "
        "  padding: 6px; "
        "  border-radius: 6px; "
        "} "
        "/* Barre de navigation - Mode sombre */ "
        "#barre-navigation { "
        "  padding: 6px 8px; "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  background-color: #282828; "
        "} "
        "#barre-navigation button { "
        "  background-image: none; "
        "} "
        "#barre-navigation button image { "
        "  background-color: transparent; "
        "} "
        "/* Barre d'tabs — même teinte que nav/favoris */ "
        "#barre-tabs { "
        "  padding: 4px 6px; "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  background-color: #282828; "
        "  border-radius: 0; "
        "} "
        "/* Boutons dans la barre d'tabs - Mode sombre */ "
        "#button-ajouter-onglet { "
        "  border: 1px solid rgba(74, 144, 226, 0.8); "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  background-color: rgba(74, 144, 226, 0.85); "
        "  background-image: none; "
        "  color: white; "
        "  min-width: 28px; "
        "  min-height: 24px; "
        "  font-size: 16px; "
        "  font-weight: bold; "
        "} "
        "#button-ajouter-onglet:hover { "
        "  background-color: rgba(74, 144, 226, 1.0); "
        "  border-color: rgba(50, 120, 200, 1.0); "
        "} "
        "#button-ajouter-onglet:active { "
        "  background-color: rgba(50, 120, 200, 1.0); "
        "} "
        "/* ScrolledWindow pour les onglets - Mode sombre */ "
        "#scrolled-tabs { "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  background-color: #282828; "
        "} "
        "#scrolled-tabs > *, #scrolled-tabs viewport, #scrolled-tabs undershoot, "
        "#scrolled-tabs overshoot, #scrolled-tabs junction { "
        "  background-color: #282828; "
        "} "
        "/* Onglets — fond uni ; enfants transparents */ "
        "#onglet { "
        "  border: 1px solid rgba(255, 255, 255, 0.10); "
        "  border-radius: 6px 6px 0 0; "
        "  padding: 4px 8px; "
        "  margin: 0 2px; "
        "  background-color: #323232; "
        "  background-image: none; "
        "  border-bottom: none; "
        "  min-height: 28px; "
        "  box-shadow: none; "
        "} "
        "#onglet > * { "
        "  background-color: transparent; "
        "  background-image: none; "
        "} "
        "#onglet image, #onglet label { "
        "  background-color: transparent; "
        "  background-image: none; "
        "  box-shadow: none; "
        "  border: none; "
        "} "
        "#onglet:hover { "
        "  background-color: #3a3a3a; "
        "  border-color: rgba(255, 255, 255, 0.18); "
        "} "
        "#onglet:active { "
        "  background-color: #404040; "
        "} "
        "/* Onglet actif */ "
        "#onglet.onglet-actif { "
        "  background-color: #3d5a80; "
        "  border-color: rgba(120, 170, 230, 0.7); "
        "  border-bottom: 2px solid #4a90e2; "
        "  font-weight: 600; "
        "  box-shadow: none; "
        "} "
        "#onglet.onglet-actif:hover { "
        "  background-color: #466891; "
        "} "
        "#onglet.onglet-actif > *, #onglet.onglet-actif image, #onglet.onglet-actif label { "
        "  background-color: transparent; "
        "} "
        "/* Labels dans les tabs - Mode sombre */ "
        "#onglet label { "
        "  color: #e0e0e0; "
        "  font-size: 12px; "
        "} "
        "#onglet.onglet-actif label { "
        "  color: #ffffff; "
        "  font-weight: 600; "
        "} "
        "/* Bouton fermer dans l'onglet — transparent, pas de pastille grise */ "
        "#onglet button { "
        "  border: none; "
        "  background-color: transparent; "
        "  background-image: none; "
        "  padding: 0 4px; "
        "  margin: 0 0 0 4px; "
        "  border-radius: 3px; "
        "  min-width: 18px; "
        "  min-height: 18px; "
        "  color: #c8c8c8; "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  box-shadow: none; "
        "} "
        "#onglet button:hover { "
        "  background-color: rgba(255, 255, 255, 0.14); "
        "  color: #ffffff; "
        "} "
        "#onglet button:active { "
        "  background-color: rgba(0, 0, 0, 0.25); "
        "} "
        "#onglet.onglet-actif button { "
        "  background-color: transparent; "
        "  color: #f0f0f0; "
        "} "
        "#onglet.onglet-actif button:hover { "
        "  background-color: rgba(255, 255, 255, 0.18); "
        "} "
        "#onglet.onglet-actif button { "
        "  background-color: rgba(255, 255, 255, 0.3); "
        "  border-color: rgba(255, 255, 255, 0.5); "
        "  color: rgba(255, 255, 255, 1.0); "
        "} "
        "#onglet.onglet-actif button:hover { "
        "  background-color: rgba(255, 255, 255, 0.5); "
        "  border-color: rgba(255, 255, 255, 0.7); "
        "  color: rgba(255, 255, 255, 1.0); "
        "} "
        "/* Container web et indicateur de chargement - Mode sombre avec debug visuel */ "
        "#container-web { "
        "  background-color: #1e1e1e; "
        "  border-top: 4px solid rgba(255, 255, 255, 0.3); "
        "  min-height: 600px; "
        "} "
        "/* WebView - Fond blanc pour voir le contenu */ "
        "#container-web > * { "
        "  background-color: #ffffff; "
        "} "
        "#loading-indicator { "
        "  background-color: rgba(30, 30, 30, 0.95); "
        "  padding: 20px; "
        "} "
        "#loading-indicator label { "
        "  font-size: 14px; "
        "  color: #cccccc; "
        "} "
        "/* Barre d'URL - Mode sombre */ "
        "#barre-navigation entry { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "/* Tous les widgets de la fenêtre - Mode sombre */ "
        "box, scrolledwindow, viewport { "
        "  background-color: #2d2d2d; "
        "  color: #e0e0e0; "
        "}";

    gtk_css_provider_load_from_data(provider, css, -1, &error);
    
    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
        g_object_unref(provider);
        return;
    }

    // Appliquer le style à l'écran avec la priorité la plus élevée
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Appliquer aussi au screen pour que tous les widgets héritent du style
    GdkScreen *screen = gtk_widget_get_screen(window);
    if (screen) {
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    
    // Ne pas libérer le provider ici - il sera libéré automatiquement par GTK


}


void Browser::onNavigateBack(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        if (webkit_web_view_can_go_back(n->activeTab->webView)) {
            webkit_web_view_go_back(n->activeTab->webView);
        }
    }
}

void Browser::onNavigateForward(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        if (webkit_web_view_can_go_forward(n->activeTab->webView)) {
            webkit_web_view_go_forward(n->activeTab->webView);
        }
    }
}

void Browser::onRefreshPage(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        webkit_web_view_reload(n->activeTab->webView);
    }
}

void Browser::onGoHome(GtkButton *, Browser *n) {
    n->loadURL(n->homepage);
}



void Browser::onUrlBarActivate(GtkEntry* entry, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    std::string text = gtk_entry_get_text(entry);
    if (text.empty()) {
        return;
    }

    // Si le texte correspond à un onglet ouvert, y basculer
    for (auto& tab : navigateur->tabs) {
        if (FavoritesJson::urlsMatch(tab.url, text) ||
            (!tab.url.empty() && tab.url.find(text) != std::string::npos)) {
            // match exact URL d'abord
        }
    }
    for (size_t i = 0; i < navigateur->tabs.size(); ++i) {
        if (FavoritesJson::urlsMatch(navigateur->tabs[i].url, text)) {
            navigateur->changeActiveTab(navigateur->tabs[i].tabWidget);
            return;
        }
    }

    navigateur->loadURL(text);
}

void Browser::loadBrowsingHistory() {
    nlohmann::json data = FileManager::readJSON(FileManager::historyJSONPath());
    if (data.is_array()) {
        browsingHistory = data;
    } else {
        browsingHistory = nlohmann::json::array();
    }
}

void Browser::saveBrowsingHistory() {
    FileManager::writeJSON(FileManager::historyJSONPath(), browsingHistory);
}

void Browser::recordHistoryVisit(const std::string& url, const std::string& title) {
    if (url.empty() || url.rfind("weedly://", 0) == 0 || url == "about:blank") {
        return;
    }
    const std::string canon = FavoritesJson::canonicalizeUrl(url);
    bool found = false;
    for (auto& entry : browsingHistory) {
        if (!entry.is_object() || !entry.contains("url")) continue;
        if (FavoritesJson::urlsMatch(entry["url"].get<std::string>(), url)) {
            entry["visits"] = entry.value("visits", 0) + 1;
            entry["last"] = static_cast<long long>(std::time(nullptr));
            if (!title.empty()) {
                entry["title"] = title;
            }
            entry["url"] = url;
            found = true;
            break;
        }
    }
    if (!found) {
        browsingHistory.push_back({
            {"url", url},
            {"title", title.empty() ? canon : title},
            {"visits", 1},
            {"last", static_cast<long long>(std::time(nullptr))}
        });
    }
    // Garder un historique raisonnable
    if (browsingHistory.size() > 500) {
        std::sort(browsingHistory.begin(), browsingHistory.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            return a.value("last", 0LL) > b.value("last", 0LL);
        });
        browsingHistory.erase(browsingHistory.begin() + 400, browsingHistory.end());
    }
    saveBrowsingHistory();
    refreshUrlCompletionModel();
}

void Browser::setupUrlBarCompletion() {
    if (!urlBar || !GTK_IS_ENTRY(urlBar)) {
        return;
    }
    // display, url, tabIndex (-1 = naviguer)
    urlCompletionStore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    GtkEntryCompletion* completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(urlCompletionStore));
    gtk_entry_completion_set_text_column(completion, 0);
    gtk_entry_completion_set_inline_completion(completion, TRUE);
    gtk_entry_completion_set_popup_completion(completion, TRUE);
    gtk_entry_completion_set_minimum_key_length(completion, 1);
    gtk_entry_set_completion(GTK_ENTRY(urlBar), completion);

    gtk_entry_completion_set_match_func(completion,
        +[](GtkEntryCompletion* /*comp*/, const gchar* key, GtkTreeIter* iter, gpointer user_data) -> gboolean {
            auto* store = static_cast<GtkListStore*>(user_data);
            gchar* display = nullptr;
            gchar* url = nullptr;
            gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &display, 1, &url, -1);
            if (!key || !*key) {
                g_free(display);
                g_free(url);
                return TRUE;
            }
            std::string k(key);
            for (char& c : k) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            auto contains = [&](const char* s) {
                if (!s) return false;
                std::string v(s);
                for (char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return v.find(k) != std::string::npos;
            };
            const bool ok = contains(display) || contains(url);
            g_free(display);
            g_free(url);
            return ok ? TRUE : FALSE;
        }, urlCompletionStore, nullptr);

    g_signal_connect(completion, "match-selected", G_CALLBACK(+[](GtkEntryCompletion* /*comp*/, GtkTreeModel* model, GtkTreeIter* iter, gpointer user_data) -> gboolean {
        auto* browser = static_cast<Browser*>(user_data);
        gchar* url = nullptr;
        gint tabIndex = -1;
        gtk_tree_model_get(model, iter, 1, &url, 2, &tabIndex, -1);
        if (!browser) {
            g_free(url);
            return FALSE;
        }
        if (tabIndex >= 0 && tabIndex < static_cast<gint>(browser->tabs.size())) {
            browser->changeActiveTab(browser->tabs[static_cast<size_t>(tabIndex)].tabWidget);
        } else if (url) {
            browser->loadURL(url);
        }
        g_free(url);
        return TRUE; // on a géré l'activation
    }), this);

    refreshUrlCompletionModel();
}

void Browser::refreshUrlCompletionModel() {
    if (!urlCompletionStore) {
        return;
    }
    gtk_list_store_clear(urlCompletionStore);

    auto appendRow = [&](const std::string& display, const std::string& url, int tabIndex) {
        GtkTreeIter iter;
        gtk_list_store_append(urlCompletionStore, &iter);
        gtk_list_store_set(urlCompletionStore, &iter,
            0, display.c_str(),
            1, url.c_str(),
            2, tabIndex,
            -1);
    };

    // Onglets ouverts
    for (size_t i = 0; i < tabs.size(); ++i) {
        std::string title = "Onglet";
        if (tabs[i].label && GTK_IS_LABEL(tabs[i].label)) {
            const gchar* t = gtk_label_get_text(GTK_LABEL(tabs[i].label));
            if (t && *t) title = t;
        }
        appendRow("🗂 " + title + " — " + tabs[i].url, tabs[i].url, static_cast<int>(i));
    }

    // Favoris (récursif)
    std::function<void(const nlohmann::json&)> walkFavs = [&](const nlohmann::json& arr) {
        if (!arr.is_array()) return;
        for (const auto& item : arr) {
            if (FavoritesJson::isFolder(item)) {
                walkFavs(item.value("children", nlohmann::json::array()));
            } else if (item.contains("url") && item.contains("name")) {
                appendRow("★ " + item["name"].get<std::string>() + " — " + item["url"].get<std::string>(),
                          item["url"].get<std::string>(), -1);
            }
        }
    };
    if (favorites) {
        walkFavs(*favorites);
    }

    // Historique trié par visites
    std::vector<nlohmann::json> hist;
    for (const auto& e : browsingHistory) {
        if (e.is_object() && e.contains("url")) hist.push_back(e);
    }
    std::sort(hist.begin(), hist.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
        return a.value("visits", 0) > b.value("visits", 0);
    });
    for (const auto& e : hist) {
        const std::string url = e["url"].get<std::string>();
        const std::string title = e.value("title", url);
        appendRow("🕒 " + title + " — " + url, url, -1);
    }
}

void Browser::closeApplication() {
    saveConfiguration();
    if (window && GTK_IS_WIDGET(window)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(window)) {
            gtk_widget_destroy(window);
        }
        window = nullptr;
    }
    gtk_main_quit();
}

void Browser::onNavigateBackWrapper(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateBack(button, navigateur);
}



void Browser::onNavigateForwardWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Browser*>(user_data)->onNavigateForward(button, static_cast<Browser*>(user_data));
}

void Browser::onRefreshPageWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Browser*>(user_data)->onRefreshPage(button, static_cast<Browser*>(user_data));
}
void Browser::createContextMenu(GtkWidget* button) {
    GtkWidget *menu = gtk_menu_new();

    // Option "Ajouter aux Favoris"
    GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter aux Favori");
    g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);
    
    // Option "Ouvrir un nouvel onglet"
    GtkWidget *nouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir un nouvel onglet");
    g_signal_connect(nouvelOngletItem, "activate", G_CALLBACK(on_ajouter_onglet), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), nouvelOngletItem);

    auto* data = new std::pair<Browser*, GtkWidget*>(this, button);
    // Option "Supprimer le favori" (Ajout si applicable)
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer le favori");
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori),
                      data, delete_user_data, G_CONNECT_AFTER);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);  // Ajout avant l'ouverture du menu
    gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}


void Browser::onStarButtonClicked(GtkButton*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        navigateur->toggleCurrentPageFavorite();
    }
}

void Browser::showCommandPalette() {
    if (commandPalette && window) {
        commandPalette->setCurrentWebView(renderingEngine.get());
        commandPalette->showPalette(GTK_WINDOW(window));
    }
}

void Browser::toggleCommandPalette() {
    if (!commandPalette || !window) return;
    
    // Vérifier si la palette est déjà visible
    if (commandPalette->isVisible()) {
        // Si elle est visible, la masquer
        commandPalette->hidePalette();
    } else {
        // Si elle n'est pas visible, l'afficher
        commandPalette->setCurrentWebView(renderingEngine.get());
        commandPalette->showPalette(GTK_WINDOW(window));
    }
}

// Fonctions hibernerOnglet et reactiverOnglet supprimées - gérées par MemoryManager
