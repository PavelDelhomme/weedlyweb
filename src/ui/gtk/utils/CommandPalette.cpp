#include "utils/CommandPalette.h"
#include "rendering/RenderingEngine.h"
#include "utils/RequestInterceptor.h"
#include "utils/CVEAnalyzer.h"
#include <iostream>
#include <algorithm>

CommandPalette::CommandPalette() 
    : m_window(nullptr), m_entry(nullptr), m_listBox(nullptr),
      m_currentWebView(nullptr), m_requestInterceptor(nullptr) {
    
    m_commands = {
        "/cve detect",
        "/request GET",
        "/request POST",
        "/analyze",
        "/history",
        "/clear cache"
    };
}

CommandPalette::~CommandPalette() {
    if (m_window && GTK_IS_WIDGET(m_window)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(m_window)) {
            gtk_widget_destroy(m_window);
        }
        m_window = nullptr;
    }
}

void CommandPalette::showPalette(GtkWindow* parent) {
    if (!m_window) {
        m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(m_window), "Palette de Commandes");
        gtk_window_set_default_size(GTK_WINDOW(m_window), 600, 400);
        gtk_window_set_transient_for(GTK_WINDOW(m_window), parent);
        gtk_window_set_modal(GTK_WINDOW(m_window), TRUE);
        gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
        
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(m_window), vbox);
        
        // Entry pour la saisie
        m_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(m_entry), "Tapez une commande... (CTRL+SHIFT+C pour fermer)");
        gtk_box_pack_start(GTK_BOX(vbox), m_entry, FALSE, FALSE, 0);
        
        // ScrolledWindow pour la liste
        GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        
        m_listBox = gtk_list_box_new();
        gtk_container_add(GTK_CONTAINER(scrolled), m_listBox);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
        
        // Connexion des signaux
        g_signal_connect(m_entry, "changed", G_CALLBACK(onEntryChanged), this);
        g_signal_connect(m_entry, "activate", G_CALLBACK(+[](GtkEntry* entry, gpointer data) {
            auto* palette = static_cast<CommandPalette*>(data);
            GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(palette->m_listBox));
            if (row) {
                onRowActivated(GTK_LIST_BOX(palette->m_listBox), row, data);
            }
        }), this);
        
        g_signal_connect(m_window, "key-press-event", G_CALLBACK(onKeyPress), this);
        g_signal_connect(m_window, "delete-event", G_CALLBACK(onDeleteEvent), this);
        g_signal_connect(m_listBox, "row-activated", G_CALLBACK(onRowActivated), this);
    }
    
    // Remplir la liste avec toutes les commandes
    filterCommands("");
    gtk_widget_show_all(m_window);
    gtk_widget_grab_focus(m_entry);
}

void CommandPalette::hidePalette() {
    if (m_window && gtk_widget_get_visible(m_window)) {
        gtk_widget_hide(m_window);
    }
}

bool CommandPalette::isVisible() const {
    return m_window && gtk_widget_get_visible(m_window);
}

void CommandPalette::filterCommands(const std::string& text) {
    // Vider la liste
    GList* children = gtk_container_get_children(GTK_CONTAINER(m_listBox));
    // Détruire les children de manière sécurisée
    for (GList* iter = children; iter != nullptr; iter = iter->next) {
        GtkWidget* child = GTK_WIDGET(iter->data);
        if (child && GTK_IS_WIDGET(child) && !gtk_widget_in_destruction(child)) {
            gtk_widget_destroy(child);
        }
    }
    g_list_free(children);
    
    // Filtrer les commandes
    for (const auto& cmd : m_commands) {
        if (text.empty() || cmd.find(text) != std::string::npos) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* label = gtk_label_new(cmd.c_str());
            gtk_label_set_xalign(GTK_LABEL(label), 0.0);
            gtk_container_add(GTK_CONTAINER(row), label);
            gtk_container_add(GTK_CONTAINER(m_listBox), row);
        }
    }
    
    gtk_widget_show_all(m_listBox);
    
    // Sélectionner la première ligne
    GtkListBoxRow* firstRow = gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), 0);
    if (firstRow) {
        gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), firstRow);
    }
}

void CommandPalette::processCommand(const std::string& command) {
    if (command.find("/cve") != std::string::npos) {
        processCVECommand(command);
    } else if (command.find("/request") != std::string::npos) {
        processRequestCommand(command);
    } else if (command == "/analyze") {
        showRequestAnalyzer();
    } else if (command == "/history") {
        // TODO: Afficher l'history
        std::cout << "Affichage de l'history..." << std::endl;
    } else if (command == "/clear cache") {
        // TODO: Vider le cache
        std::cout << "Vidage du cache..." << std::endl;
    }
    
    if (onCommandSelected) {
        onCommandSelected(command);
    }
}

void CommandPalette::processCVECommand(const std::string& command) {
    if (!m_currentWebView) {
        std::cerr << "Aucune vue web active" << std::endl;
        return;
    }
    
    // TODO: Récupérer le HTML de la page actuelle
    std::string html = ""; // m_currentWebView->getHTML();
    auto cves = CVEAnalyzer::detectCVEs(html);
    CVEAnalyzer::displayCVEResults(cves);
}

void CommandPalette::processRequestCommand(const std::string& command) {
    // TODO: Implémenter l'envoi de requêtes GET/POST
    std::cout << "Traitement de la requête: " << command << std::endl;
}

void CommandPalette::showRequestAnalyzer() {
    if (m_requestInterceptor) {
        // TODO: Afficher les requêtes interceptées
        std::cout << "Affichage de l'analyseur de requêtes..." << std::endl;
    }
}

gboolean CommandPalette::onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
    auto* palette = static_cast<CommandPalette*>(user_data);
    
    // Échap pour fermer
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(palette->m_window);
        return TRUE;
    }
    
    // CTRL+SHIFT+C pour fermer (toggle)
    if ((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
        event->keyval == GDK_KEY_c) {
        gtk_widget_hide(palette->m_window);
        return TRUE;
    }
    
    return FALSE;
}

void CommandPalette::onEntryChanged(GtkEntry* entry, gpointer user_data) {
    auto* palette = static_cast<CommandPalette*>(user_data);
    const gchar* text = gtk_entry_get_text(entry);
    palette->filterCommands(text ? text : "");
}

void CommandPalette::onRowActivated(GtkListBox* listBox, GtkListBoxRow* row, gpointer user_data) {
    auto* palette = static_cast<CommandPalette*>(user_data);
    
    GtkWidget* label = gtk_bin_get_child(GTK_BIN(row));
    const gchar* text = gtk_label_get_text(GTK_LABEL(label));
    
    if (text) {
        palette->processCommand(text);
        gtk_widget_hide(palette->m_window);
    }
}

gboolean CommandPalette::onDeleteEvent(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
    gtk_widget_hide(widget);
    return TRUE;
}

void CommandPalette::setCurrentWebView(RenderingEngine* webView) {
    m_currentWebView = webView;
}

void CommandPalette::setRequestInterceptor(RequestInterceptor* interceptor) {
    m_requestInterceptor = interceptor;
}

