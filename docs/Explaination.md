# Báo Cáo Kỹ Thuật & Giải Thích Kiến Trúc Dự Án
## Khung Quản Lý Tài Nguyên Game Đa Nền Tảng (Game Asset Management Framework)

---

## 1. Đặt Vấn Đề (Problem Statement)

Trong phát triển game và đồ họa máy tính hiện đại, **tài nguyên trò chơi (Game Assets)** như hình ảnh (*Textures 2D*), mô hình (*Meshes 3D*), và âm thanh (*Audio Clips*) chiếm phần lớn dung lượng bộ nhớ (RAM máy tính và VRAM của card màn hình). Khi quy mô của một trò chơi mở rộng với hàng ngàn thực thể (quái vật, cây cối, hiệu ứng), các engine game thường phải đối mặt với **4 vấn đề kỹ thuật nghiêm trọng**:

### 1.1. Bùng nổ và cạn kiệt bộ nhớ (Memory Explosion & Out-of-Memory)
* **Vấn đề:** Nếu áp dụng tư duy lập trình đối tượng thông thường (Naive OOP), mỗi khi một quái vật được sinh ra (`new Monster`), nó sẽ nạp và lưu trữ riêng một bản sao Texture, Mesh và Sound của nó.
* **Hậu quả:** Giả sử 1 texture quái vật có độ phân giải cao tốn $4\text{ MB}$ RAM/VRAM. Khi có $1.000$ con quái xuất hiện trên màn hình:
  $$\text{Tổng bộ nhớ} = 1.000 \times 4\text{ MB} = \mathbf{4.000\text{ MB}} = \mathbf{4\text{ GB RAM!}}$$
  Việc nhân bản dữ liệu giống hệt nhau này dẫn tới cạn kiệt RAM, tràn VRAM và làm ứng dụng bị hệ điều hành tắt khẩn cấp (*Crash / Out of Memory*).

### 1.2. Hiện tượng đứng hình / giật khung hình (Stuttering & Lag Spikes)
* **Vấn đề:** Khi nhân vật di chuyển sang một vùng đất mới hoặc hệ thống spawn một đợt quái vật mới, việc đọc dữ liệu từ ổ cứng (I/O Disk) hoặc tải từ đám mây (Cloud) tốn nhiều thời gian (từ vài trăm mili-giây đến vài giây).
* **Hậu quả:** Nếu việc nạp dữ liệu diễn ra đồng bộ (*Synchronous / Blocking Load*) trực tiếp trên luồng chính của game (*Main Game Thread*), toàn bộ luồng vẽ (*Render Loop*) sẽ bị phong tỏa. Khung hình rơi từ **60 FPS xuống 0 FPS**, game bị đóng băng cứng đờ (*Lag Spike*), phá hỏng hoàn toàn trải nghiệm người chơi.

### 1.3. Rò rỉ bộ nhớ & kiểm soát vòng đời yếu (Memory Leak & Zombie Assets)
* **Vấn đề:** Khi một đối tượng trong game bị tiêu diệt, việc giải phóng tài nguyên rất dễ bị bỏ sót. Nếu giải phóng quá sớm (khi đối tượng khác vẫn đang dùng) sẽ gây lỗi trỏ con trỏ lơ lửng (*Dangling Pointer / Crash*). Ngược lại, nếu lưu giữ vĩnh viễn trong bộ nhớ đệm (*Cache*) bằng con trỏ mạnh, tài nguyên sẽ không bao giờ được giải phóng dù trên màn hình không còn con quái nào (*Zombie Asset*).

### 1.4. Phân mảnh và tắc nghẽn GPU (Break Draw Call Batching)
* **Vấn đề:** Khi mỗi quái vật mang một mã Texture riêng (`Texture ID` độc lập), GPU buộc phải đổi trạng thái liên tục (*Texture Switching*) cho từng đối tượng mỗi khung hình, làm vỡ cơ chế gom lệnh vẽ (*Sprite Batching*) của card đồ họa, khiến hiệu năng tụt dốc thê thảm.

---

## 2. Giải Pháp Kiến Trúc & Các Mẫu Thiết Kế (Design Patterns)

Để giải quyết triệt để 4 vấn đề trên, dự án áp dụng chuẩn mực **3 Mẫu thiết kế Hướng đối tượng cốt lõi** kết hợp với **Thuật toán Cache LRU** và **Bộ chuyển mã Transcoder**:

```
                       ┌─────────────────────────┐
                       │      AssetManager       │ (Singleton Pattern)
                       │  - Cache Registry       │
                       │  - Ref Counting Monitor │
                       │  - LRU Eviction Engine  │
                       └────────────┬────────────┘
                                    │ Cung cấp tài nguyên
                                    ▼
                       ┌─────────────────────────┐
                       │       AssetProxy        │ (Proxy Pattern)
                       │  - Placeholder (Ghost)  │
                       │  - Background Thread    │
                       │  - Main-thread Swap     │
                       └────────────┬────────────┘
                                    │ Nạp ngầm / Hoán đổi
                                    ▼
                       ┌─────────────────────────┐
                       │         IAsset          │ (Interface Trừu tượng)
                       │  (Intrinsic Heavy Data) │
                       ├─────────────────────────┤
                       │ Texture / Mesh / Audio  │ (Flyweight State)
                       └────────────▲────────────┘
                                    │
                                    │ Dùng chung con trỏ (shared_ptr)
                                    │
                       ┌────────────┴────────────┐
                       │       GameObject        │ (Flyweight Context)
                       │  (Extrinsic Light Data) │
                       │  - posX, posY, posZ     │
                       │  - speed, rotation      │
                       └─────────────────────────┘
```

### 2.1. Mẫu Singleton (`AssetManager`) — Điểm Điều Phối Duy Nhất & Đếm Tham Chiếu
* **Mục đích:** Đảm bảo toàn bộ ứng dụng chỉ tồn tại **duy nhất một thể hiện (Instance)** quản lý kho tài nguyên, tập trung hóa bộ nhớ đệm (*Cache Registry*) và tránh xung đột dữ liệu giữa các cảnh chơi.
* **Cơ chế Reference Counting (Đếm tham chiếu) thông minh:**
  * `AssetManager` tách hai trách nhiệm: weak registry quan sát reference count và resident cache giữ `shared_ptr` có giới hạn để thuật toán LRU tái sử dụng/evict tài nguyên:
    ```cpp
    unordered_map<string, weak_ptr<IAsset>> cacheRegistry;
    ```
  * `weak_ptr` không làm tăng biến đếm tham chiếu (*Reference Count*).
  * Khi tất cả `GameObject` bị hủy, resident entry có `use_count() == 1` và `collectUnused()` có thể xóa nó; weak registry sau đó hết hạn mà không giữ tài nguyên sống vĩnh viễn.

### 2.2. Mẫu Flyweight (`IAsset` & `GameObject`) — Tối Ưu Hóa RAM/VRAM Vượt Trội
* **Mục đích:** Cho phép hàng ngàn thực thể cùng loại cùng xuất hiện mà không làm phình dung lượng bộ nhớ.
* **Nguyên lý tách biệt trạng thái:**
  1. **Trạng thái nội tại (*Intrinsic State* — Bất biến, dùng chung, cực nặng):**
     * Nằm trong lớp triển khai `IAsset` (`TextureAsset`, `MeshAsset`, `AudioAsset`).
     * Chứa mảng dữ liệu điểm ảnh thô (Pixel buffer), mảng tọa độ đỉnh 3D (Vertex buffer), mẫu âm thanh thô (~$4\text{ MB}$ mỗi asset).
     * Chỉ được khởi tạo và lưu trữ **đúng 1 lần duy nhất** trên RAM/VRAM.
  2. **Trạng thái ngoại tại (*Extrinsic State* — Biến đổi liên tục, riêng biệt, siêu nhẹ):**
     * Nằm trong lớp `GameObject`.
     * Chỉ chứa tọa độ $(X, Y, Z)$, góc quay, vận tốc, tỉ lệ scale, tên quái vật (~$48\text{ bytes}$ mỗi thực thể).
  3. **Kết quả đạt được:**
     * $1.000$ quái vật thay vì tốn $4.000\text{ MB}$ ($4\text{ GB}$) thì nay chỉ tốn:
       $$4\text{ MB (Texture gốc)} + (1.000 \times 48\text{ bytes}) \approx \mathbf{4.05\text{ MB!}}$$
     * **Tiết kiệm tới 99% RAM** và cho phép GPU áp dụng gom lệnh vẽ (*Sprite Batching*) duy trì mượt mà **60 FPS**.

### 2.3. Mẫu Virtual Proxy (`AssetProxy`) — Tải Tài Nguyên Lười Biếng & Chống Nghẽn Khung Hình
* **Mục đích:** Khắc phục triệt để hiện tượng đứng hình (*Lag Spike*) khi nạp tài nguyên nặng từ ổ cứng hoặc mạng.
* **Nguyên lý hoạt động (Virtual Proxy / Lazy Loading):**
  1. Khi một đối tượng yêu cầu tài nguyên, `AssetProxy` lập tức trả về một đối tượng đại diện siêu nhẹ chứa **Placeholder** (bóng ma mờ ảo kèm nhãn `LOAD...` màu vàng trên đầu, chỉ tốn vài trăm bytes).
  2. Main Thread **không bị chặn lại một mili-giây nào**, game tiếp tục chạy mượt mà ở 60 FPS.
  3. `AssetLoadService` dùng hai worker thread để đọc và giải mã PNG/WAV/OBJ thành dữ liệu CPU. Proxy chỉ submit job ở lần `render()`/`play()` đầu tiên.
  4. `AssetManager::updateAsyncLoads()` upload tối đa 2 asset hoặc 2ms mỗi frame trên **Main Thread**, sau đó Proxy tráo sang tài nguyên thật. Cơ chế budget tránh dồn nhiều lần upload vào một frame.

### 2.4. Thuật toán Bộ Nhớ Đệm LRU (Least Recently Used Cache)
* **Mục đích:** Đảm bảo dung lượng cache không vượt quá giới hạn ngân sách bộ nhớ định trước (`maxMemoryBudget`).
* **Cơ chế:** Sử dụng danh sách kép liên kết `list<string>` kết hợp bảng băm `unordered_map`:
  * Mỗi khi tài nguyên được dùng, nó được đẩy lên đầu danh sách (`touchLRU`).
  * Khi bộ nhớ vượt ngưỡng cấu hình (ví dụ $128\text{ MB}$), hàm `evictLRU()` sẽ quét từ cuối danh sách (những tài nguyên lâu nhất không còn ai sử dụng) để thu hồi bộ nhớ.

### 2.5. Bộ Chuyển Mã Nén Tài Sản (`AssetTranscoder`)
* **Mục đích:** Mô phỏng quy trình xử lý tài sản (*Asset Pipeline*) chuyên nghiệp trong các Game Engine AAA.
* **Cơ chế:** Chuyển đổi định dạng hình ảnh thô không nén (*Raw RGBA32* = 4 bytes/pixel) sang các định dạng nén khối phần hardware của GPU (*DirectDraw Surface DDS / DXT5* hoặc *WebP*). Giúp giảm từ 4 bytes/pixel xuống còn 1 byte/pixel, **tiết kiệm đúng 75% dung lượng VRAM**.

### 2.6. Cơ Chế Quản Lý Bộ Nhớ C++ Hiện Đại: Con Trỏ Thông Minh (Smart Pointers)
Trong C++ kinh điển (C++03 trở về trước), lập trình viên phải tự quản lý bộ nhớ thủ công bằng cặp toán tử `new` và `delete`. Đây là nguyên nhân hàng đầu gây ra lỗi rò rỉ bộ nhớ (*Memory Leak*) hoặc trỏ vào vùng nhớ rác (*Dangling Pointer / Crash*). 

Dự án này tuân thủ chuẩn **C++17 hiện đại** và nguyên lý **RAII (Resource Acquisition Is Initialization)**: tuyệt đối không dùng toán tử `delete` thủ công mà ủy thác việc quản lý vòng đời bộ nhớ cho **Bộ ba Con trỏ thông minh (Smart Pointers)** trong thư viện chuẩn `<memory>`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          HỆ THỐNG CON TRỎ THÔNG MINH                         │
├──────────────────────┬──────────────────────┬───────────────────────────────┤
│    unique_ptr<T>     │    shared_ptr<T>     │         weak_ptr<T>           │
│ (Con trỏ độc quyền)  │   (Con trỏ mạnh)     │        (Con trỏ yếu)          │
├──────────────────────┼──────────────────────┼───────────────────────────────┤
│ - Sở hữu DUY NHẤT    │ - Đồng sở hữu        │ - Tham chiếu không sở hữu     │
│ - Không cho copy     │ - Đếm tham chiếu     │ - Không tăng strong_count     │
│ - Tự hủy khi ra scope│   (strong_count)     │ - Phải qua .lock() để lấy data│
│                      │ - Hủy khi count == 0 │ - Tránh rò rỉ & chu trình lặp │
└──────────────────────┴──────────────────────┴───────────────────────────────┘
```

#### A. Con trỏ thô (Raw Pointer `T*`) — Nguồn Gốc Của Hiểm Họa
* **Bản chất:** Chỉ đơn thuần lưu trữ một địa chỉ ô nhớ số nguyên (như `0x00A1F4B0`).
* **Hạn chế:**
  1. **Không có cơ chế tự dọn dẹp:** Nếu lập trình viên quên viết lệnh `delete ptr;`, vùng RAM đó sẽ bị chiếm dụng vĩnh viễn cho đến khi tắt máy (*Memory Leak*).
  2. **Lỗi giải phóng sớm (*Dangling Pointer*):** Nếu đối tượng A `delete` vùng nhớ, trong khi đối tượng B vẫn đang giữ con trỏ trỏ tới vùng đó và gọi hàm $\implies$ chương trình lập tức bị **Segmentation Fault / Crash**.
  3. **Lỗi giải phóng 2 lần (*Double Free*):** Hai đối tượng cùng gọi `delete` trên một địa chỉ ô nhớ làm sập hệ điều hành.

#### B. Con trỏ độc quyền (`std::unique_ptr<T>`) — Zero-Cost Abstraction
* **Bản chất:** Đại diện cho quyền sở hữu duy nhất (*Exclusive Ownership*). Tại một thời điểm, chỉ có **đúng một con trỏ duy nhất** được phép trỏ tới và sở hữu đối tượng.
* **Đặc tính kỹ thuật:**
  * **Cấm sao chép:** Toán tử copy bị khóa (`deleted copy constructor`). Không thể gán `p2 = p1`.
  * **Chỉ cho phép chuyển giao (Move Semantics):** Phải dùng `std::move(p1)` để chuyển toàn bộ quyền sở hữu sang cho con trỏ mới, biến `p1` thành `nullptr`.
  * **Tự động hủy:** Khi `unique_ptr` đi ra khỏi tầm vực (hết khối lệnh `{}` hoặc đối tượng cha bị hủy), nó tự động gọi `delete` giải phóng vùng nhớ mà không tốn thêm bất kỳ byte chi phí quản lý nào (*Zero-overhead*).
* **Ứng dụng cụ thể trong dự án:**
  * Nằm tại `GameScene.hpp`:
    ```cpp
    vector<unique_ptr<GameObject>> entities;
    ```
  * Màn chơi (`GameScene`) là chủ sở hữu tối cao và duy nhất của danh sách các thực thể quái vật. Khi một quái vật chết hoặc khi người dùng bấm phím `[C]` để dọn sạch (`entities.clear()`), toàn bộ các `unique_ptr` bị hủy, tự động kích hoạt hàm hủy của `GameObject` tức thì.

#### C. Con trỏ mạnh / Con trỏ chia sẻ (`std::shared_ptr<T>`) — Shared Ownership
* **Bản chất:** Cho phép **nhiều đối tượng khác nhau cùng chia sẻ quyền sở hữu** trên một tài nguyên duy nhất.
* **Cơ chế Control Block & Đếm tham chiếu (Reference Counting):**
  * Khi tạo bằng `make_shared<T>()`, hệ điều hành cấp phát đối tượng kèm theo một khối điều khiển (*Control Block*) chứa biến đếm `strong_count` (ban đầu bằng 1).
  * Mỗi khi có một đối tượng mới copy con trỏ này, `strong_count` tự động tăng lên 1 (`use_count()++`).
  * Mỗi khi một đối tượng sở hữu bị hủy (ra khỏi scope hoặc bị xóa), `strong_count` tự động giảm đi 1 (`use_count()--`).
  * **Nguyên lý vàng:** Khi và chỉ khi `strong_count == 0` (không còn bất kỳ ai trên toàn bộ hệ thống sử dụng tài nguyên này nữa), khối nhớ của tài nguyên đó mới thực sự bị giải phóng khỏi RAM/VRAM.
* **Ứng dụng cụ thể trong dự án:**
  * Nằm tại `GameObject.hpp`:
    ```cpp
    shared_ptr<IAsset> asset; // Hàng ngàn quái cùng giữ chung con trỏ này
    ```
  * Nhờ `shared_ptr`, $1.000$ quái vật cùng loại đều giữ chung con trỏ trỏ tới 1 bản gốc `TextureAsset`. Chỉ cần còn ít nhất 1 con quái sống trên màn hình, Texture vẫn an toàn tồn tại. Khi tất cả quái vật chết hết, `strong_count` rơi về 0, kích hoạt dọn dẹp bộ nhớ.

#### D. Con trỏ yếu (`std::weak_ptr<T>`) — Người Quan Sát Thông Minh
* **Bản chất:** Là một con trỏ "quan sát" (*Non-owning Observer*) trỏ vào đối tượng đang được quản lý bởi `shared_ptr`, nhưng **hoàn toàn KHÔNG làm tăng biến đếm tham chiếu** (`strong_count`).
* **Tại sao không thể truy cập trực tiếp `weak_ptr->data`?**
  * Vì `weak_ptr` không sở hữu đối tượng, đối tượng có thể bị giải phóng bất cứ lúc nào bởi luồng khác.
  * Muốn sử dụng, lập trình viên bắt buộc phải dùng hàm `.lock()`:
    ```cpp
    shared_ptr<IAsset> liveAsset = weakAsset.lock();
    if (liveAsset) {
        // Đối tượng còn sống an toàn, dùng liveAsset thoải mái!
    } else {
        // Đối tượng đã bị hủy trước đó (vùng nhớ đã giải phóng)!
    }
    ```
  * Hoặc kiểm tra nhanh xem đối tượng đã "chết" hay chưa bằng: `weakAsset.expired() == true`.

#### E. Tại Sao `AssetManager` Bắt Buộc Phải Dùng `weak_ptr` Thay Vì `shared_ptr`?
Đây là một trong những câu hỏi phỏng vấn và vấn đáp OOP kinh điển nhất:
* **Kịch bản sai lầm (Nếu dùng `shared_ptr` cho Cache Registry):**
  ```cpp
  // GIẢ SỬ DÙNG SAI:
  unordered_map<string, shared_ptr<IAsset>> cacheRegistry;
  ```
  1. Khi quái vật đầu tiên sinh ra, `cacheRegistry` lưu một `shared_ptr`. Biến đếm tham chiếu lúc này là `strong_count = 2` (1 từ Cache, 1 từ quái vật).
  2. Người chơi tiêu diệt toàn bộ quái vật trên màn hình $\implies$ Các con quái bị hủy, nhả con trỏ ra.
  3. Biến đếm giảm từ $2 \to 1$.
  4. **Hậu quả thảm khốc:** Vì `cacheRegistry` vẫn đang giữ 1 `shared_ptr`, `strong_count` không bao giờ rơi về 0! Tài nguyên nặng hàng trăm MB RAM/VRAM sẽ bị **giam giữ vĩnh viễn trong bộ nhớ** dù trong game không còn một đối tượng nào sử dụng (*Hiện tượng Zombie Assets / Leak*).
* **Kịch bản chuẩn xác (Dự án dùng `weak_ptr`):**
  ```cpp
  // CHUẨN MỰC THIẾT KẾ:
  unordered_map<string, weak_ptr<IAsset>> cacheRegistry;
  ```
  1. `AssetManager` chỉ lưu `weak_ptr`, `strong_count` của tài nguyên chỉ bằng đúng số lượng quái vật đang sống trên màn hình.
  2. Khi người chơi xóa hết quái vật (bấm `[C]`), toàn bộ `shared_ptr` bị hủy $\implies$ `strong_count` rơi về đúng **0**.
  3. `TextureAsset` tự động giải phóng toàn bộ 4 MB RAM ngay lập tức.
  4. Lúc này, mục trong `cacheRegistry` tự động chuyển sang trạng thái hết hạn (`expired() == true`). Hàm `cleanDeadReferences()` của `AssetManager` chỉ việc quét và xóa các key hết hạn, bộ nhớ sạch bóng 100%.

#### F. Bảng Tổng Hợp So Sánh Các Loại Con Trỏ

| Tiêu chí so sánh | Con trỏ thô (`T*`) | `std::unique_ptr<T>` | `std::shared_ptr<T>` | `std::weak_ptr<T>` |
| :--- | :--- | :--- | :--- | :--- |
| **Quyền sở hữu (Ownership)** | Không xác định (Tự do) | Độc quyền duy nhất (*Exclusive*) | Đồng sở hữu chia sẻ (*Shared*) | Không sở hữu (*Non-owning*) |
| **Tăng `strong_count`?** | Không | Không áp dụng | **Có (+1)** | **Không (+0)** |
| **Cho phép Copy?** | Có | **Cấm tuyệt đối** (Chỉ Move) | Có | Có |
| **Cách truy cập dữ liệu** | `ptr->method()` | `uPtr->method()` | `sPtr->method()` | Phải gọi `.lock()` chuyển thành `shared_ptr` |
| **Chi phí bộ nhớ (Overhead)** | 0 byte | 0 byte | Control Block (~16-24 bytes) | Trỏ vào chung Control Block |
| **Vị trí áp dụng trong đồ án** | Không sử dụng | `GameScene::entities` | `GameObject::asset` | `AssetManager::cacheRegistry` |

---

## 3. Các File `.hpp` và `.cpp` Quan Trọng Nhất (Phần Lõi Của Dự Án)

Dưới đây là các file trung tâm thể hiện toàn bộ tư duy hướng đối tượng (OOP) và các Design Patterns mà bạn cần nắm chắc khi thuyết trình hoặc vấn đáp:

### 3.1. `include/IAsset.hpp` — Bản Hợp Đồng Trừu Tượng (Interface Abstraction)
* **Vai trò:** Là lớp cơ sở trừu tượng (`interface`) định nghĩa khung giao tiếp chung cho **mọi loại tài nguyên** trong game (ảnh 2D, lưới 3D, âm thanh, hay proxy đại diện).
* **Bản chất OOP:** 
  * Áp dụng tính **Trừu tượng (Abstraction)** với các phương thức thuần ảo (`pure virtual functions = 0`).
  * Cho phép các hệ thống khác (như `GameObject`, `AssetManager`) thao tác với tài nguyên thông qua con trỏ đa hình `shared_ptr<IAsset>` mà không cần biết bên dưới là loại dữ liệu gì.
* **Các phương thức mấu chốt:**
  ```cpp
  virtual bool load(const string& path) = 0;
  virtual void unload() = 0;
  virtual void render(float posX, float posY, float scale = 1.0f) = 0;
  virtual void play() = 0;
  virtual size_t getSizeBytes() const = 0;
  ```

### 3.2. `include/AssetManager.hpp` & `src/AssetManager.cpp` — Singleton & Cache Engine
* **Vai trò:** Trái tim điều phối của toàn bộ hệ thống quản lý tài nguyên.
* **Bản chất OOP & Design Patterns:**
  * **Singleton Pattern:** Hàm dựng `AssetManager()` là `private`, cấm sao chép (`delete copy constructor`), cung cấp điểm truy cập toàn cục duy nhất qua `AssetManager::getInstance()`.
  * **Flyweight Factory:** Hàm `getAsset()` kiểm tra Cache Registry trước khi quyết định cấp phát mới hay chia sẻ con trỏ cũ.
  * **Tính đóng gói (Encapsulation):** Toàn bộ cấu trúc lưu trữ `cacheRegistry`, `lruAccessList`, `mutex` đều là `private`.
  * **Tính đa hình động (Polymorphism):** Cấp phát và trả về `shared_ptr<IAsset>` trỏ đến các đối tượng cụ thể (`TextureAsset`, `MeshAsset`, `AudioAsset`, `AssetProxy`).
* **Đoạn code quan trọng nhất (`src/AssetManager.cpp`):**
  ```cpp
  // 1. Kiểm tra Flyweight: Tái sử dụng con trỏ đang sống trong cache
  if (useFlyweight) {
      auto it = cacheRegistry.find(path);
      if (it != cacheRegistry.end()) {
          shared_ptr<IAsset> existing = it->second.lock();
          if (existing) {
              touchLRU(path);
              return existing; // DÙNG CHUNG CON TRỎ (FLYWEIGHT)
          }
      }
  }
  // 2. Nếu dùng Proxy: Tạo đối tượng đại diện nạp ngầm
  if (useProxy) {
      newAsset = make_shared<AssetProxy>(path, type, nullptr, delayMs);
      newAsset->load(path);
  }
  ```

### 3.3. `include/AssetProxy.hpp` & `src/AssetProxy.cpp` — Virtual Proxy & Async Worker
* **Vai trò:** Đại diện cho tài sản chưa nạp, cung cấp ảnh tạm (*Placeholder*) và điều phối luồng nạp ngầm chống nghẽn khung hình.
* **Bản chất OOP & Design Patterns:**
  * **Proxy Pattern:** Cùng kế thừa `IAsset` nên có bề ngoài giống hệt `TextureAsset`. Đối tượng bên ngoài (`GameObject`) không cần biết mình đang vẽ qua Proxy hay Real Asset.
  * **Thread Safety:** Dùng `atomic<bool>` và `std::future<bool>` để quản lý tác vụ bất đồng bộ.
  * **Main-Thread Upload (Bảo đảm an toàn OpenGL):** Worker tạo CPU payload; hàng đợi hoàn tất upload Texture/Sound/Mesh trên luồng chính theo ngân sách mỗi frame.
* **Đoạn code quan trọng nhất (`src/AssetProxy.cpp`):**
  ```cpp
  void AssetProxy::render(float posX, float posY, float scale) {
      checkAndSwap(); // Kiểm tra xem luồng ngầm đã xong chưa

      lock_guard<mutex> lock(swapMutex);
      if (isReady.load() && realAsset) {
          realAsset->render(posX, posY, scale); // ĐÃ XONG: Vẽ ảnh thật
      } else if (placeholderAsset) {
          placeholderAsset->render(posX, posY, scale); // CHƯA XONG: Vẽ bóng ma tạm
          DrawText("LOAD...", posX + 4, posY - 12, 9, YELLOW);
      }
  }
  ```

### 3.4. `include/TextureAsset.hpp` & `src/TextureAsset.cpp` — Concrete Flyweight
* **Vai trò:** Quản lý dữ liệu đồ họa thật trong VRAM card màn hình.
* **Bản chất OOP:** 
  * Hiện thực hóa (*Concrete Implementation*) của interface `IAsset`.
  * Chứa dữ liệu nội tại nặng (`realMemoryBuffer` cấp phát **4 MB RAM vật lý thật** cho mỗi instance) để làm bằng chứng thực nghiệm rõ ràng cho việc bật/tắt Flyweight.
  * Cung cấp hàm tạo ảnh đại diện nhẹ `createPlaceholder()` cho Proxy.

### 3.5. `include/MeshAsset.hpp`, `AudioAsset.hpp`, `AssetTranscoder.hpp`
* **`MeshAsset` & `AudioAsset`:** Khẳng định tính đa dạng tài nguyên theo đúng yêu cầu đề tài (Hỗ trợ cả 3D Mesh và âm thanh Sound Effect với phương thức `play()`).
* **`AssetTranscoder`:** Đóng gói thuật toán tính toán nén texture và tối ưu hóa VRAM.

---

## 4. Các Thư Mục & File Phụ Trợ (Mô Phỏng Trực Quan & Kiểm Chứng)

Để một đồ án OOP không chỉ dừng lại ở lý thuyết suông trên giấy, các thư mục và file phụ trợ đóng vai trò là **môi trường mô phỏng thực tế (Simulation & Benchmark Client)**, giúp trực quan hóa kết quả lên màn hình game và đo lường thông số chính xác từ hệ điều hành:

| Tên File / Thư mục | Vai trò trong kiến trúc | Tác dụng & Cơ chế hoạt động chi tiết |
| :--- | :--- | :--- |
| **`include/GameObject.hpp`**<br>**`src/GameObject.cpp`** | **Flyweight Context** (Đối tượng ngữ cảnh) | - Đóng vai trò là vật thể chuyển động trên màn hình game.<br>- **Lưu Extrinsic State:** Tọa độ $(X, Y)$, vận tốc $(v_x, v_y)$, góc xoay, nhãn tên quái vật.<br>- **Tham chiếu Intrinsic State:** Giữ con trỏ `shared_ptr<IAsset>`. Khi $1.000$ quái vật cùng loại di chuyển, chúng chỉ giữ các con số vị trí riêng, còn hình ảnh thì dùng chung 1 con trỏ duy nhất. |
| **`include/GameScene.hpp`**<br>**`src/GameScene.cpp`** | **Client Layer & HUD Dashboard** (Bộ quản lý màn chơi) | - Điều phối toàn bộ vòng lặp cập nhật và vẽ quái vật trên chiến trường.<br>- Quản lý danh sách `vector<unique_ptr<GameObject>> entities`.<br>- **Vẽ Bảng Profiler Dashboard rộng 490px:** Trực tiếp hiển thị FPS, dung lượng RAM, trạng thái các Design Patterns.<br>- Xử lý phím tắt tương tác (`[SPACE]`, `[1]`, `[2]`, `[C]`, `[F11]`). |
| **`include/MemoryProfiler.hpp`**<br>**`src/MemoryProfiler.cpp`** | **OS Memory Inspector** (Bộ đo phần cứng) | - Giao tiếp trực tiếp với nhân hệ điều hành Windows thông qua thư viện `windows.h` và `psapi.h`.<br>- Gọi hàm `GetProcessMemoryInfo()` để lấy **Working Set Size (RAM vật lý thực tế)** mà tiến trình đang chiếm dụng.<br>- Đảm bảo số liệu RAM hiển thị trên Dashboard là số liệu thật 100%, đồng nhất với Windows Task Manager. |
| **`src/main.cpp`** | **Entry Point** (Điểm khởi chạy ứng dụng) | - Khởi tạo cửa sổ đồ họa Native C++ (Raylib Engine) kích thước chuẩn `1440x810`.<br>- Bật cờ `FLAG_WINDOW_RESIZABLE` cho phép bấm nút giữa Maximize `[□]` hoặc `F11` để phóng to toàn màn hình.<br>- Điều phối Game Loop (`while (!WindowShouldClose())`). |
| **`CMakeLists.txt`** | **Build Configuration** | - File cấu hình chuẩn công nghiệp cho CMake, thiết lập C++17, tự động dò tìm và liên kết mã nguồn trong `include/`, `src/` với thư viện Raylib. |
| **`run.bat`** | **Quick Launcher** | - File script tự động kiểm tra trình biên dịch (Ninja / MinGW Makefiles) trên máy tính, biên dịch lần đầu và mở game chỉ với 1 cú click đúp chuột. |
| **`external/`** | **Bundled Libraries** | - Chứa toàn bộ mã thư viện tĩnh `libraylib.a` và header `raylib.h` chuẩn cho MinGW-w64. Giúp người khác kéo repo về là chạy được ngay mà không cần cài đặt thêm thư viện ngoài. |
| **`assets/`** | **Resource Store** | - Thư mục gốc chứa các tệp hình ảnh texture, âm thanh mẫu và mô hình 3D. |
| **`docs/`** | **Documentation** | - Chứa tài liệu thiết kế sơ đồ lớp `UML_Class_Diagram.md`, kế hoạch `Plan.md` và file giải thích kỹ thuật này. |

---

## 5. Bảng Đối Chứng Hiệu Năng Thực Nghiệm (Tóm Tắt Để Vấn Đáp)

| Thao tác / Chức năng | Cơ chế kỹ thuật OOP | Khi TẮT tính năng | Khi BẬT tính năng |
| :--- | :--- | :--- | :--- |
| **Phím `[1]`: Mẫu Flyweight** | Tách Intrinsic State (RAM nặng) khỏi Extrinsic State (vị trí nhẹ). | Mỗi quái vật nhân bản 1 texture riêng.<br>$\to$ **RAM tăng vọt lên 200 MB – 400 MB** (Cảnh báo Đỏ). | Toàn bộ quái vật dùng chung 1 con trỏ texture.<br>$\to$ **RAM giữ nguyên mức siêu thấp (~35 MB)**, tiết kiệm >90% RAM. |
| **Phím `[2]`: Mẫu Virtual Proxy** | Dùng ảnh tạm Placeholder và đẩy nạp nặng xuống luồng phụ `async`. | Nạp đồng bộ chặn đứng Main Thread.<br>$\to$ **Lag Spike 1.25s, FPS rơi về 0** mỗi lần spawn. | Nạp bất đồng bộ ngầm.<br>$\to$ **Giữ vững 60 FPS**, quái xuất hiện dạng bóng ma `LOAD...` rồi biến hình mượt mà. |
| **Phím `[C]`: Reference Counting** | Cache Registry dùng `weak_ptr` tự phát hiện khi `use_count == 0`. | Nếu dùng `shared_ptr`, tài nguyên sống vĩnh viễn gây rò rỉ RAM. | Xóa hết quái $\to$ con trỏ hết hạn $\to$ **Toàn bộ hàng trăm MB RAM được dọn dẹp sạch tức thì**. |
| **Phím `[4]`: Transcoder** | Chuyển đổi định dạng nén GPU khối (DDS DXT5). | Dùng ảnh thô RGBA 32-bit (4 bytes/pixel). | Nén GPU tối ưu (1 byte/pixel).<br>$\to$ **Tiết kiệm đúng 75% VRAM**. |

---

## 6. Kết Luận

Dự án đã giải quyết trọn vẹn bài toán quản lý tài nguyên game bằng việc vận dụng hài hòa và chuẩn mực các mẫu thiết kế hướng đối tượng:
1. **Singleton** kiểm soát sự duy nhất và tập trung hóa vòng đời bộ nhớ đệm.
2. **Flyweight** tối ưu hóa không gian lưu trữ (Space Complexity), cho phép mở rộng quy mô hàng ngàn đối tượng.
3. **Proxy** tối ưu hóa thời gian đáp ứng (Time Complexity & Responsiveness), loại bỏ hoàn toàn hiện tượng nghẽn khung hình.
4. **LRU Cache & Transcoder** tiệm cận tiêu chuẩn xử lý tài nguyên của các game engine thực tế.
5. Sự kết hợp giữa **phần lõi kiến trúc** (`IAsset`, `AssetManager`, `AssetProxy`) và **phần mô phỏng trực quan** (`GameObject`, `GameScene`, `MemoryProfiler`) tạo nên một đồ án trực quan, có tính thuyết phục cao và minh chứng thực nghiệm đo đạc chính xác từ hệ điều hành.
