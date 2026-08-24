#ifndef WEEDLYWEB_CORE_FAVORITES_JSON_H
#define WEEDLYWEB_CORE_FAVORITES_JSON_H

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace FavoritesJson {

bool isFolder(const nlohmann::json& item);
std::string canonicalizeUrl(const std::string& url);
bool urlsMatch(const std::string& a, const std::string& b);
bool containsUrlRecursive(const nlohmann::json& rootArray, const std::string& url);
bool removeByNameRecursive(nlohmann::json& rootArray, const std::string& name);
bool removeByUrlRecursive(nlohmann::json& rootArray, const std::string& url);
bool duplicateNameOrUrl(const nlohmann::json& rootArray, const std::string& nom, const std::string& url);

/** Retourne true si l'URL est trouvée ; remplit outName (et outUrl si présent). */
bool findByUrlRecursive(const nlohmann::json& rootArray, const std::string& url,
                        std::string& outName, std::string* outUrl = nullptr);

/** Collecte récursive name/url pour barres de favoris (GTK/Qt). */
void collectBookmarkEntries(const nlohmann::json& rootArray,
                            std::vector<std::pair<std::string, std::string>>& out,
                            int maxItems = 40);

} // namespace FavoritesJson

#endif
