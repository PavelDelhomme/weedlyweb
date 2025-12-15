#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <functional>

class RenderingEngine;
class RequestInterceptor;

class CommandPalette {
public:
    CommandPalette();
    ~CommandPalette();
    
    void showPalette(GtkWindow* parent);
    void setCurrentWebView(RenderingEngine* webView);
    void setRequestInterceptor(RequestInterceptor* interceptor);
    void processCommand(const std::string& command);
    
    // Callback pour les commandes
    std::function<void(const std::string&)> onCommandSelected;

private:
    GtkWidget* m_window;
    GtkWidget* m_entry;
    GtkWidget* m_listBox;
    RenderingEngine* m_currentWebView;
    RequestInterceptor* m_requestInterceptor;
    std::vector<std::string> m_commands;
    
    void filterCommands(const std::string& text);
    void processCVECommand(const std::string& command);
    void processRequestCommand(const std::string& command);
    void showRequestAnalyzer();
    
    // Callbacks GTK
    static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer user_data);
    static void onEntryChanged(GtkEntry* entry, gpointer user_data);
    static void onRowActivated(GtkListBox* listBox, GtkListBoxRow* row, gpointer user_data);
    static gboolean onDeleteEvent(GtkWidget* widget, GdkEvent* event, gpointer user_data);
};

#endif // COMMANDPALETTE_H

