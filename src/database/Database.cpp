#include "database/Database.h"
#include <iostream>
#include <filesystem>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "GestionnaireFichiers.h"

Database::Database() : m_db(nullptr) {
    m_dbPath = getDatabasePath();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

std::string Database::getDatabasePath() {
    std::string homeDir = std::getenv("HOME") ? std::getenv("HOME") : ".";
    std::string appDataDir = homeDir + "/.weedlyweb";
    
    // Créer le répertoire s'il n'existe pas
    std::filesystem::create_directories(appDataDir);
    
    return appDataDir + "/favorites.db";
}

bool Database::initDatabase() {
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::cerr << "Erreur lors de l'ouverture de la base de données: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    // Activer les clés étrangères
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    
    // Créer la table des dossiers
    const char* createFoldersTable = R"(
        CREATE TABLE IF NOT EXISTS folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            parent_id INTEGER DEFAULT 0,
            FOREIGN KEY (parent_id) REFERENCES folders(id)
        )
    )";
    
    rc = sqlite3_exec(m_db, createFoldersTable, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Erreur lors de la création de la table folders: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    // Créer la table des favoris
    const char* createFavoritesTable = R"(
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            url TEXT NOT NULL,
            icon_path TEXT,
            parent_id INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (parent_id) REFERENCES folders(id)
        )
    )";
    
    rc = sqlite3_exec(m_db, createFavoritesTable, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Erreur lors de la création de la table favorites: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    // Créer la table d'historique
    const char* createHistoryTable = R"(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL,
            title TEXT,
            visited_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    rc = sqlite3_exec(m_db, createHistoryTable, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Erreur lors de la création de la table history: " 
                  << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    // Créer les index
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_favorites_parent ON favorites(parent_id);", 
                 nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_favorites_url ON favorites(url);", 
                 nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_history_visited ON history(visited_at DESC);", 
                 nullptr, nullptr, nullptr);
    
    // Créer le dossier "Favoris" par défaut s'il n'existe pas
    sqlite3_stmt* stmt;
    const char* checkFolder = "SELECT id FROM folders WHERE name = 'Favoris'";
    rc = sqlite3_prepare_v2(m_db, checkFolder, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            // Le dossier n'existe pas, le créer
            sqlite3_finalize(stmt);
            const char* insertFolder = "INSERT INTO folders (name, parent_id) VALUES ('Favoris', 0)";
            sqlite3_exec(m_db, insertFolder, nullptr, nullptr, nullptr);
        } else {
            sqlite3_finalize(stmt);
        }
    }
    
    return true;
}

bool Database::addFavorite(const std::string& title, const std::string& url, 
                         const std::string& iconPath, int parentId) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO favorites (title, url, icon_path, parent_id) VALUES (?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Erreur lors de la préparation: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, iconPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, parentId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::deleteFavorite(int id) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM favorites WHERE id = ?";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::updateFavorite(int id, const std::string& title, 
                              const std::string& url, int parentId) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE favorites SET title = ?, url = ?, parent_id = ? WHERE id = ?";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, parentId);
    sqlite3_bind_int(stmt, 4, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Favorite> Database::getFavorites(int parentId) {
    std::vector<Favorite> favorites;
    sqlite3_stmt* stmt;
    const char* sql;
    
    if (parentId == 0) {
        sql = "SELECT id, title, url, icon_path, parent_id, created_at FROM favorites";
    } else {
        sql = "SELECT id, title, url, icon_path, parent_id, created_at FROM favorites WHERE parent_id = ?";
    }
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return favorites;
    }
    
    if (parentId != 0) {
        sqlite3_bind_int(stmt, 1, parentId);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Favorite fav;
        fav.id = sqlite3_column_int(stmt, 0);
        fav.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        fav.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        fav.icon_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        fav.parent_id = sqlite3_column_int(stmt, 4);
        fav.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        favorites.push_back(fav);
    }
    
    sqlite3_finalize(stmt);
    return favorites;
}

Favorite Database::getFavoriteByUrl(const std::string& url) {
    Favorite fav = {0, "", "", "", 0, ""};
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, title, url, icon_path, parent_id, created_at FROM favorites WHERE url = ?";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return fav;
    }
    
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        fav.id = sqlite3_column_int(stmt, 0);
        fav.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        fav.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        fav.icon_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        fav.parent_id = sqlite3_column_int(stmt, 4);
        fav.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }
    
    sqlite3_finalize(stmt);
    return fav;
}

bool Database::addFolder(const std::string& name, int parentId) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO folders (name, parent_id) VALUES (?, ?)";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, parentId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Folder> Database::getFolders(int parentId) {
    std::vector<Folder> folders;
    sqlite3_stmt* stmt;
    const char* sql;
    
    if (parentId == 0) {
        sql = "SELECT id, name, parent_id FROM folders";
    } else {
        sql = "SELECT id, name, parent_id FROM folders WHERE parent_id = ?";
    }
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return folders;
    }
    
    if (parentId != 0) {
        sqlite3_bind_int(stmt, 1, parentId);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Folder folder;
        folder.id = sqlite3_column_int(stmt, 0);
        folder.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        folder.parent_id = sqlite3_column_int(stmt, 2);
        folders.push_back(folder);
    }
    
    sqlite3_finalize(stmt);
    return folders;
}

bool Database::addHistory(const std::string& url, const std::string& title) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO history (url, title) VALUES (?, ?)";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<std::pair<std::string, std::string>> Database::getHistory(int limit) {
    std::vector<std::pair<std::string, std::string>> history;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT url, title FROM history ORDER BY visited_at DESC LIMIT ?";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return history;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        history.push_back({url, title});
    }
    
    sqlite3_finalize(stmt);
    return history;
}

bool Database::migrateFromJson() {
    // TODO: Implémenter la migration depuis favoris.json
    return true;
}

bool Database::updateFavicon(int id, const std::string& faviconPath) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE favorites SET icon_path = ? WHERE id = ?";
    
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, faviconPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::clearHistory() {
    const char* sql = "DELETE FROM history";
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
    return rc == SQLITE_OK;
}

