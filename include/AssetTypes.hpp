#pragma once
#include <string>

namespace GameEngine {

    /**
     * @brief Phân loại các định dạng tài sản được Engine hỗ trợ.
     */
    enum class AssetType {
        TEXTURE_2D,
        MESH_3D,
        AUDIO_CLIP,
        UNKNOWN
    };

    /**
     * @brief Hàm tiện ích Inline (không tốn chi phí gọi hàm) để 
     * tự động nhận diện loại tài nguyên dựa trên đuôi mở rộng file.
     */
    inline AssetType DetectAssetTypeFromPath(const std::string& path) {
        if (path.find(".png") != std::string::npos || path.find(".jpg") != std::string::npos) {
            return AssetType::TEXTURE_2D;
        }
        if (path.find(".obj") != std::string::npos || path.find(".fbx") != std::string::npos) {
            return AssetType::MESH_3D;
        }
        if (path.find(".wav") != std::string::npos || path.find(".mp3") != std::string::npos) {
            return AssetType::AUDIO_CLIP;
        }
        return AssetType::UNKNOWN;
    }

} // namespace GameEngine
