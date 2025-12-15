#ifndef REQUESTINTERCEPTOR_H
#define REQUESTINTERCEPTOR_H

#include <webkit2/webkit2.h>
#include <string>
#include <map>
#include <vector>
#include <functional>

struct InterceptedRequest {
    std::string id;
    std::string url;
    std::string method;
    std::map<std::string, std::string> headers;
    std::string body;
    time_t timestamp;
};

class RequestInterceptor {
public:
    RequestInterceptor();
    ~RequestInterceptor();
    
    // Activer/désactiver l'interception
    void enable();
    void disable();
    bool isEnabled() const { return m_enabled; }
    
    // Obtenir les requêtes interceptées
    std::vector<InterceptedRequest> getInterceptedRequests() const;
    void clearRequests();
    
    // Modifier une requête interceptée
    void modifyRequest(const std::string& id, const std::string& newUrl);
    
    // Callback pour les requêtes interceptées
    std::function<void(const InterceptedRequest&)> onRequestIntercepted;

private:
    bool m_enabled;
    std::vector<InterceptedRequest> m_requests;
    int m_requestCounter;
    
    // Callback WebKit
    static void onResourceLoadStarted(WebKitWebView* webView, 
                                     WebKitWebResource* resource, 
                                     WebKitURIRequest* request, 
                                     gpointer userData);
    
    static void onDecidePolicy(WebKitWebView* webView,
                              WebKitPolicyDecision* decision,
                              WebKitPolicyDecisionType type,
                              gpointer userData);
};

#endif // REQUESTINTERCEPTOR_H

