# 🎮 Game Asset Pipeline & Resource Management

> Khung quản lý tài nguyên và quy trình xử lý tài sản trò chơi đa nền tảng bằng C++. Đồ án ứng dụng chuyên sâu Kỹ thuật phần mềm (Software Engineering) với các Design Patterns và Quản lý bộ nhớ an toàn.

---

## 🌟 Tính năng Cốt lõi (Core Features)

Dự án mô phỏng Lõi (Core) của một Game Engine hiện đại, tập trung giải quyết 3 bài toán kinh điển:

1. **Zero Memory Leak & RAII:** Ứng dụng triệt để Smart Pointers (`std::shared_ptr`, `std::weak_ptr`) chuẩn C++17 và thu gom rác (Garbage Collection) thông minh.
2. **Flyweight Pattern:** Chia sẻ tài nguyên (Mesh, Texture) giữa hàng nghìn Entity (Thực thể), giảm mức tiêu thụ RAM từ **Hàng chục GB xuống còn vài chục MB**.
3. **Proxy Pattern & Async Lazy Loading:** Trì hoãn việc nạp ổ đĩa cho đến khi máy ảnh thực sự nhìn thấy (Lazy Load). Kết hợp nạp đa luồng (Async Load) giúp Game không bị giật/khựng (stuttering) khi chuyển cảnh.

---

## 📂 Cấu trúc Thư mục

```text
📦 project/
 ┣ 📂 include/                  # Chứa toàn bộ giao diện và kiến trúc lõi
 ┃ ┣ 📂 Assets/
 ┃ ┃ ┣ 📜 AudioClip.hpp      # (Âm thanh sóng PCM)
 ┃ ┃ ┣ 📜 Mesh3D.hpp         # (Mô hình 3D đỉnh không gian)
 ┃ ┃ ┣ 📜 ProxyAsset.hpp     # (Vỏ bọc Proxy Nạp muộn & Nạp nền)
 ┃ ┃ ┣ 📜 Texture2D.hpp      # (Điểm ảnh Texture 2D 4K)
 ┃ ┃ ┗ 📜 Transcoder.hpp     # (Giả lập giải mã/nén file đồ họa)
 ┃ ┣ 📜 AssetTypes.hpp       # Phân loại tài sản (Enum an toàn kiểu)
 ┃ ┣ 📜 IAsset.hpp           # Giao diện (Interface) tiêu chuẩn
 ┃ ┗ 📜 ResourceManager.hpp  # Singleton Sổ cái trung tâm (Trái tim của hệ thống)
 ┣ 📂 src/
 ┃ ┗ 📜 main.cpp             # Game Loop thực tế (Demo Stress-test 1000 quái vật)
 ┣ 📜 CMakeLists.txt         # Cấu hình biên dịch tự động chuẩn công nghiệp
 ┗ 📜 README.md              # Tài liệu này
```

---

## 🚀 Hướng dẫn Sử dụng (Tutorial)

### 1. Yêu cầu Hệ thống
- **Hệ điều hành:** Windows, macOS, hoặc Linux.
- Trình biên dịch hỗ trợ **C++17** (GCC 9+, Clang 10+, hoặc MSVC 2019+).
- **CMake** (phiên bản 3.15 trở lên) và **Ninja** (hoặc MinGW Makefiles).

### 2. Hướng dẫn Biên dịch (Build)
Mở Terminal tại thư mục gốc của dự án (nơi chứa file `CMakeLists.txt`) và gõ lần lượt các lệnh sau:

```bash
# 1. Tạo thư mục Build để chứa các file hệ thống sinh ra (Tránh rác thư mục chính)
mkdir build
cd build

# 2. Sinh kịch bản cấu hình bằng CMake (Khuyên dùng Ninja để biên dịch nhanh)
cmake -G Ninja ..

# 3. Tiến hành biên dịch mã nguồn thành File thực thi
cmake --build .
```

### 3. Khởi chạy (Run)
Sau khi biên dịch thành công 100% không cảnh báo (nhờ cờ bảo mật `/WX` hoặc `-Werror`), chạy file thực thi:

- **Trên Windows:**
  ```bash
  .\GameAssetPipeline.exe
  ```
- **Trên Linux/macOS:**
  ```bash
  ./GameAssetPipeline
  ```

### 4. Kết quả mong đợi (Demo Output)
Khi chạy, kịch bản `main.cpp` sẽ tiến hành biểu diễn sức mạnh kiến trúc qua 4 giai đoạn:
1. **Pha 1 (Spawn):** Sinh ra 1000 con quái vật Orc cùng lúc. Bạn sẽ thấy quá trình này hoàn tất ngay lập tức nhờ **Lazy Loading** (Quái vật chỉ ôm Proxy, không load ổ đĩa cứng).
2. **Pha 2 (Render Cache HIT):** Game kích hoạt vẽ quái vật. Con #1 ngốn 88 MB bộ nhớ. Từ con #2 đến #1.000 đều báo **Cache HIT** -> Sử dụng lại vùng nhớ có sẵn nhờ **Flyweight Pattern**.
3. **Pha 3 (Async Task):** Gọi nạp một bản Nhạc nền đánh Boss siêu nặng (`LoadAsync`). Màn hình vẫn in dòng chữ "Đang xử lý logic vật lý..." đều đặn ở 60 FPS, chứng tỏ Main Thread không bị đứng hình.
4. **Pha 4 (Garbage Collection):** Kết thúc Màn chơi 1, 1000 quái vật bị xóa bỏ. RAM tự động xả 88MB về mức 0 (Tính năng RAII). Trình dọn rác quét dọn sạch sẽ các con trỏ treo. Hệ thống báo dòng chữ **[Thành công] Khong xay ra Memory Leak.**
