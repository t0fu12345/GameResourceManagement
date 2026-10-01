#pragma once
#include "../IAsset.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

namespace GameEngine {

    class Mesh3D : public IAsset {
    private:
        std::string m_path;
        bool m_isLoaded = false;
        std::size_t m_sizeBytes = 0;
        
        // Giả lập dữ liệu hình học (Geometry Data)
        std::vector<float> m_vertices;
        std::vector<uint32_t> m_indices;

    public:
        Mesh3D(const std::string& path) : m_path(path) {}

        ~Mesh3D() override {
            if (m_isLoaded) {
                Unload();
            }
        }

        void Load(const std::string& path) override {
            if (m_isLoaded) return;
            
            std::cout << "[Mesh3D] Đang nạp mô hình 3D: " << path << "...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            // Giả lập Mesh nặng: 1 triệu đỉnh (~12 MB) và 3 triệu indices (~12 MB)
            m_sizeBytes = 1000000 * 3 * sizeof(float);
            m_vertices.resize(1000000 * 3);
            m_indices.resize(3000000); 
            m_sizeBytes += 3000000 * sizeof(uint32_t);
            
            m_isLoaded = true;
            std::cout << "[Mesh3D] Nạp thành công! Tiêu tốn " << (m_sizeBytes / (1024 * 1024)) << " MB RAM.\n";
        }

        void Unload() override {
            if (!m_isLoaded) return;
            std::cout << "[Mesh3D] Đang giải phóng bộ nhớ cho: " << m_path << "...\n";
            m_vertices.clear(); m_vertices.shrink_to_fit();
            m_indices.clear();  m_indices.shrink_to_fit();
            m_sizeBytes = 0;
            m_isLoaded = false;
        }

        bool IsLoaded() const override { return m_isLoaded; }
        std::size_t GetMemoryFootprint() const override { return m_sizeBytes; }
        const std::string& GetPath() const override { return m_path; }
    };

} // namespace GameEngine
