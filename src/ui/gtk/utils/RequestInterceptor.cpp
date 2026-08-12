#include "utils/RequestInterceptor.h"
#include <webkit2/webkit2.h>
#include <libsoup/soup.h>
#include <iostream>
#include <ctime>
#include <sstream>

RequestInterceptor::RequestInterceptor() 
    : m_enabled(false), m_requestCounter(0) {
}

RequestInterceptor::~RequestInterceptor() {
}

void RequestInterceptor::enable() {
    m_enabled = true;
    std::cout << "Intercepteur de requêtes activé" << std::endl;
}

void RequestInterceptor::disable() {
    m_enabled = false;
    std::cout << "Intercepteur de requêtes désactivé" << std::endl;
}

void RequestInterceptor::onResourceLoadStarted(WebKitWebView* webView, 
                                              WebKitWebResource* resource, 
                                              WebKitURIRequest* request, 
                                              gpointer userData) {
    auto* interceptor = static_cast<RequestInterceptor*>(userData);
    
    if (!interceptor->m_enabled) {
        return;
    }
    
    InterceptedRequest req;
    req.id = "req_" + std::to_string(++interceptor->m_requestCounter);
    
    const gchar* uri = webkit_uri_request_get_uri(request);
    req.url = uri ? uri : "";
    
    req.method = "GET"; // Par défaut, WebKit ne donne pas directement la méthode
    req.timestamp = time(nullptr);
    
    // Récupérer les en-têtes
    SoupMessageHeaders* headers = webkit_uri_request_get_http_headers(request);
    if (headers) {
        SoupMessageHeadersIter iter;
        soup_message_headers_iter_init(&iter, headers);
        const char* name, *value;
        while (soup_message_headers_iter_next(&iter, &name, &value)) {
            req.headers[name] = value;
        }
    }
    
    interceptor->m_requests.push_back(req);
    
    if (interceptor->onRequestIntercepted) {
        interceptor->onRequestIntercepted(req);
    }
    
    std::cout << "Requête interceptée: " << req.method << " " << req.url << std::endl;
}

void RequestInterceptor::onDecidePolicy(WebKitWebView* webView,
                                        WebKitPolicyDecision* decision,
                                        WebKitPolicyDecisionType type,
                                        gpointer userData) {
    // Cette fonction peut être utilisée pour intercepter les décisions de navigation
    // Pour l'instant, on laisse passer toutes les requêtes
}

std::vector<InterceptedRequest> RequestInterceptor::getInterceptedRequests() const {
    return m_requests;
}

void RequestInterceptor::clearRequests() {
    m_requests.clear();
    m_requestCounter = 0;
}

void RequestInterceptor::modifyRequest(const std::string& id, const std::string& newUrl) {
    for (auto& req : m_requests) {
        if (req.id == id) {
            req.url = newUrl;
            break;
        }
    }
}

