
// void Browser::createContextMenu(GtkWidget* button) {
//     GtkWidget *menu = gtk_menu_new();

//     GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter Favori");
//     // g_signal_connect(ajouterItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
//     //     auto* navigateur = static_cast<Browser*>(user_data);
//     //     std::string urlActuelle = navigateur->getCurrentURL();
//     //     navigateur->addFavorite("Nouveau Favori", urlActuelle, "Général");
//     // }), this);
//     g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);

//     // // Création des champs de saisie pour URL et nom
//     // GtkWidget *entryNom = gtk_entry_new();
//     // gtk_entry_set_placeholder_text(GTK_ENTRY(entryNom), "Nom du favori");
//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), entryNom);

//     // GtkWidget *entryURL = gtk_entry_new();
//     // gtk_entry_set_placeholder_text(GTK_ENTRY(entryURL), "URL");
//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), entryURL);

//     // // Correction ici : changement de nom de la variable
//     // GtkWidget *ajouterItem2 = gtk_menu_item_new_with_label("Confirmer l'ajout");
//     // auto* data = new std::pair<Browser*, std::pair<GtkWidget*, GtkWidget*>>(this, {entryNom, entryURL});
//     // g_signal_connect(ajouterItem2, "activate", G_CALLBACK(on_ajouter_favori_menu), data);

//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem2);



//     gtk_widget_show_all(menu);
//     gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
// }

// void Browser::createContextMenu(GtkWidget* button) {
//     GtkWidget *menu = gtk_menu_new();

//     // Créer un item pour ajouter un favori
//     GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter Favori");
//     g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);

//     // Créer un item pour gérer les favorites
//     GtkWidget *gererFavorisItem = gtk_menu_item_new_with_label("Gérer les Favoris");
//     g_signal_connect(gererFavorisItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
//         auto* navigateur = static_cast<Browser*>(user_data);
//         navigateur->showFavoritesManager();
//     }), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), gererFavorisItem);

//     // Afficher le menu correctement
//     gtk_widget_show_all(menu);
//     gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
// }


    // addButton(navigationBar, "go-previous", G_CALLBACK(&Browser::onNavigateBackWrapper), this);
    // addButton(navigationBar, "go-next", G_CALLBACK(&Browser::onNavigateForwardWrapper), this);
    // addButton(navigationBar, "view-refresh", G_CALLBACK(&Browser::onRefreshPageWrapper), this);
    // addButton(navigationBar, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    // addButton(navigationBar, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    // addButton(navigationBar, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    //gtk_widget_show_all(mainContainer);
        // GtkWidget *boutonAjouterOnglet = renderingEngine->createButton("list-add", G_CALLBACK(+[](GtkButton *, Browser *n) {
    //     n->addNewTab(n->homepage);
    // }), this);
    // void Browser::onUrlBarActivate(GtkEntry *entry, Browser *n) {
//     n->loadURL(gtk_entry_get_text(entry));
// }
    // auto *navigateur = static_cast<Browser*>(user_data);
    // if (navigateur) {
    //     navigateur->onNavigateForward(button, navigateur);
    // }
    // auto *navigateur = static_cast<Browser*>(user_data);
    // if (navigateur) {
    //     navigateur->onRefreshPage(button, navigateur);
    // }

    // addButton(navigationBar, "go-home", G_CALLBACK(&Browser::onGoHome), this);

