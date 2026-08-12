#include "core/FavoritesJson.h"
#include <cctype>
#include <utility>
#include <vector>

namespace FavoritesJson {

bool isFolder(const nlohmann::json& item) {
    return item.is_object() && item.contains("type") && item["type"] == "folder" &&
           item.contains("children") && item["children"].is_array();
}

std::string canonicalizeUrl(const std::string& url) {
    std::string u = url;
    for (char& c : u) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const auto hash = u.find('#');
    if (hash != std::string::npos) {
        u.erase(hash);
    }
    while (u.size() > 8 && u.back() == '/') {
        u.pop_back();
    }
    auto stripWww = [](std::string s) {
        const std::string markers[] = {"https://www.", "http://www.", "https://", "http://"};
        for (const auto& m : markers) {
            if (s.rfind(m, 0) == 0) {
                s = s.substr(m.size());
                break;
            }
        }
        return s;
    };
    return stripWww(u);
}

bool urlsMatch(const std::string& a, const std::string& b) {
    if (a == b) {
        return true;
    }
    return canonicalizeUrl(a) == canonicalizeUrl(b);
}

bool containsUrlRecursive(const nlohmann::json& rootArray, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (const auto& item : rootArray) {
        if (isFolder(item)) {
            if (containsUrlRecursive(item["children"], url)) {
                return true;
            }
        } else if (item.contains("url") && item["url"].is_string() &&
                   urlsMatch(item["url"].get<std::string>(), url)) {
            return true;
        }
    }
    return false;
}

bool removeByNameRecursive(nlohmann::json& rootArray, const std::string& name) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (auto it = rootArray.begin(); it != rootArray.end(); ++it) {
        if (!it->is_object() || !(*it).contains("name")) {
            continue;
        }
        if ((*it)["name"].get<std::string>() == name) {
            rootArray.erase(it);
            return true;
        }
        if (isFolder(*it)) {
            if (removeByNameRecursive((*it)["children"], name)) {
                return true;
            }
        }
    }
    return false;
}

bool removeByUrlRecursive(nlohmann::json& rootArray, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (auto it = rootArray.begin(); it != rootArray.end(); ++it) {
        if (!it->is_object()) {
            continue;
        }
        if (isFolder(*it)) {
            if (removeByUrlRecursive((*it)["children"], url)) {
                return true;
            }
        } else if ((*it).contains("url") && (*it)["url"].is_string() &&
                   urlsMatch((*it)["url"].get<std::string>(), url)) {
            rootArray.erase(it);
            return true;
        }
    }
    return false;
}

bool duplicateNameOrUrl(const nlohmann::json& rootArray, const std::string& nom, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (const auto& item : rootArray) {
        if (isFolder(item)) {
            if (item.contains("name") && item["name"].is_string() && item["name"].get<std::string>() == nom) {
                return true;
            }
            if (duplicateNameOrUrl(item["children"], nom, url)) {
                return true;
            }
        } else {
            if (item.contains("name") && item["name"].is_string() && item["name"].get<std::string>() == nom) {
                return true;
            }
            if (item.contains("url") && item["url"].is_string() &&
                urlsMatch(item["url"].get<std::string>(), url)) {
                return true;
            }
        }
    }
    return false;
}

void collectBookmarkEntries(const nlohmann::json& rootArray,
                            std::vector<std::pair<std::string, std::string>>& out,
                            int maxItems) {
    if (!rootArray.is_array() || maxItems <= 0) {
        return;
    }
    for (const auto& item : rootArray) {
        if (static_cast<int>(out.size()) >= maxItems) {
            return;
        }
        if (!item.is_object()) {
            continue;
        }
        if (isFolder(item)) {
            collectBookmarkEntries(item["children"], out, maxItems);
        } else if (item.contains("url") && item["url"].is_string()) {
            std::string name = item.contains("name") && item["name"].is_string()
                                   ? item["name"].get<std::string>()
                                   : item["url"].get<std::string>();
            out.emplace_back(std::move(name), item["url"].get<std::string>());
        }
    }
}

} // namespace FavoritesJson
