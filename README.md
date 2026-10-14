# Khung Quản Lý Tài Nguyên Game Đa Nền Tảng (Thuần C++ / OOP)

Dự án đồ án môn **Lập Trình Hướng Đối Tượng (OOP)** - Viết hoàn toàn bằng **C++ thuần (Modern C++17)**, giao diện đồ họa **Native C++ Game Window (Raylib)**.

---

## 1. Mẫu Thiết Kế (Design Patterns) Đã Cài Đặt

1. **Singleton Pattern (`AssetManager`)**:
   - Quản lý Cache Registry duy nhất toàn cục.
   - Sử dụng `unordered_map<string, weak_ptr<IAsset>>` kết hợp **Reference Counting** tự động dọn dẹp bộ nhớ khi không còn đối tượng nào tham chiếu (`weak_ptr::expired() == true`).
2. **Flyweight Pattern (`IAsset` & `GameObject`)**:
   - Tách biệt **Intrinsic State** nặng (Texture 2D, 3D Mesh, Audio Buffer) và **Extrinsic State** nhẹ (X, Y, vận tốc, góc quay trong `GameObject`).
   - Hàng ngàn thực thể quái vật dùng chung 1 con trỏ `shared_ptr<IAsset>`, tiết kiệm >90% RAM/VRAM.
3. **Proxy Pattern (`AssetProxy`)**:
   - Đóng vai trò **Virtual Proxy** / **Lazy Loading**: hiển thị hình ảnh tạm thời (*Placeholder*) và nạp dữ liệu thật trên luồng ngầm (*Background Thread*) không gây đứng hình màn hình chính (anti-stuttering).
4. **Cache Engine & LRU Eviction**:
   - Thuật toán LRU dọn dẹp các tài sản ít dùng nhất khi bộ nhớ vượt ngưỡng.
5. **Asset Transcoder (`AssetTranscoder`)**:
   - Mô phỏng và chuyển mã nén texture GPU (Raw RGBA $\to$ DDS/DXT5/WebP) giúp tiết kiệm 75-80% VRAM.
6. **Memory Profiler Dashboard**:
   - Bảng đo đạc thời gian thực ngay trên màn hình game: RAM vật lý của hệ thống (OS Windows API `GetProcessMemoryInfo`), FPS, Frame Time, Cache Count.

---

## 2. Hướng Dẫn Điều Khiển Màn Hình Game

Khi khởi chạy cửa sổ game 1280x720, bạn có thể tương tác bằng các phím:

| Phím bấm / Thao tác | Chức năng & Tác dụng thực tế |
| :---: | :--- |
| **`[Nút Giữa □ (Maximize)]`** | **Phóng to cửa sổ toàn màn hình** (hoặc bấm **`[F11]`** / **`[Alt + Enter]`**). |
| **`[SPACE]` / `[ENTER]`** | **Spawn thêm 50 quái vật** mỗi lần nhấn vào chiến trường. |
| **`[C]`** | **Xóa sạch quái vật**: Toàn bộ tham chiếu `shared_ptr` bị hủy, kiểm chứng `weak_ptr` tự dọn RAM về 0. |
| **`[1]`** | **Bật/Tắt Flyweight**: Tắt Flyweight để quan sát RAM tăng vọt do mỗi quái vật nhân bản 1 texture riêng. |
| **`[2]`** | **Bật/Tắt Proxy (Async)**: Bật để nạp ngầm mượt mà 60 FPS, tắt để thấy hiện tượng giật lag khi nạp đồng bộ. |
| **`[3]`** | **Kích hoạt LRU Evict**: Thủ công dọn dẹp các asset ít dùng nhất trong Cache Registry. |
| **`[4]`** | **Transcoder Nén VRAM**: Chạy thử nghiệm nén texture sang định dạng GPU tối ưu. |
| **`[P]`** | **Phát Âm Thanh SFX**: Thử nghiệm phương thức `play()` của `AudioAsset`. |
| **`[F11]`** | **Bật / Tắt Toàn màn hình (Fullscreen Toggle)**. |

---

## 3. Cách Biên Dịch & Khởi Chạy

### Cách 1: Khởi chạy nhanh
Chạy file script [run.bat](file:///c:/Users/PC/OneDrive%20-%20National%20Economics%20University/K%C3%AC%203/OOP/project/run.bat) hoặc gõ trong terminal:
```powershell
.\run.bat
```

### Cách 2: Biên dịch thủ công bằng CMake
```powershell
cmake -B build -G "Ninja"
cmake --build build
.\build\GameAssetManager.exe
```

---

## 4. Phân Tích Chức Năng Trước & Sau Khi Bật/Tắt (Đối Chứng Thực Nghiệm)

Phần dưới đây mô tả chi tiết bản chất kỹ thuật OOP và hiện tượng trực quan trên màn hình game giúp người xem và giảng viên dễ dàng nhận biết:

### 4.1. Mẫu Flyweight (Phím `[1]`)
* **Bản chất kỹ thuật**: Tách biệt **Intrinsic State** (dữ liệu nặng không đổi như mảng pixel, đỉnh 3D) và **Extrinsic State** (tọa độ vị trí X, Y, góc xoay thay đổi liên tục).

| Trạng thái | Cơ chế bên trong | Trực quan trên màn hình Game |
| :--- | :--- | :--- |
| **Khi TẮT Flyweight (`[OFF]`)** | Mỗi quái vật sinh ra bị nhân bản một bản sao Texture độc lập, cấp phát riêng **4 MB RAM vật lý thật**. | - Khi có 50 quái: **Process RAM (OS)** trên Dashboard nhảy vọt từ ~35 MB lên **235 MB** (chữ hiển thị chuyển sang màu ĐỎ cảnh báo).<br>- Spawn thêm 50 quái (100 quái): RAM tăng vọt lên **435 MB**!<br>- Bảng trạng thái hiển thị: `1. Flyweight: [OFF] Duplicate!`. |
| **Khi BẬT Flyweight (`[ON]`)** | Toàn bộ quái vật cùng loại chỉ chia sẻ và trỏ chung vào **duy nhất 1 con trỏ `shared_ptr<IAsset>`** trong Cache Registry của `AssetManager`. | - Ngay khi bấm phím `[1]`, RAM lập tức tụt từ 235 MB / 435 MB về mức siêu nhẹ **~35 MB - 40 MB** (tiết kiệm **>90% RAM**).<br>- Dù spawn thêm hàng trăm quái vật, RAM vẫn giữ nguyên mức thấp, không bị phình to.<br>- Bảng trạng thái hiển thị: `1. Flyweight: [ON] Shared RAM` (màu xanh lá). |

---

### 4.2. Mẫu Virtual Proxy & Pipeline Nạp Bất Đồng Bộ (Phím `[2]`)
* **Bản chất kỹ thuật**: Trì hoãn nạp dữ liệu nặng từ đĩa/mạng, cung cấp một đối tượng đại diện mờ nhẹ (*Placeholder*) và nạp ngầm trên luồng phụ (*Background Thread*), ngăn chặn hiện tượng đứng hình (stuttering).

| Trạng thái | Cơ chế bên trong | Trực quan trên màn hình Game |
| :--- | :--- | :--- |
| **Khi TẮT Proxy (`[OFF]`)** | **Nạp đồng bộ (Blocking Load)**: Quá trình đọc I/O và giải nén tài nguyên diễn ra trực tiếp trên Main Thread (luồng vẽ của Game Loop), khiến CPU bị nghẽn. | - Mỗi lần bấm spawn 50 quái vật, toàn bộ màn hình game bị **đóng băng / khựng lại (Lag Spike / Freeze)** trong khoảng 1 - 1.5 giây.<br>- Chỉ số **FPS tụt thẳng đứng từ 60 FPS xuống còn 0 - 15 FPS**, các quái vật đang di chuyển bị giật đứng yên chớp nhoáng.<br>- Bảng trạng thái hiển thị: `2. Proxy: [OFF] Blocking` (màu đỏ). |
| **Khi BẬT Proxy (`[ON]`)** | **Nạp bất đồng bộ (Virtual Proxy)**: Main Thread lập tức tạo `AssetProxy` với `PlaceholderAsset` siêu nhẹ (~1KB) và đẩy tác vụ I/O xuống luồng phụ `std::async`. Sau khi xong sẽ hoán đổi (*atomic swap*) an toàn trên Main Thread. | - Khi spawn 50 quái vật, game **chạy mượt mà tuyệt đối 60 FPS**, không hề có giật lag.<br>- Trong 1.2 giây đầu tiên, quái vật xuất hiện ngay dưới dạng **Bóng ma mờ ảo (Placeholder Ghost)** kèm biểu tượng xoay và nhãn vàng `LOAD...` trên đầu.<br>- Sau 1.2 giây, khi nạp xong, bóng ma tự động biến hình mượt mà thành quái vật sắc nét hoàn chỉnh (mắt, sừng, miệng máu riêng biệt).<br>- Bảng trạng thái hiển thị: `2. Proxy: [ON] Async/Lazy` (màu xanh lá). |

---

### 4.3. Mẫu Singleton & Cơ Chế Đếm Tham Chiếu Reference Counting (Phím `[C]`)
* **Bản chất kỹ thuật**: `AssetManager` là điểm truy cập duy nhất điều phối tài nguyên thông qua Cache Registry sử dụng `unordered_map<string, weak_ptr<IAsset>>`. Tránh bẫy rò rỉ bộ nhớ kinh điển khi Cache lưu `shared_ptr`.
* **Thao tác kiểm chứng**:
  1. Khi trên màn hình đang có hàng trăm quái vật và chiếm lượng RAM lớn, nhấn phím **`[C]`**.
  2. Toàn bộ danh sách `GameObject` bị hủy $\implies$ số lượng tham chiếu `shared_ptr` trỏ tới các asset rơi về 0.
  3. `AssetManager` quét Cache Registry, phát hiện `weak_ptr::expired() == true` và lập tức giải phóng toàn bộ vùng đệm bộ nhớ.
* **Trực quan trên màn hình Game**:
  - Toàn bộ quái vật trên màn hình lập tức biến mất sạch sẽ.
  - Số lượng `Entities` giảm về `0 objects`.
  - Bộ nhớ `Process RAM (OS)` ngay lập tức hạ xuống mức nền của hệ điều hành.
  - Thanh thông báo trạng thái phía dưới chân màn hình hiện dòng chữ: *"Da xoa toan bo quai vat! Reference Counting da giam ve 0."*

---

### 4.4. Bộ Chuyển Mã Nén Texture Transcoder (Phím `[4]`)
* **Bản chất kỹ thuật**: Chuyển đổi dữ liệu hình ảnh thô không nén (Raw RGBA 32-bit = 4 bytes/pixel) sang các định dạng khối nén phần cứng GPU tối ưu (như DDS DXT5 hoặc WebP).
* **Trực quan trên màn hình Game**:
  - Nhấn phím `[4]`, hệ thống tính toán và hiển thị thông báo ngay trên Status Bar:
    * Kích thước Texture gốc: 4 MB VRAM.
    * Kích thước sau chuyển mã DXT5: 1 MB VRAM (chỉ còn 25%).
    * **Tỉ lệ tiết kiệm VRAM: 75.0%**.

---

## 5. Tài Liệu Tham Khảo
- Bản vẽ thiết kế lớp chi tiết: [docs/UML_Class_Diagram.md](file:///c:/Users/PC/OneDrive%20-%20National%20Economics%20University/K%C3%AC%203/OOP/project/docs/UML_Class_Diagram.md)
- Kế hoạch triển khai đầy đủ: [Plan.md](file:///c:/Users/PC/OneDrive%20-%20National%20Economics%20University/K%C3%AC%203/OOP/project/Plan.md)
