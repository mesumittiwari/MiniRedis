#include "store.h"
#include <chrono>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>

void Store::save_to_disk(const std::string& filename) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    std::ofstream outfile(filename);
    
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file for saving snapshot.\n";
        return;
    }

    for (const auto& pair : data) {
        // Skip expired keys so we don't save dead data
        if (pair.second.has_expiry) {
            auto now = std::chrono::system_clock::now();
            if (now >= pair.second.expiry_time) continue;
        }

        // Format: KEY VALUE HAS_EXPIRY EXPIRY_EPOCH_SECONDS
        outfile << pair.first << " " << pair.second.value << " " << pair.second.has_expiry;
        
        if (pair.second.has_expiry) {
            auto duration = pair.second.expiry_time.time_since_epoch();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            outfile << " " << seconds;
        }
        outfile << "\n";
    }
    
    outfile.flush(); // Flushes OS write buffer before closing
    outfile.close();
    std::cout << "Database successfully saved to " << filename << "\n";
}

void Store::load_from_disk(const std::string& filename) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    std::ifstream infile(filename);
    
    if (!infile.is_open()) {
        std::cout << "No existing snapshot found. Starting with empty store.\n";
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        bool has_expiry;
        
        if (!(iss >> key >> value >> has_expiry)) continue;

        CacheItem item;
        item.value = value;
        item.has_expiry = has_expiry;

        if (has_expiry) {
            long long epoch_seconds;
            if (iss >> epoch_seconds) {
                auto duration = std::chrono::seconds(epoch_seconds);
                item.expiry_time = TimePoint(duration);
                
                // If it already expired while the server was offline, skip loading it
                if (std::chrono::system_clock::now() >= item.expiry_time) {
                    continue;
                }
            }
        }

        data[key] = item;
    }
    
    infile.close();
    std::cout << "Database successfully loaded from " << filename << "\n";
}

// Private helper: assumes caller already holds rw_lock!
bool Store::is_expired(const std::string& key) {
    auto it = data.find(key);
    if (it == data.end()) return false;

    if (it->second.has_expiry) {
        auto now = std::chrono::system_clock::now();
        if (now >= it->second.expiry_time) {
            data.erase(it); // Lazy deletion happens right here
            return true;
        }
    }
    return false;
}

void Store::set(const std::string& key, const std::string& value) {
    // By default, new keys have no expiry, needs EXCLUSIVE lock
    // LOCK: one write at a time
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    data[key] = {value, false, TimePoint()};
}

std::string Store::get(const std::string& key) {
    // If it is expired, is_expired() deletes it and we return (nil)
    // Needs a write lock because is_expired() might delete a key
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (is_expired(key)) return "(nil)"; 
    
    auto it = data.find(key);
    if (it != data.end()) {
        return it->second.value;
    }
    return "(nil)";
}

bool Store::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    return data.erase(key) > 0;
}

bool Store::exists(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (is_expired(key)) return false;
    return data.find(key) != data.end();
}

bool Store::expire(const std::string& key, int seconds) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (is_expired(key)) return false;
    
    auto it = data.find(key);
    if (it != data.end()) {
        it->second.has_expiry = true;
        it->second.expiry_time = std::chrono::system_clock::now() + std::chrono::seconds(seconds);
        return true;
    }
    return false;
}

int Store::ttl(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (is_expired(key)) return -2; // Redis standard: -2 means key does not exist
    
    auto it = data.find(key);
    if (it != data.end()) {
        if (!it->second.has_expiry) return -1; // Redis standard: -1 means no expiry set
        
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(it->second.expiry_time - now);
        return duration.count();
    }
    return -2;
}

std::vector<std::string> Store::keys() {
    // Even keys() might delete expired items as it iterates, so it needs a write lock
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    std::vector<std::string> result;
    
    // We must use a while loop (or carefully structured for loop) to safely erase while iterating
    for (auto it = data.begin(); it != data.end(); ) {
        if (it->second.has_expiry && std::chrono::system_clock::now() >= it->second.expiry_time) {
            // C++11 standard: erase() returns an iterator to the NEXT element
            it = data.erase(it); 
        } else {
            result.push_back(it->first);
            ++it;
        }
    }
    return result;
}
