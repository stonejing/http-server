#pragma once

#include <iostream>
#include <unordered_map>
#include <list>
#include <string>

// A simple LRU cache implementation using a hash map and a doubly linked list
class LRUCache {
private:
    size_t capacity;
    std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>> cache;
    std::list<std::string> lruList;

public:
    LRUCache(size_t capacity) : capacity(capacity) {}

    void put(const std::string& key, const std::string& value) {
        if (cache.find(key) != cache.end()) {
            // If the key already exists, update the value and move the node to the front
            cache[key].first = value;
            lruList.erase(cache[key].second);
        } else {
            // If the cache is full, remove the least recently used item
            if (cache.size() == capacity) {
                cache.erase(lruList.back());
                lruList.pop_back();
            }
        }

        lruList.push_front(key);
        cache[key] = {value, lruList.begin()};
    }

    std::string get(const std::string& key) {
        if (cache.find(key) != cache.end()) {
            // If the key exists, move the node to the front and return the value
            lruList.erase(cache[key].second);
            lruList.push_front(key);
            cache[key].second = lruList.begin();
            return cache[key].first;
        }
        return ""; // If key doesn't exist, return an empty string
    }
};