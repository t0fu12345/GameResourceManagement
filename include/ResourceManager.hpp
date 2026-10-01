#pragma once
#include "IAsset.hpp"
#include "Assets/ProxyAsset.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <iostream>

namespace GameEngine {

    class ResourceManager {
    private:
        // ĐÃ NÂNG CẤP: Thay std::shared_ptr bằng std::weak_ptr để loại bỏ Memory Leak.
        // weak_ptr cho phép quan sát vùng nhớ mà không sở hữu nó.
        std::unordered_map<std::string, std::weak_ptr<IAsset>> m_assetCache;
        std::mutex m_mutex;

        ResourceManager() {
            std::cout << "[ResourceManager] Da khoi tao (Version 2 - Tich hop Garbage Collection & Proxy).\n";
        }

    public:
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        static ResourceManager& GetInstance() {
            static ResourceManager instance;
            return instance;
        }

        // Lấy tài nguyên dưới vỏ bọc Proxy. Chưa hề có I/O nào xảy ra ở bước này.
        template <typename T>
        std::shared_ptr<ProxyAsset<T>> GetAsset(const std::string& path) {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_assetCache.find(path);
            if (it != m_assetCache.end()) {
                // lock() biến weak_ptr thành shared_ptr để kiểm tra xem tài nguyên còn tồn tại trên RAM không
                if (auto sharedAsset = it->second.lock()) {
                    std::cout << "[Flyweight] Cache HIT: " << path << ". Tra ve ban sao Proxy.\n";
                    return std::dynamic_pointer_cast<ProxyAsset<T>>(sharedAsset);
                }
            }

            // Cache MISS hoặc Tài sản đã bị xóa (Dangling pointer) -> Khởi tạo Proxy mới
            std::cout << "[Flyweight] Cache MISS: " << path << ". Tao Proxy moi.\n";
            auto newProxy = std::make_shared<ProxyAsset<T>>(path);
            
            // Cất vào bộ đệm dưới dạng weak_ptr
            m_assetCache[path] = newProxy;

            return newProxy;
        }

        // Tự động thu gom rác (Garbage Collection)
        // Càn quét Hash Map để dọn dẹp các đường dẫn (Keys) mà dữ liệu của chúng đã bị hủy.
        void CollectGarbage() {
            std::lock_guard<std::mutex> lock(m_mutex);
            int count = 0;
            
            for (auto it = m_assetCache.begin(); it != m_assetCache.end(); ) {
                if (it->second.expired()) {
                    std::cout << "[GarbageCollection] Phat hien " << it->first << " da chet. Thu hoi Key.\n";
                    it = m_assetCache.erase(it); // Xóa an toàn khỏi map
                    count++;
                } else {
                    ++it;
                }
            }
            std::cout << "[GarbageCollection] Hoan tat don rac. So luong index da don: " << count << "\n";
        }
    };

} // namespace GameEngine
