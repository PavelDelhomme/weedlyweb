#include "managers/FavoritesManager.h"
#include "utils/Utils.h"
#include "managers/FileManager.h"
#include <iostream>
#include <algorithm>

namespace {

bool findAndErase(nlohmann::json& arr, const std::string& name, nlohmann::json* extracted = nullptr) {
    if (!arr.is_array()) {
        return false;
    }
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        if (it->is_object() && it->contains("name") && (*it)["name"].get<std::string>() == name) {
            if (extracted) {
                *extracted = *it;
            }
            arr.erase(it);
            return true;
        }
        if (FavoritesJson::isFolder(*it)) {
            if (findAndErase((*it)["children"], name, extracted)) {
                return true;
            }
        }
    }
    return false;
}

bool findFolderArray(nlohmann::json& root, const std::string& folderName, nlohmann::json** out) {
    if (!root.is_array()) {
        return false;
    }
    for (auto& item : root) {
        if (FavoritesJson::isFolder(item) && item.value("name", "") == folderName) {
            *out = &item["children"];
            return true;
        }
        if (FavoritesJson::isFolder(item)) {
            if (findFolderArray(item["children"], folderName, out)) {
                return true;
            }
        }
    }
    return false;
}

bool renameItem(nlohmann::json& arr, const std::string& oldName, const std::string& newName, const std::string& newUrl, bool touchUrl) {
    for (auto& it : arr) {
        if (FavoritesJson::isFolder(it)) {
            if (it.value("name", "") == oldName) {
                it["name"] = newName;
                return true;
            }
            if (renameItem(it["children"], oldName, newName, newUrl, touchUrl)) {
                return true;
            }
        } else if (it.contains("name") && it["name"].get<std::string>() == oldName) {
            it["name"] = newName;
            if (touchUrl) {
                it["url"] = newUrl;
            }
            return true;
        }
    }
    return false;
}

std::vector<std::string> collectFolderNames(const nlohmann::json& arr, const std::string& exclude = "") {
    std::vector<std::string> names;
    if (!arr.is_array()) {
        return names;
    }
    for (const auto& it : arr) {
        if (FavoritesJson::isFolder(it)) {
            const std::string n = it.value("name", "");
            if (!n.empty() && n != exclude) {
                names.push_back(n);
            }
            auto nested = collectFolderNames(it["children"], exclude);
            names.insert(names.end(), nested.begin(), nested.end());
        }
    }
    return names;
}

void on_row_more_clicked(GtkButton* b, gpointer u) {
    auto* self = static_cast<FavoritesManager*>(u);
    nlohmann::json fake;
    const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "item-name"));
    const char* ur = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "item-url"));
    const bool isFolder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "is-folder")) != 0;
    if (isFolder) {
        fake = {{"name", n ? n : ""}, {"type", "folder"}, {"children", nlohmann::json::array()}};
    } else {
        fake = {{"name", n ? n : ""}, {"url", ur ? ur : ""}};
    }
    self->showItemMenu(GTK_WIDGET(b), fake);
}

void on_row_edit_clicked(GtkButton* b, gpointer u) {
    auto* self = static_cast<FavoritesManager*>(u);
    const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "item-name"));
    const char* ur = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "item-url"));
    const bool isFolder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "is-folder")) != 0;
    self->promptRename(n ? n : "", ur ? ur : "", isFolder);
}

void on_row_delete_clicked(GtkButton* b, gpointer u) {
    auto* self = static_cast<FavoritesManager*>(u);
    const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "item-name"));
    if (n) {
        self->removeFavorite(n);
    }
}

} // namespace

FavoritesManager::FavoritesManager(std::shared_ptr<nlohmann::json> favorites, std::function<void()> callbackRafraichir)
    : favorites(std::move(favorites)), callbackRafraichir(std::move(callbackRafraichir)) {
    createInterface();
}

FavoritesManager::~FavoritesManager() {
    closeWindow();
}

void FavoritesManager::closeWindow() {
    if (window && GTK_IS_WIDGET(window)) {
        gtk_widget_destroy(window);
    }
    window = nullptr;
    favoritesList = nullptr;
    scrolled = nullptr;
    pathLabel = nullptr;
    backButton = nullptr;
    forwardButton = nullptr;
    upButton = nullptr;
    deleteButton = nullptr;
    addForm = nullptr;
    nameEntry = nullptr;
    urlEntry = nullptr;
    toolbar = nullptr;
}

nlohmann::json& FavoritesManager::currentArray() {
    nlohmann::json* cur = favorites.get();
    for (const auto& segment : currentPath) {
        bool found = false;
        for (auto& item : *cur) {
            if (FavoritesJson::isFolder(item) && item.value("name", "") == segment) {
                cur = &item["children"];
                found = true;
                break;
            }
        }
        if (!found) {
            currentPath.clear();
            return *favorites;
        }
    }
    return *cur;
}

void FavoritesManager::setPageContext(const std::string& title, const std::string& url) {
    pageTitle = title;
    pageUrl = url;
}

void FavoritesManager::createInterface() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de favoris");
    gtk_window_set_default_size(GTK_WINDOW(window), 720, 480);
    gtk_widget_set_name(window, "favorites-manager");
    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        static_cast<FavoritesManager*>(user_data)->closeWindow();
        return TRUE;
    }), this);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);

    // Navigation
    GtkWidget* nav = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(nav, 8);
    gtk_widget_set_margin_end(nav, 8);
    gtk_widget_set_margin_top(nav, 8);
    gtk_widget_set_margin_bottom(nav, 4);

    backButton = gtk_button_new_with_label("←");
    gtk_widget_set_tooltip_text(backButton, "Précédent");
    gtk_widget_set_sensitive(backButton, FALSE);
    g_signal_connect(backButton, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->navigateBack();
    }), this);

    forwardButton = gtk_button_new_with_label("→");
    gtk_widget_set_tooltip_text(forwardButton, "Suivant");
    gtk_widget_set_sensitive(forwardButton, FALSE);
    g_signal_connect(forwardButton, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->navigateForward();
    }), this);

    upButton = gtk_button_new_with_label("↑");
    gtk_widget_set_tooltip_text(upButton, "Dossier parent");
    gtk_widget_set_sensitive(upButton, FALSE);
    g_signal_connect(upButton, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->navigateUp();
    }), this);

    pathLabel = gtk_label_new("Favoris");
    gtk_widget_set_halign(pathLabel, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(pathLabel), PANGO_ELLIPSIZE_MIDDLE);

    gtk_box_pack_start(GTK_BOX(nav), backButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav), forwardButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav), upButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav), pathLabel, TRUE, TRUE, 8);
    gtk_box_pack_start(GTK_BOX(root), nav, FALSE, FALSE, 0);

    // Toolbar
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 8);
    gtk_widget_set_margin_bottom(toolbar, 4);

    GtkWidget* addBtn = gtk_button_new_with_label("Ajouter un favori");
    g_signal_connect(addBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->showAddForm(true);
    }), this);

    GtkWidget* folderBtn = gtk_button_new_with_label("Nouveau dossier");
    g_signal_connect(folderBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        GtkWidget* dlg = gtk_dialog_new_with_buttons(
            "Nouveau dossier", GTK_WINDOW(self->window),
            static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            "Annuler", GTK_RESPONSE_CANCEL, "Créer", GTK_RESPONSE_ACCEPT, nullptr);
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nom du dossier");
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), entry);
        gtk_widget_show_all(dlg);
        if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
            const gchar* t = gtk_entry_get_text(GTK_ENTRY(entry));
            if (t && *t) {
                self->addFolder(t);
            }
        }
        gtk_widget_destroy(dlg);
    }), this);

    deleteButton = gtk_button_new_with_label("Supprimer la sélection");
    gtk_widget_set_no_show_all(deleteButton, TRUE);
    gtk_widget_hide(deleteButton);
    g_signal_connect(deleteButton, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        if (!self->selectedName.empty()) {
            self->removeFavorite(self->selectedName);
            self->selectedName.clear();
            self->updateDeleteVisibility();
        }
    }), this);

    gtk_box_pack_start(GTK_BOX(toolbar), addBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), folderBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), deleteButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

    // Add form (hidden by default)
    addForm = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(addForm, 8);
    gtk_widget_set_margin_end(addForm, 8);
    gtk_widget_set_margin_bottom(addForm, 6);
    gtk_widget_set_name(addForm, "favorites-add-form");
    gtk_widget_set_no_show_all(addForm, TRUE);

    nameEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(nameEntry), "Nom du favori");
    gtk_widget_set_hexpand(nameEntry, TRUE);

    urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "URL");
    gtk_widget_set_hexpand(urlEntry, TRUE);

    GtkWidget* okBtn = gtk_button_new_from_icon_name("dialog-ok", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(okBtn, "Ajouter le favori (Entrée)");
    g_signal_connect(okBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->commitAddForm();
    }), this);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Annuler");
    gtk_widget_set_tooltip_text(cancelBtn, "Annuler (Échap)");
    g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->cancelAddForm();
    }), this);

    auto activateCommit = +[](GtkEntry*, gpointer u) {
        static_cast<FavoritesManager*>(u)->commitAddForm();
    };
    g_signal_connect(nameEntry, "activate", G_CALLBACK(activateCommit), this);
    g_signal_connect(urlEntry, "activate", G_CALLBACK(activateCommit), this);

    g_signal_connect(addForm, "key-press-event", G_CALLBACK(+[](GtkWidget*, GdkEventKey* e, gpointer u) -> gboolean {
        if (e->keyval == GDK_KEY_Escape) {
            static_cast<FavoritesManager*>(u)->cancelAddForm();
            return TRUE;
        }
        return FALSE;
    }), this);

    gtk_box_pack_start(GTK_BOX(addForm), nameEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(addForm), urlEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(addForm), okBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(addForm), cancelBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), addForm, FALSE, FALSE, 0);
    gtk_widget_hide(addForm);

    // List
    scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);

    favoritesList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(favoritesList), GTK_SELECTION_SINGLE);
    gtk_widget_set_name(favoritesList, "favorites-list");
    g_signal_connect(favoritesList, "row-selected", G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        if (!row) {
            self->selectedName.clear();
        } else {
            const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "item-name"));
            self->selectedName = name ? name : "";
        }
        self->updateDeleteVisibility();
    }), this);
    g_signal_connect(favoritesList, "row-activated", G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        if (!row) return;
        const char* type = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "item-type"));
        const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "item-name"));
        if (type && name && std::string(type) == "folder") {
            self->openFolder(name);
        }
    }), this);

    gtk_container_add(GTK_CONTAINER(scrolled), favoritesList);
    gtk_box_pack_start(GTK_BOX(root), scrolled, TRUE, TRUE, 0);

    GtkWidget* closeBtn = gtk_button_new_with_label("Fermer");
    gtk_widget_set_margin_start(closeBtn, 8);
    gtk_widget_set_margin_end(closeBtn, 8);
    gtk_widget_set_margin_top(closeBtn, 4);
    gtk_widget_set_margin_bottom(closeBtn, 8);
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer u) {
        static_cast<FavoritesManager*>(u)->closeWindow();
    }), this);
    gtk_box_pack_start(GTK_BOX(root), closeBtn, FALSE, FALSE, 0);

    rebuildList();
}

GtkWidget* FavoritesManager::buildRow(const nlohmann::json& item) {
    const bool folder = FavoritesJson::isFolder(item);
    const std::string name = item.value("name", folder ? "Dossier" : "Favori");
    const std::string url = folder ? "" : item.value("url", "");

    GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(rowBox, 6);
    gtk_widget_set_margin_end(rowBox, 6);
    gtk_widget_set_margin_top(rowBox, 4);
    gtk_widget_set_margin_bottom(rowBox, 4);

    GtkWidget* icon = gtk_image_new_from_icon_name(folder ? "folder" : "text-html", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(rowBox), icon, FALSE, FALSE, 0);

    GtkWidget* texts = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* nameLbl = gtk_label_new((folder ? ("📁 " + name) : name).c_str());
    gtk_widget_set_halign(nameLbl, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(nameLbl), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(texts), nameLbl, FALSE, FALSE, 0);
    if (!folder) {
        GtkWidget* urlLbl = gtk_label_new(url.c_str());
        gtk_widget_set_halign(urlLbl, GTK_ALIGN_START);
        gtk_widget_set_opacity(urlLbl, 0.65);
        gtk_label_set_ellipsize(GTK_LABEL(urlLbl), PANGO_ELLIPSIZE_END);
        gtk_box_pack_start(GTK_BOX(texts), urlLbl, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(rowBox), texts, TRUE, TRUE, 0);

    GtkWidget* editBtn = gtk_button_new_from_icon_name("document-edit", GTK_ICON_SIZE_MENU);
    gtk_widget_set_tooltip_text(editBtn, "Modifier");
    gtk_button_set_relief(GTK_BUTTON(editBtn), GTK_RELIEF_NONE);
    g_object_set_data_full(G_OBJECT(editBtn), "item-name", g_strdup(name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(editBtn), "item-url", g_strdup(url.c_str()), g_free);
    g_object_set_data(G_OBJECT(editBtn), "is-folder", GINT_TO_POINTER(folder ? 1 : 0));
    g_signal_connect(editBtn, "clicked", G_CALLBACK(on_row_edit_clicked), this);

    GtkWidget* delBtn = gtk_button_new_from_icon_name("edit-delete", GTK_ICON_SIZE_MENU);
    gtk_widget_set_tooltip_text(delBtn, "Supprimer");
    gtk_button_set_relief(GTK_BUTTON(delBtn), GTK_RELIEF_NONE);
    g_object_set_data_full(G_OBJECT(delBtn), "item-name", g_strdup(name.c_str()), g_free);
    g_signal_connect(delBtn, "clicked", G_CALLBACK(on_row_delete_clicked), this);

    GtkWidget* moreBtn = gtk_button_new_from_icon_name("view-more-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_set_tooltip_text(moreBtn, "Plus d'options");
    gtk_button_set_relief(GTK_BUTTON(moreBtn), GTK_RELIEF_NONE);
    g_object_set_data_full(G_OBJECT(moreBtn), "item-name", g_strdup(name.c_str()), g_free);
    g_object_set_data(G_OBJECT(moreBtn), "is-folder", GINT_TO_POINTER(folder ? 1 : 0));
    g_object_set_data_full(G_OBJECT(moreBtn), "item-url", g_strdup(url.c_str()), g_free);
    g_signal_connect(moreBtn, "clicked", G_CALLBACK(on_row_more_clicked), this);

    gtk_box_pack_end(GTK_BOX(rowBox), moreBtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rowBox), delBtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rowBox), editBtn, FALSE, FALSE, 0);

    GtkWidget* row = gtk_list_box_row_new();
    gtk_container_add(GTK_CONTAINER(row), rowBox);
    g_object_set_data_full(G_OBJECT(row), "item-name", g_strdup(name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(row), "item-type", g_strdup(folder ? "folder" : "bookmark"), g_free);
    g_object_set_data_full(G_OBJECT(row), "item-url", g_strdup(url.c_str()), g_free);
    return row;
}

void FavoritesManager::rebuildList() {
    if (!favoritesList || !GTK_IS_LIST_BOX(favoritesList)) {
        return;
    }
    GList* children = gtk_container_get_children(GTK_CONTAINER(favoritesList));
    for (GList* it = children; it; it = it->next) {
        gtk_widget_destroy(GTK_WIDGET(it->data));
    }
    g_list_free(children);

    nlohmann::json& arr = currentArray();
    if (arr.empty()) {
        GtkWidget* empty = gtk_label_new("Ce dossier est vide");
        gtk_widget_set_margin_top(empty, 24);
        gtk_widget_set_opacity(empty, 0.6);
        GtkWidget* row = gtk_list_box_row_new();
        gtk_widget_set_sensitive(row, FALSE);
        gtk_container_add(GTK_CONTAINER(row), empty);
        gtk_list_box_insert(GTK_LIST_BOX(favoritesList), row, -1);
    } else {
        for (const auto& item : arr) {
            if (!item.is_object() || !item.contains("name")) {
                continue;
            }
            gtk_list_box_insert(GTK_LIST_BOX(favoritesList), buildRow(item), -1);
        }
    }
    gtk_widget_show_all(favoritesList);
    updateNavButtons();
    updatePathLabel();
    updateDeleteVisibility();
}

void FavoritesManager::updateNavButtons() {
    if (backButton) {
        gtk_widget_set_sensitive(backButton, !backStack.empty());
    }
    if (forwardButton) {
        gtk_widget_set_sensitive(forwardButton, !forwardStack.empty());
    }
    if (upButton) {
        gtk_widget_set_sensitive(upButton, !currentPath.empty());
    }
}

void FavoritesManager::updatePathLabel() {
    if (!pathLabel) {
        return;
    }
    std::string path = "Favoris";
    for (const auto& p : currentPath) {
        path += " › " + p;
    }
    gtk_label_set_text(GTK_LABEL(pathLabel), path.c_str());
}

void FavoritesManager::updateDeleteVisibility() {
    if (!deleteButton) {
        return;
    }
    if (selectedName.empty()) {
        gtk_widget_hide(deleteButton);
    } else {
        gtk_widget_show(deleteButton);
    }
}

void FavoritesManager::showAddForm(bool show) {
    if (!addForm) {
        return;
    }
    if (!show) {
        gtk_widget_hide(addForm);
        return;
    }
    std::string nom = pageTitle.empty() ? "Nouveau favori" : pageTitle;
    if (nom.size() > 60) {
        nom = nom.substr(0, 57) + "...";
    }
    gtk_entry_set_text(GTK_ENTRY(nameEntry), nom.c_str());
    gtk_entry_set_text(GTK_ENTRY(urlEntry), pageUrl.c_str());
    gtk_widget_show_all(addForm);
    gtk_widget_grab_focus(nameEntry);
    gtk_editable_select_region(GTK_EDITABLE(nameEntry), 0, -1);
}

void FavoritesManager::commitAddForm() {
    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(nameEntry));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(urlEntry));
    if (!nom || !*nom || !url || !*url) {
        return;
    }
    addFavorite(nom, url, "Général");
    showAddForm(false);
}

void FavoritesManager::cancelAddForm() {
    showAddForm(false);
}

void FavoritesManager::pushHistoryBeforeNavigate() {
    backStack.push_back(currentPath);
    forwardStack.clear();
}

void FavoritesManager::openFolder(const std::string& folderName) {
    pushHistoryBeforeNavigate();
    currentPath.push_back(folderName);
    selectedName.clear();
    rebuildList();
}

void FavoritesManager::navigateBack() {
    if (backStack.empty()) {
        return;
    }
    forwardStack.push_back(currentPath);
    currentPath = backStack.back();
    backStack.pop_back();
    selectedName.clear();
    rebuildList();
}

void FavoritesManager::navigateForward() {
    if (forwardStack.empty()) {
        return;
    }
    backStack.push_back(currentPath);
    currentPath = forwardStack.back();
    forwardStack.pop_back();
    selectedName.clear();
    rebuildList();
}

void FavoritesManager::navigateUp() {
    if (currentPath.empty()) {
        return;
    }
    pushHistoryBeforeNavigate();
    currentPath.pop_back();
    selectedName.clear();
    rebuildList();
}

void FavoritesManager::displayFavoritesList() {
    rebuildList();
}

void FavoritesManager::addFolder(const std::string& nom) {
    if (nom.empty()) {
        return;
    }
    nlohmann::json& arr = currentArray();
    for (const auto& it : arr) {
        if (it.contains("name") && it["name"] == nom) {
            std::cerr << "Un élément porte déjà ce nom : " << nom << std::endl;
            return;
        }
    }
    arr.push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    saveModifications();
    callbackRafraichir();
    rebuildList();
}

void FavoritesManager::addFavorite(const std::string& nom, const std::string& url, const std::string& tag) {
    if (FavoritesJson::containsUrlRecursive(*favorites, url)) {
        return;
    }
    currentArray().push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    saveModifications();
    callbackRafraichir();
    rebuildList();
}

void FavoritesManager::addFavoriteToFolder(const std::string& folderName, const std::string& nom,
                                          const std::string& url, const std::string& tag) {
    nlohmann::json* folderArr = nullptr;
    if (!findFolderArray(*favorites, folderName, &folderArr) || !folderArr) {
        std::cerr << "Dossier introuvable : " << folderName << std::endl;
        return;
    }
    folderArr->push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    saveModifications();
    callbackRafraichir();
    rebuildList();
}

void FavoritesManager::moveItemToFolder(const std::string& itemName, const std::string& targetFolderName) {
    nlohmann::json extracted;
    if (!findAndErase(*favorites, itemName, &extracted)) {
        return;
    }
    nlohmann::json* folderArr = nullptr;
    if (targetFolderName.empty()) {
        favorites->push_back(extracted);
    } else if (findFolderArray(*favorites, targetFolderName, &folderArr) && folderArr) {
        folderArr->push_back(extracted);
    } else {
        favorites->push_back(extracted);
    }
    saveModifications();
    callbackRafraichir();
    rebuildList();
}

void FavoritesManager::removeFavorite(const std::string& favoriteName) {
    if (FavoritesJson::removeByNameRecursive(*favorites, favoriteName)) {
        saveModifications();
        callbackRafraichir();
        selectedName.clear();
        rebuildList();
    }
}

bool FavoritesManager::removeFavoriteFromList(const std::string& favoriteName) {
    return FavoritesJson::removeByNameRecursive(*favorites, favoriteName);
}

void FavoritesManager::modifyFavorite(const std::string& favoriteName, const std::string& newUrl) {
    modifyFavoriteFull(favoriteName, favoriteName, newUrl);
}

void FavoritesManager::modifyFavoriteFull(const std::string& oldName, const std::string& newName, const std::string& newUrl) {
    if (renameItem(*favorites, oldName, newName, newUrl, true)) {
        saveModifications();
        callbackRafraichir();
        rebuildList();
    }
}

void FavoritesManager::promptRename(const std::string& name, const std::string& url, bool isFolder) {
    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        isFolder ? "Renommer le dossier" : "Modifier le favori",
        GTK_WINDOW(window),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "Annuler", GTK_RESPONSE_CANCEL, "Enregistrer", GTK_RESPONSE_ACCEPT, nullptr);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    GtkWidget* eName = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(eName), name.c_str());
    gtk_box_pack_start(GTK_BOX(box), eName, FALSE, FALSE, 0);
    GtkWidget* eUrl = nullptr;
    if (!isFolder) {
        eUrl = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(eUrl), url.c_str());
        gtk_box_pack_start(GTK_BOX(box), eUrl, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), box);
    gtk_widget_show_all(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        const gchar* nn = gtk_entry_get_text(GTK_ENTRY(eName));
        if (nn && *nn) {
            if (isFolder) {
                renameItem(*favorites, name, nn, "", false);
                saveModifications();
                callbackRafraichir();
                rebuildList();
            } else {
                const gchar* nu = eUrl ? gtk_entry_get_text(GTK_ENTRY(eUrl)) : url.c_str();
                modifyFavoriteFull(name, nn, nu ? nu : url);
            }
        }
    }
    gtk_widget_destroy(dlg);
}

void FavoritesManager::promptMove(const std::string& itemName) {
    auto folders = collectFolderNames(*favorites, itemName);
    GtkWidget* menu = gtk_menu_new();

    GtkWidget* rootItem = gtk_menu_item_new_with_label("Racine (Favoris)");
    g_object_set_data_full(G_OBJECT(rootItem), "target", g_strdup(""), g_free);
    g_object_set_data_full(G_OBJECT(rootItem), "item", g_strdup(itemName.c_str()), g_free);
    g_signal_connect(rootItem, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        const char* item = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "item"));
        self->moveItemToFolder(item ? item : "", "");
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), rootItem);

    for (const auto& f : folders) {
        GtkWidget* mi = gtk_menu_item_new_with_label(("📁 " + f).c_str());
        g_object_set_data_full(G_OBJECT(mi), "target", g_strdup(f.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(mi), "item", g_strdup(itemName.c_str()), g_free);
        g_signal_connect(mi, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
            auto* self = static_cast<FavoritesManager*>(u);
            const char* item = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "item"));
            const char* target = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "target"));
            self->moveItemToFolder(item ? item : "", target ? target : "");
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
}

void FavoritesManager::showItemMenu(GtkWidget* anchor, const nlohmann::json& item) {
    const bool folder = FavoritesJson::isFolder(item);
    const std::string name = item.value("name", "");
    const std::string url = item.value("url", "");

    GtkWidget* menu = gtk_menu_new();

    if (folder) {
        GtkWidget* open = gtk_menu_item_new_with_label("Ouvrir le dossier");
        g_object_set_data_full(G_OBJECT(open), "name", g_strdup(name.c_str()), g_free);
        g_signal_connect(open, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
            const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "name"));
            static_cast<FavoritesManager*>(u)->openFolder(n ? n : "");
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), open);
    }

    GtkWidget* edit = gtk_menu_item_new_with_label("Modifier…");
    g_object_set_data_full(G_OBJECT(edit), "name", g_strdup(name.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(edit), "url", g_strdup(url.c_str()), g_free);
    g_object_set_data(G_OBJECT(edit), "folder", GINT_TO_POINTER(folder ? 1 : 0));
    g_signal_connect(edit, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
        auto* self = static_cast<FavoritesManager*>(u);
        const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "name"));
        const char* ur = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "url"));
        const bool isFolder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "folder")) != 0;
        self->promptRename(n ? n : "", ur ? ur : "", isFolder);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), edit);

    GtkWidget* move = gtk_menu_item_new_with_label("Déplacer vers…");
    g_object_set_data_full(G_OBJECT(move), "name", g_strdup(name.c_str()), g_free);
    g_signal_connect(move, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
        const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "name"));
        static_cast<FavoritesManager*>(u)->promptMove(n ? n : "");
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), move);

    GtkWidget* del = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data_full(G_OBJECT(del), "name", g_strdup(name.c_str()), g_free);
    g_signal_connect(del, "activate", G_CALLBACK(+[](GtkWidget* w, gpointer u) {
        const char* n = static_cast<const char*>(g_object_get_data(G_OBJECT(w), "name"));
        static_cast<FavoritesManager*>(u)->removeFavorite(n ? n : "");
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), del);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), anchor, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}

void FavoritesManager::createFavoriteContextMenu(GtkWidget* button, const std::string& favoriteName) {
    nlohmann::json fake = {{"name", favoriteName}, {"url", ""}};
    showItemMenu(button, fake);
}

void FavoritesManager::saveModifications() {
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
}

void FavoritesManager::refreshInterface() {
    rebuildList();
}

GtkWidget* FavoritesManager::getListeFavoris() {
    return favoritesList;
}

void FavoritesManager::showWindow() {
    if (!window || !GTK_IS_WIDGET(window)) {
        createInterface();
    }
    rebuildList();
    gtk_widget_show_all(window);
    if (addForm) {
        gtk_widget_hide(addForm);
    }
    if (deleteButton && selectedName.empty()) {
        gtk_widget_hide(deleteButton);
    }
    gtk_window_present(GTK_WINDOW(window));
}

void FavoritesManager::selectRowByName(const std::string& name) {
    selectedName = name;
    updateDeleteVisibility();
}
