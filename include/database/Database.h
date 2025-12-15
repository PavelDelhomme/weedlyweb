#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include <memory>

struct Favorite {
    int id;
    std::string title;
    std::string url;
    std::string icon_path;
    int parent_id;
    std::string created_at;
};

struct Folder {
    int id;
    std::string name;
    int parent_id;
};

class Database {
public:
    Database();
    ~Database();
    
    bool initDatabase();
    bool migrateFromJson();
    
    // Opérations sur les favorites
    bool addFavorite(const std::string& title, const std::string& url, 
                     const std::string& iconPath = "", int parentId = 0);
    bool deleteFavorite(int id);
    bool updateFavorite(int id, const std::string& title, 
                       const std::string& url, int parentId);
    std::vector<Favorite> getFavorites(int parentId = 0);
    Favorite getFavoriteByUrl(const std::string& url);
    bool updateFavicon(int id, const std::string& faviconPath);
    
    // Opérations sur les dossiers
    bool addFolder(const std::string& name, int parentId = 0);
    bool deleteFolder(int id);
    std::vector<Folder> getFolders(int parentId = 0);
    
    // Historique
    bool addHistory(const std::string& url, const std::string& title);
    std::vector<std::pair<std::string, std::string>> getHistory(int limit = 100);
    bool clearHistory();

private:
    sqlite3* m_db;
    std::string m_dbPath;
    
    bool executeQuery(const std::string& query);
    bool prepareStatement(sqlite3_stmt** stmt, const std::string& sql);
    std::string getDatabasePath();
};

#endif // DATABASE_H

