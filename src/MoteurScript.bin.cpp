#include "MoteurScript.h"
#include <iostream>


void MoteurScript::executerScript(WebKitWebView* vueWeb, const std::string& script) {
    if (!vueWeb) {
        std::cerr << "Erreur : WebView invalide." << std::endl;
        return;
    }
    webkit_web_view_evaluate_javascript(
        vueWeb, 
        script.c_str(), 
        -1, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr
    );
    std::cout << "Script exécuté : " << script << std::endl;
}



std::string MoteurScript::obtenirValeur(WebKitWebView* vueWeb, const std::string& script) {
    std::string result;
    webkit_web_view_evaluate_javascript(
        vueWeb, 
        script.c_str(), 
        -1, 
        nullptr, 
        nullptr, 
        nullptr, 
        [](GObject* source_object, GAsyncResult* res, gpointer user_data) {
            GError* error = nullptr;
            JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source_object), res, &error);

            if (error) {
                std::cerr << "Erreur JavaScript : " << error->message << std::endl;
                g_error_free(error);
                return;
            }

            if (value) {
                if (jsc_value_is_string(value)) {
                    *static_cast<std::string*>(user_data) = jsc_value_to_string(value);
                }
                g_object_unref(value);  // Libération de JSCValue
            }
        }, 
        &result
    );
    return result;
}

void MoteurScript::ajouterEvenement(WebKitWebView* vueWeb, const std::string& evenement, const std::string& callback) {
    std::string script = "document.addEventListener('" + evenement + "', " + callback + ");";
    executerScript(vueWeb, script);
}

