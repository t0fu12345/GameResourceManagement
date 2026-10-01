#pragma once
#include "../IAsset.hpp"
#include <memory>
#include <future>
#include <mutex>
#include <iostream>

namespace GameEngine {

    /**
     * @brief ProxyAsset đóng vai trò đại diện cho một tài nguyên nặng.
     * Cung cấp cơ chế Lazy Loading (nạp muộn) và Async Loading (nạp bất đồng bộ).
     */
    template <typename T>
    class ProxyAsset : public IAsset {
    private:
        std::string m_path;
        std::shared_ptr<T> m_realAsset;
        
        // Quản lý luồng tải nền
        std::future<void> m_asyncLoadTask;
        std::mutex m_mutex;
        bool m_isLoadingAsync = false;

    public:
        ProxyAsset(const std::string& path) : m_path(path), m_realAsset(nullptr) {}

        ~ProxyAsset() override {
            Unload();
        }

        // Tải đồng bộ (Blocking Main Thread)
        void Load(const std::string& path) override {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_realAsset) {
                m_realAsset = std::make_shared<T>(path);
                m_realAsset->Load(path);
            }
        }

        // Tải bất đồng bộ (Async Loading chạy ngầm)
        void LoadAsync() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_realAsset && !m_isLoadingAsync) {
                m_isLoadingAsync = true;
                std::cout << "[ProxyAsset] Khoi dong tien trinh nap chay nen (Async) cho: " << m_path << "\n";
                
                // std::async đẩy công việc I/O nặng nhọc sang một Worker Thread khác,
                // giải phóng Main Thread để giữ Game chạy mượt ở 60 FPS.
                m_asyncLoadTask = std::async(std::launch::async, [this]() {
                    auto tempAsset = std::make_shared<T>(m_path);
                    tempAsset->Load(m_path);
                    
                    std::lock_guard<std::mutex> innerLock(m_mutex);
                    m_realAsset = tempAsset;
                    m_isLoadingAsync = false;
                });
            }
        }

        void Unload() override {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Đảm bảo luồng ẩn đã hoàn thành xong trước khi hủy
            if (m_asyncLoadTask.valid()) {
                m_asyncLoadTask.wait();
            }
            if (m_realAsset) {
                m_realAsset->Unload();
                m_realAsset.reset();
            }
        }

        bool IsLoaded() const override {
            return m_realAsset != nullptr && m_realAsset->IsLoaded();
        }

        std::size_t GetMemoryFootprint() const override {
            if (m_realAsset) return m_realAsset->GetMemoryFootprint();
            return sizeof(*this); // Rất nhẹ (vài byte) khi chưa nạp RealAsset
        }

        const std::string& GetPath() const override { return m_path; }

        // ĐIỂM KÍCH HOẠT LAZY LOADING
        std::shared_ptr<T> GetRealAsset() {
            if (!m_realAsset) {
                std::cout << "[ProxyAsset] Lazy Loading kich hoat vi co yeu cau Render: " << m_path << "\n";
                Load(m_path); // Ép nạp ngay lập tức vì đang cần gấp
            }
            return m_realAsset;
        }
    };

} // namespace GameEngine
