#pragma once
#include <string>
#include <iostream>

namespace GameEngine {

    /**
     * @brief Lớp Transcoder giả lập quá trình giải nén.
     * Trong thực tế, dữ liệu nén (PNG, MP3) cần được giải mã 
     * thành dữ liệu thô (Raw Pixels, PCM) trước khi đẩy lên GPU/Sound Card.
     */
    class Transcoder {
    public:
        static void DecodeToRawPixels(const std::string& encodedData) {
            std::cout << "[Transcoder] Đang giải nén Texture sang chuẩn RGBA thô...\n";
        }
        
        static void DecodeToPCM(const std::string& compressedAudio) {
            std::cout << "[Transcoder] Đang giải nén Audio MP3/OGG sang chuẩn PCM...\n";
        }
    };

} // namespace GameEngine
