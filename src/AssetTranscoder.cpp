#include "AssetTranscoder.hpp"
#include <iostream>
#include <sstream>

using namespace std;

TranscodeResult AssetTranscoder::transcodeTexture(const string& inputPath, const string& targetFormat) {
    TranscodeResult result;
    result.sourceFormat = "Raw RGBA32 (.png)";
    result.targetFormat = targetFormat;

    // Gia su mot texture 1024x1024 RGBA 32-bit = 4 MB
    size_t simulatedRawBytes = 4 * 1024 * 1024;
    result.originalSizeBytes = simulatedRawBytes;

    float ratio = 0.25f; // DXT5/DDS nen giam 75% dung luong (chi con 25% kich thuoc goc)
    if (targetFormat == "WEBP") {
        ratio = 0.18f; // WebP nen sau hon (con 18%)
    }

    result.transcodedSizeBytes = static_cast<size_t>(simulatedRawBytes * ratio);
    result.compressionRatio = (1.0f - static_cast<float>(result.transcodedSizeBytes) / simulatedRawBytes) * 100.0f;
    result.success = true;

    stringstream ss;
    ss << "[Transcoder] Chuyen doi thanh cong [" << inputPath << "]:\n"
       << "  - Dinh dang: " << result.sourceFormat << " -> " << result.targetFormat << "\n"
       << "  - Dung luong truoc nen: " << result.originalSizeBytes / (1024 * 1024) << " MB\n"
       << "  - Dung luong sau nen:   " << result.transcodedSizeBytes / (1024 * 1024) << " MB (" 
       << result.transcodedSizeBytes / 1024 << " KB)\n"
       << "  - VRAM tiet kiem duoc:  " << result.compressionRatio << "%";

    result.logMessage = ss.str();
    cout << result.logMessage << "\n";

    return result;
}

size_t AssetTranscoder::calculateCompressedSize(size_t rawBytes, float ratio) {
    return static_cast<size_t>(rawBytes * ratio);
}
