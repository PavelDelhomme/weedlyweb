#include "GestionnaireFavoris.h"
#include "Utils.h"  // Ajout de l'import
#include "GestionnaireFichiers.h"
#include <iostream>
#include <algorithm>
#include <gtk/gtk.h>

GestionnaireFavoris::GestionnaireFavoris(std::shared_ptr<nlohmann::json> favoris, std::function<void()> callbackRafraichir)
    : favoris(std::move(favoris)), callbackRafraichir(callbackRafraichir) {
    creerInterface();
}

GestionnaireFavoris::~GestionnaireFavoris() {
    if (fenetre) gtk_widget_destroy(fenetre);
}

void GestionnaireFavoris::fermerFenetre() {
    if (fenetre) {
        gtk_widget_destroy(fenetre);
        fenetre = nullptr;
    }
}

static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
    if (gestionnaire) {
        gestionnaire->fermerFenetre();
    }
}

static void on_supprimer_favori(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
    
    // Utilisation de l'accesseur public
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gestionnaire->getListeFavoris()));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *nomFavori;
        gtk_tree_model_get(model, &iter, 0, &nomFavori, -1); // Colonne 0 = Nom du favori
        gestionnaire->supprimerFavori(nomFavori);
        g_free(nomFavori);
    } else {
        std::cerr << "Aucun favori sélectionné pour suppression." << std::endl;
    }
}


void GestionnaireFavoris::creerInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "Gestionnaire de Favoris");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 600, 400);

    GtkWidget *conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Liste des favoris
    listeFavoris = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), listeFavoris, TRUE, TRUE, 0);

    // Bouton de fermeture
    GtkWidget *boutonFermer = gtk_button_new_with_label("Fermer");
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(on_bouton_fermer_clicked), this);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), boutonFermer, FALSE, FALSE, 0);
    
    // Bouton d'ajout
    GtkWidget *boutonAjouterFavori = gtk_button_new_with_label("Ajouter Favori");
    g_signal_connect(boutonAjouterFavori, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
        if (gestionnaire) {
            gestionnaire->ajouterFavori("Nouveau Favori", "https::example.com", "Exemple");
        }
    }), this);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), boutonAjouterFavori, FALSE, FALSE, 0);

    // Bouton supprimer (Correction : déplacement ici dans le constructeur)
    GtkWidget *supprimerItem = gtk_button_new_with_label("Supprimer");
    g_signal_connect(supprimerItem, "clicked", G_CALLBACK(on_supprimer_favori), this);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), supprimerItem, FALSE, FALSE, 0);

    afficherListeFavoris();
    gtk_widget_show_all(fenetre);
}

void GestionnaireFavoris::afficherListeFavoris() {
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    // Parcourir et afficher les favoris
    for (const auto& favori : favoris) {
        if (favori.contains("name") && favori.contains("url")) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, favori["name"].get<std::string>().c_str(),
                               1, favori["url"].get<std::string>().c_str(),
                               -1);
        }
    }

    // Configuration des colonnes
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *colNom = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 0, NULL);
    GtkTreeViewColumn *colURL = gtk_tree_view_column_new_with_attributes("URL", renderer, "text", 1, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colNom);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colURL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(listeFavoris), GTK_TREE_MODEL(store));
    g_object_unref(store); // Libérer correctement la mémoire
}

// void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
//     for (const auto& favori : *favoris) {
//         if (favori["url"] == url) {
//             std::cerr << "Favori déjà existant : " << url << std::endl;
//             return;
//         }
//     }
//     favoris->push_back({{"name", nom}, {"url", url}, {"tag", tag}});
//     gestionnaireFavoris->sauvegarderModifications();
//     rafraichirBarreFavoris();
//     callbackRafraichir();
//     rafraichirInterface();
// }


// void GestionnaireFavoris::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
//     if (verifierDoublonFavori(favoris, url)) {
//         afficherMessageConsole("Impossible d'ajouter, le favori existe déjà.");
//         return;
//     }

//     favoris->push_back({{"name", nom}, {"url", url}, {"tag", tag}});
//     gestionnaireFavoris->sauvegarderModifications();
//     callbackRafraichir();
//     rafraichirInterface();
// }
// Ajout d'un favori (évite les duplications)

void GestionnaireFavoris::ajouterDossier(const std::string& nom) {
    favoris.push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    callbackRafraichir();
    rafraichirInterface();
}

// void GestionnaireFavoris::supprimerFavori(const std::string& nomFavori) {
//     auto it = std::remove_if(favoris.begin(), favoris.end(), [&](const nlohmann::json& favori) {
//         return favori.contains("name") && favori["name"] == nomFavori;
//     });

//     if (it != favoris.end()) {
//         favoris.erase(it, favoris.end());
//         callbackRafraichir();
//         sauvegarderModifications();
//         rafraichirInterface();
//     } else {
//         std::cerr << "Favori introuvable : " << nomFavori << std::endl;
//     }
// }
void GestionnaireFavoris::ajouterFavori(const std::string& nom, const std::string& tag) {
    if (verifierDoublonFavori(favoris, url)) {
        afficherMessageConsole("Le favori existe déjà"),
            // favoris->push_back({{"name", nom}, {"url", url}, {"tag", tag}});
        return
    }
    favoris->push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    sauvegarderModifications();
    callbackRafraichir();
    rafraichirInterface();
}

// void GestionnaireFavoris::supprimerFavori(const std::string& nomFavori) {
//     auto it = std::remove_if(favoris.begin(), favoris.end(), [&](const nlohmann::json& favori) {
//         return favori.contains("name") && favori["name"] == nomFavori;
//     });

//     if (it != favoris.end()) {
//         favoris.erase(it, favoris.end());
//         sauvegarderModifications();
//         rafraichirInterface();
//     }
// }


void GestionnaireFavoris::supprimerFavori(const std::string& nomFavori) {
    if (supprimerFavoriDeListe(favoris, nomFavori)) {
        sauvegarderModifications();
        rafraichirInterface();
    } else {
        afficherMessageConsole("Favori non trouvé.");
    }
}


void GestionnaireFavoris::modifierFavori(const std::string& nomFavori, const std::string& nouvelURL) {
    for (auto& favori : favoris) {
        if (favori["name"] == nomFavori) {
            favori["url"] = nouvelURL;
            callbackRafraichir();
            rafraichirInterface();
            return;
        }
    }
    std::cerr << "Favori non trouvé : " << nomFavori << std::endl;
}


void GestionnaireFavoris::creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori) {
    GtkWidget *menu = gtk_menu_new();

    // Ouvrir dans un nouvel onglet
    GtkWidget *ouvrirNouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    g_signal_connect(ouvrirNouvelOngletItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
    gestionnaire->callbackRafraichir();
    }), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOngletItem);

    // Modifier le favori
    GtkWidget *modifierItem = gtk_menu_item_new_with_label("Modifier");
    g_signal_connect(modifierItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
        std::string nom = static_cast<const char*>(g_object_get_data(G_OBJECT(user_data), "favori_nom"));
        gestionnaire->modifierFavori(nom, "https://example.com");
    }), this);

    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    // Supprimer le favori
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data(G_OBJECT(supprimerItem), "nomFavori", (gpointer)new std::string(nomFavori));

    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget* widget, gpointer user_data) {
        auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
        std::string* nomFavori = static_cast<std::string*>(g_object_get_data(G_OBJECT(widget), "nomFavori"));
        gestionnaire->supprimerFavori(*nomFavori);
        delete nomFavori;  // Libération de la mémoire allouée
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    // Gérer les favoris (ouvre la fenêtre complète)
    GtkWidget *gererFavorisItem = gtk_menu_item_new_with_label("Gérer les Favoris");
    g_signal_connect(gererFavorisItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
        gestionnaire->afficherFenetre();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gererFavorisItem);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}


void GestionnaireFavoris::sauvegarderModifications() {
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), favoris);
}

void GestionnaireFavoris::rafraichirInterface() {
    if (listeFavoris) {
        gtk_widget_destroy(listeFavoris);
    }
    listeFavoris = gtk_tree_view_new();  // Réinitialisation complète de la liste
    afficherListeFavoris();  // Recharge les favoris actualisés
}

GtkWidget* GestionnaireFavoris::getListeFavoris() {
    return listeFavoris;
}


void GestionnaireFavoris::afficherFenetre() {
    gtk_widget_show_all(fenetre);
}
