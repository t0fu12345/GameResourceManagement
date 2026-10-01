#pragma once
#include "../IAsset.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

namespace GameEngine {

    class Texture2D : public IAsset {
    private:
        std::string m_path;
        bool m_isLoaded = false;
        std::size_t m_sizeBytes = 0;
        
        // Giả lập vùng nhớ chứa điểm ảnh (pixels)
        std::vector<uint8_t> m_pixelData;
        int m_width = 0;
        int m_height = 0;

    public:
        Texture2D(const std::string& path) : m_path(path) {}

        ~Texture2D() override {
            if (m_isLoaded) {
                Unload();
            }
        }

        void Load(const std::string& path) override {
            if (m_isLoaded) return;
            
            std::cout << "[Texture2D] Đang đọc file từ ổ đĩa: " << path << "...\n";
            // Giả lập độ trễ I/O
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
            
            // Giả lập dữ liệu Texture 4K (4096 x 4096 x 4 bytes = 64 MB)
            m_width = 4096;
            m_height = 4096;
            m_sizeBytes = m_width * m_height * 4;
            m_pixelData.resize(m_sizeBytes); // Cấp phát RAM khổng lồ
            
            m_isLoaded = true;
            std::cout << "[Texture2D] Nạp thành công! Tiêu tốn " << (m_sizeBytes / (1024 * 1024)) << " MB RAM.\n";
        }

        void Unload() override {
            if (!m_isLoaded) return;
            
            std::cout << "[Texture2D] Đang giải phóng bộ nhớ cho: " << m_path << "...\n";
            m_pixelData.clear();
            
            // TRICK C++: vector.clear() không thực sự trả lại RAM cho HĐH. 
            // Phải gọi shrink_to_fit() để dọn dẹp triệt để Capacity.
            m_pixelData.shrink_to_fit(); 
            
            m_sizeBytes = 0;
            m_isLoaded = false;
        }

        bool IsLoaded() const override { return m_isLoaded; }
        std::size_t GetMemoryFootprint() const override { return m_sizeBytes; }
        const std::string& GetPath() const override { return m_path; }
    };

} // namespace GameEngine
