#ifndef STORE_H
#define STORE_H

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>
#include <shared_mutex> // Included for read-write locks

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

struct CacheItem {
    std::string value;
    bool has_expiry;
    TimePoint expiry_time;
};

class Store {
private:
    std::unordered_map<std::string, CacheItem> data;
    
    // Our read-write lock to protect the data map
    std::shared_mutex rw_lock; 

    // Helper function (internal logic only, caller must handle locking)
    bool is_expired(const std::string& key);

public:
    Store() = default;
    ~Store() = default;

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    bool expire(const std::string& key, int seconds);
    int ttl(const std::string& key);
    std::vector<std::string> keys();

    void save_to_disk(const std::string& filename);
    void load_from_disk(const std::string& filename);
};

#endif // STORE_H
