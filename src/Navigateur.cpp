#include "Navigateur.h"
#include "GestionnaireFichiers.h"
#include "GestionnaireFavoris.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>

std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
}

Navigateur::Navigateur() 
    : moteurRendu(std::make_unique<MoteurRendu>()),
      gestionnaireHTTP(std::make_unique<GestionnaireHTTP>()),
      gestionnaireMemoire(std::make_unique<GestionnaireMemoire>()),
      moteurScript(std::make_unique<MoteurScript>()) {
    chargerConfiguration();
    construireInterface();
}


Navigateur::~Navigateur() {
    if (fenetre) gtk_widget_destroy(fenetre);    
}

void Navigateur::construireInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);

    // Conteneur principal
    conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Ajout de la barre d'onglets
    initialiserBarreOnglets();
    chargerURL(homepage);
    initialiserBarreNavigation();
    initialiserBarreFavoris();
    moteurRendu->initialiserRendu(conteneurPrincipal);

    moteurRendu->connecterSignalURLChangee([this](const std::string& url) {
        if (barreURL) {
            gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
        }
    });

    g_signal_connect(fenetre, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
        static_cast<Navigateur *>(data)->fermerApplication();
    }), this);

    gtk_widget_show_all(fenetre);
}
void Navigateur::ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *bouton = moteurRendu->creerBouton(iconName, callback, data);
    gtk_box_pack_start(GTK_BOX(conteneur), bouton, FALSE, FALSE, 0);
}

GtkWidget* obtenirDernierEnfant(GtkWidget* parent) {
    GList* enfants = gtk_container_get_children(GTK_CONTAINER(parent));
    return g_list_last(enfants) ? GTK_WIDGET(g_list_last(enfants)->data) : nullptr;
}

GtkWidget* obtenirPremierEnfant(GtkWidget* parent) {
    GList* enfants = gtk_container_get_children(GTK_CONTAINER(parent));
    return enfants ? GTK_WIDGET(enfants->data) : nullptr;
}

void Navigateur::initialiserBarreNavigation() {
    barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(&Navigateur::onNaviguerRetourWrapper), this);
    ajouterBouton(barreNavigation, "go-next", G_CALLBACK(&Navigateur::onNaviguerSuivantWrapper), this);
    ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(&Navigateur::onRafraichirPageWrapper), this);
    ajouterBouton(barreNavigation, "go-home", G_CALLBACK(&Navigateur::onAllerAccueil), this);

    barreURL = moteurRendu->creerChampTexte(G_CALLBACK(&Navigateur::onBarreURLActivate), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);

    GtkWidget *boutonFavoris = moteurRendu->creerBouton("star", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        std::string url = n->moteurRendu->obtenirURLActuelle();
        if (!url.empty()) {
            GestionnaireFavoris gestionnaire(n->favoris, [n]() {
                n->rafraichirBarreFavoris();
            });
            gestionnaire.ajouterFavori("Favori", url, "");
            n->rafraichirBarreFavoris();
        }
    }), this);

    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonFavoris, FALSE, FALSE, 0);
    gkt_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
}

void Navigateur::initialiserBarreFavoris() {
    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *boutonGestionFavoris = moteurRendu->creerBouton("folder", G_CALLBACK([](GtkButton*, gpointer user_data) {
        auto *navigateur = static_cast<Navigateur*>(user_data);
        GestionnaireFavoris gestionnaireFavoris(navigateur->favoris, [navigateur]() {
            navigateur->rafraichirBarreFavoris();
        });
        gestionnaireFavoris.afficherFenetre();
    }), this);

    gtk_box_pack_start(GTK_BOX(barreFavoris), boutonGestionFavoris, FALSE, FALSE, 0);

    // Chargement des favoris
    for (const auto& favori : favoris) {
        if (favori.contains("name") && favori.contains("url")) {
            GtkWidget *boutonFavori = moteurRendu->creerBouton(favori["name"], G_CALLBACK(&Navigateur::onCliqueFavoriWrapper),
                                                                new std::pair<Navigateur*, std::string>(this, favori["url"]));
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 0);
        }
    }

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 0);
    gtk_widget_show_all(conteneurPrincipal);
}


void Navigateur::rafraichirBarreFavoris() {
    gtk_widget_destroy(barreFavoris);
    initialiserBarreFavoris();
}


void Navigateur::onCliqueFavoriWrapper(GtkButton *button, gpointer user_data) {
    auto *data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    if (data) {
        data->first->chargerURL(data->second);
        delete data; // Libérez la mémoire
    }
}


void Navigateur::initialiserBarreOnglets() {
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    ajouterNouvelOnglet(homepage);

    // Ajouter le bouton "+"
    GtkWidget *boutonAjouterOnglet = moteurRendu->creerBouton("list-add", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        n->ajouterNouvelOnglet(n->homepage);
    }), this);

    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonAjouterOnglet, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
}


void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *labelTitre = gtk_label_new("Nouvel Onglet");
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, FALSE, FALSE, 0);

    GtkWidget *boutonFermer = moteurRendu->creerBouton("window-close", G_CALLBACK(+[](GtkButton *button, Navigateur *n) {
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        n->supprimerOnglet(parent);
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(barreOnglets), hboxOnglet, FALSE, FALSE, 0);
    onglets.push_back({url, hboxOnglet});

    // Correct : lambda avec capture explicite de `this`
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton*, gpointer user_data) {
        auto* n = static_cast<Navigateur*>(user_data);
        if (n) {
            n->changerOngletActif(widget);
        }
        return FALSE;
    }), this);


    moteurRendu->afficherPage(url);
    moteurRendu->connecterSignalPageChargee([labelTitre](const std::string &titre) {
        gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
    });

    gtk_widget_show_all(barreOnglets);
}

void Navigateur::changerOngletActif(GtkWidget* ongletWidget) {
    for (auto &[url, widget] : onglets) {
        if (widget == ongletWidget) {
            moteurRendu->afficherPage(url);
            mettreEnSurbrillance(widget);
            return;
        }
    }
}



void Navigateur::executerScriptDansOngletActif(const std::string& script) {
    if (!onglets.empty()) {
        GtkWidget* ongletActif = onglets.back().second;
        WebKitWebView* vueWeb = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(ongletActif)));
        moteurScript->executerScript(vueWeb, script);
    }
}

void Navigateur::supprimerOnglet(GtkWidget *ongletWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(onglets.begin(), onglets.end(), [ongletWidget](const auto &pair) {
        return pair.second == ongletWidget;
    });

    if (it != onglets.end()) {
        GtkWidget *hboxOnglet = it->second;

        // Conversion explicite vers WebKitWebView*
        WebKitWebView *vueWeb = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(hboxOnglet)));
        if (vueWeb && WEBKIT_IS_WEB_VIEW(vueWeb)) {
            gestionnaireMemoire->hibernerOnglet(vueWeb); // Mise en veille de l'onglet
        }

        // Supprimer l'onglet de la liste
        gtk_widget_destroy(hboxOnglet);
        onglets.erase(it);

        // Si aucun onglet n'est présent, fermer l'application
        if (onglets.empty()) {
            fermerApplication();
        } else {
            // Charger l'URL du premier onglet
            chargerURL(onglets.front().first);
        }
    }
}

void Navigateur::mettreEnSurbrillance(GtkWidget *ongletWidget) {
    for (auto &[url, widget] : onglets) {
        gtk_widget_set_name(widget, widget == ongletWidget ? "onglet-actif": "onlget-inactif");
    }
}

void Navigateur::chargerURL(const std::string& url) {
    moteurRendu->afficherPage(url);
    historique.push_back(url);

    if (barreURL) {
        gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
    }

    // Mettre à jour le titre de l'onglet actif
    moteurRendu->connecterSignalPageChargee([this](const std::string &titre) {
        if (!onglets.empty()) {
            GtkWidget *hboxOnglet = onglets.back().second;
            GtkWidget *labelTitre = obtenirPremierEnfant(GTK_WIDGET(hboxOnglet));
            gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
        }
    });
    gestionnaireMemoire->optimiserMemoire();
}

void Navigateur::afficherMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}


void Navigateur::chargerConfiguration() {
    std::string chemin = GestionnaireFichiers::cheminConfigJSON();
    nlohmann::json config = GestionnaireFichiers::lireJSON(chemin);
    
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://www.duckduckgo.com";
        GestionnaireFichiers::ecrireJSON(chemin, config);
    }
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminConfigJSON(), config);
}

void Navigateur::chargerFavoris() {
    std::string chemin = GestionnaireFichiers::cheminFavorisJSON();
    favoris = GestionnaireFichiers::lireJSON(chemin);
    
    if (favoris.is_null() || !favoris.is_array() || favoris.empty()) {
        std::cerr << "Fichier favoris.json introuvable, invalide ou vide. Initialisation avec un favori par défaut." << std::endl;

        // Ajouter un favori par défaut
        favoris = nlohmann::json::array({
            {{"name", "DuckDuckGo"}, {"url", "https://www.duckduckgo.com"}, {"tag", "Recherche"}}
        });

        GestionnaireFichiers::ecrireJSON(chemin, favoris); // Sauvegarde du fichier vide
    } else {
        std::cout << "Favoris chargés avec succès : " << favoris.dump(4) << std::endl;
    }

    // Éviter les doublons
    std::set<std::string> urls;
    favoris.erase(std::remove_if(favoris.begin(), favoris.end(), [&urls](const nlohmann::json& favori) {
        return !favori.contains("url") || !urls.insert(favori["url"]).second;
    }), favoris.end());
}

void Navigateur::sauvegarderFavoris() {
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), favoris);
}


void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    // Vérifie si l'URL existe déjà
    for (const auto& favori : favoris) {
        if (favori["url"] == url) {
            std::cerr << "Favori déjà existant : " << url << std::endl;
            return;
        }
    }

    // Ajoute le nouveau favori
    favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    sauvegarderFavoris();
}


void Navigateur::ajouterDossierFavoris(const std::string& nom) {
    favoris.push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    sauvegarderFavoris();
}


void Navigateur::ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url, const std::string& tag) {
    for (auto& favori : favoris) {
        if (favori["name"] == dossier && favori["type"] == "folder") {
            favori["children"].push_back({{"name", nom}, {"url", url}, {"tag", tag}});
            sauvegarderFavoris();
            return;
        }
    }
    std::cerr << "Dossier introuvable : " << dossier << std::endl;
}

void Navigateur::creerMenuContextuelFavoris(GtkWidget* bouton, const nlohmann::json& favori) {
    GtkWidget *menu = gtk_menu_new();

    // Modifier le favori
    GtkWidget *modifierItem = gtk_menu_item_new_with_label("Modifier");
    g_signal_connect(modifierItem, "activate", G_CALLBACK(+[](GtkWidget*, Navigateur* navigateur, nlohmann::json favori) {
        navigateur->modifierFavori(favori);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    // Supprimer le favori
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget*, Navigateur* navigateur, nlohmann::json favori) {
        navigateur->supprimerFavori(favori);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}

void Navigateur::modifierFavori(const nlohmann::json& favori) {
    std::cout << "Modification du favori : " << favori.dump() << std::endl;
}

void Navigateur::supprimerFavori(const nlohmann::json& favori) {
    std::cout << "Suppression du favori : " << favori.dump() << std::endl;
    favoris.erase(std::remove(favoris.begin(), favoris.end(), favori), favoris.end());
    sauvegarderFavoris();
    rafraichirBarreFavoris(); 
}

void Navigateur::afficherFavoris(const nlohmann::json& favoris) {
    gtk_widget_destroy(barreFavoris); // Supprime la barre actuelle
    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    for (const auto& favori : favoris) {
        if (!favori.contains("name") || !favori.contains("url")) {
            continue;
        }

        GtkWidget *boutonFavori = moteurRendu->creerBouton(
            favori["name"].get<std::string>(),
            G_CALLBACK(&Navigateur::onCliqueFavoriWrapper),
            new std::pair<Navigateur*, std::string>(this, favori["url"].get<std::string>())
        );
        gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 0);
    gtk_widget_show_all(conteneurPrincipal);
}


void Navigateur::naviguerDansDossier(const std::string& dossier) {
    for (const auto& favori : favoris) {
        if (favori["name"] == dossier && favori["type"] == "folder") {
            afficherFavoris(favori["children"]);
            return;
        }
    }
    std::cerr << "Dossier introuvable : " << dossier << std::endl;
}

void Navigateur::creerDossierFavoris(const std::string& nom) {
    favoris.push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    sauvegarderFavoris();
    rafraichirBarreFavoris();
}


void Navigateur::deplacerFavori(const std::string& nomFavori, const std::string& dossierDestination) {
    nlohmann::json* dossierCible = nullptr;
    nlohmann::json favoriADeplacer;

    // Trouver le dossier cible
    for (auto& favori : favoris) {
        if (favori["name"] == dossierDestination && favori["type"] == "folder") {
            dossierCible = &favori["children"];
            break;
        }
    }

    if (!dossierCible) {
        std::cerr << "Dossier introuvable : " << dossierDestination << std::endl;
        return;
    }

    // Trouver et retirer le favori de la liste principale
    auto it = std::find_if(favoris.begin(), favoris.end(), [&nomFavori](const auto& f) {
        return f["name"] == nomFavori;
    });

    if (it != favoris.end()) {
        favoriADeplacer = *it;
        favoris.erase(it);
        dossierCible->push_back(favoriADeplacer);
        sauvegarderFavoris();
        rafraichirBarreFavoris();
    } else {
        std::cerr << "Favori introuvable : " << nomFavori << std::endl;
    }
}

void Navigateur::afficherParametres() {
    std::string cheminParametres = GestionnaireFichiers::cheminParametresHTML();
    moteurRendu->afficherPage("file://" + cheminParametres);
}


void Navigateur::configurerRaccourcisClavier() {
    g_signal_connect(fenetre, "key-press-event", G_CALLBACK(+[](GtkWidget *, GdkEventKey *event, Navigateur *n) {
        if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_t) {
            n->ajouterNouvelOnglet();
        } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w) {
            if (!n->onglets.empty()) {
                n->supprimerOnglet(n->onglets.back().second);
            }
        } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d) {
            std::string url = n->moteurRendu->obtenirURLActuelle();
            if (!url.empty()) {
                n->ajouterFavori("Favori", url, "");
                n->rafraichirBarreFavoris();
            }
        }
        return FALSE;
    }), this);
}


void Navigateur::chargerStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    std::string cheminCSS = GestionnaireFichiers::cheminStylesCSS();
    gtk_css_provider_load_from_path(provider, cheminCSS.c_str(), &error);

    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
    }

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}


void Navigateur::onNaviguerRetour(GtkButton *, Navigateur *n) {
    n->moteurRendu->naviguerRetour();
}

void Navigateur::onNaviguerSuivant(GtkButton *, Navigateur *n) {
    n->moteurRendu->naviguerSuivant();
}

void Navigateur::onRafraichirPage(GtkButton *, Navigateur *n) {
    n->chargerURL(n->moteurRendu->obtenirURLActuelle());
}

void Navigateur::onAllerAccueil(GtkButton *, Navigateur *n) {
    n->chargerURL(n->homepage);
}

void Navigateur::onAjouterFavori(GtkButton *, Navigateur *n) {
    std::string url = n->moteurRendu->obtenirURLActuelle();
    if (!url.empty()) {
        n->ajouterFavori("Favori", url, "");
        n->rafraichirBarreFavoris();
    }
}

void Navigateur::onBarreURLActivate(GtkEntry *entry, Navigateur *n) {
    n->chargerURL(gtk_entry_get_text(entry));
}

void Navigateur::onCliqueFavori(GtkButton *, Navigateur *n, const std::string& url) {
    n->chargerURL(url);
}

void Navigateur::fermerApplication() {
    sauvegarderConfiguration();
    if (fenetre) gtk_widget_destroy(fenetre);
    gtk_main_quit();
}


void Navigateur::onNaviguerRetourWrapper(GtkButton *button, gpointer user_data) {
    auto *navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->onNaviguerRetour(button, navigateur);
    }
}


void Navigateur::onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data) {
    auto *navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->onNaviguerSuivant(button, navigateur);
    }
}

void Navigateur::onRafraichirPageWrapper(GtkButton *button, gpointer user_data) {
    auto *navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->onRafraichirPage(button, navigateur);
    }
}