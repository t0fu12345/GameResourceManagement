#pragma once
#include "../IAsset.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

namespace GameEngine {

    class AudioClip : public IAsset {
    private:
        std::string m_path;
        bool m_isLoaded = false;
        std::size_t m_sizeBytes = 0;
        
        // Dữ liệu sóng âm thanh thô (PCM) để bơm vào Sound Card
        std::vector<uint8_t> m_pcmData; 

    public:
        AudioClip(const std::string& path) : m_path(path) {}
        
        ~AudioClip() override { 
            if (m_isLoaded) Unload(); 
        }

        void Load(const std::string& path) override {
            if (m_isLoaded) return;
            std::cout << "[AudioClip] Đang giải mã âm thanh: " << path << "...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Giả lập file Audio (khoảng 5MB)
            m_sizeBytes = 5 * 1024 * 1024;
            m_pcmData.resize(m_sizeBytes);
            m_isLoaded = true;
            std::cout << "[AudioClip] Nạp thành công! Tiêu tốn 5 MB RAM.\n";
        }

        void Unload() override {
            if (!m_isLoaded) return;
            std::cout << "[AudioClip] Đang giải phóng bộ nhớ cho: " << m_path << "...\n";
            m_pcmData.clear(); 
            m_pcmData.shrink_to_fit();
            m_sizeBytes = 0;
            m_isLoaded = false;
        }

        bool IsLoaded() const override { return m_isLoaded; }
        std::size_t GetMemoryFootprint() const override { return m_sizeBytes; }
        const std::string& GetPath() const override { return m_path; }
    };
    
} // namespace GameEngine
