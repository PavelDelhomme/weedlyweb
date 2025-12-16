#ifndef RENDERINGENGINE_H
#define RENDERINGENGINE_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string>
#include <functional>

// Forward declaration pour éviter les dépendances circulaires
typedef void (*LoadingStateCallback)(bool loading);

class RenderingEngine {
public:
    RenderingEngine();
    ~RenderingEngine();

    void connectURLChangedSignal(std::function<void(const std::string&)> callback);
    void connectPageLoadedSignal(std::function<void(const std::string&)> callback);
    void connectFaviconChangedSignal(std::function<void(cairo_surface_t*)> callback);
    void connectLoadCompleteSignal(std::function<void(const std::string&)> callback);
    void connectTitleChangedSignal(std::function<void(const std::string&)> callback);
    void initializeRendering(GtkWidget *mainContainer);
    void displayPage(const std::string& url);
    void refreshPage();
    void navigateBack();
    void navigateForward();
    void cleanupSignals();
    static void onNotifyUri(GObject *object, GParamSpec *param_spec, gpointer user_data);
    
    GtkWidget* createButton(const std::string& label, GCallback callback, gpointer data);
    GtkWidget* createTextEntry(GCallback callback, gpointer data);

    std::string getCurrentURL() const;
    std::string getCurrentTitle() const;
    WebKitWebView* getVueWeb() const { return webView; }
    
    // Callback pour gérer l'état de chargement dans la barre d'URL
    void setLoadingStateCallback(LoadingStateCallback callback) { loadingStateCallback = callback; }

private:
    WebKitWebView *webView;
    std::function<void(const std::string&)> callbackTitreChange;
    LoadingStateCallback loadingStateCallback;
};

#endif
