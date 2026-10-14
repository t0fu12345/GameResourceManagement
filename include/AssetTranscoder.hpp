#pragma once

#include <string>
#include <cstddef>

struct TranscodeResult {
    bool success{false};
    std::string sourceFormat;
    std::string targetFormat;
    size_t originalSizeBytes{0};
    size_t transcodedSizeBytes{0};
    float compressionRatio{0.0f}; // Ti le tiet kiem bo nho (%)
    std::string logMessage;
};

// Asset Transcoder: Chuyen ma va nen dinh dang tai nguyen de toi uu RAM/VRAM
class AssetTranscoder {
public:
    AssetTranscoder() = default;

    // Chuyen ma anh tu dinh dang thô (Raw PNG/RGBA32) sang dinh dang nen GPU (DDS/DXT5 hoac WebP)
    TranscodeResult transcodeTexture(const std::string& inputPath, const std::string& targetFormat = "DDS_DXT5");

    // Mo phong ti le nen cho bat ky dung luong bytes nao
    static size_t calculateCompressedSize(size_t rawBytes, float ratio = 0.25f);
};
