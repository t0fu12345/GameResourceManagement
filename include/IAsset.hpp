#pragma once
#include <string>
#include <cstddef>

namespace GameEngine {

    /**
     * @class IAsset
     * @brief Interface cốt lõi cho mọi tài nguyên trong Game Engine.
     * Tuân thủ Dependency Inversion Principle (SOLID).
     */
    class IAsset {
    public:
        // Virtual destructor là bắt buộc để tránh rò rỉ bộ nhớ khi đa hình
        virtual ~IAsset() = default;

        // Kích hoạt tiến trình nạp tài nguyên từ ổ cứng/mạng
        virtual void Load(const std::string& path) = 0;

        // Hủy cấp phát vùng nhớ RAM/VRAM
        virtual void Unload() = 0;

        // Trạng thái tài nguyên hiện tại (đã nạp xong chưa)
        virtual bool IsLoaded() const = 0;

        // Trả về dung lượng bộ nhớ (Bytes) mà tài sản đang chiếm giữ
        virtual std::size_t GetMemoryFootprint() const = 0;

        // Định danh tài nguyên
        virtual const std::string& GetPath() const = 0;
    };

} // namespace GameEngine
