# Thiết Kế Hệ Thống & Sơ Đồ Lớp (UML Class Diagram)

## 1. Tổng quan Kiến trúc (Architecture Overview)
Dự án được xây dựng dựa trên các nguyên lý Hướng đối tượng (OOP) và 3 mẫu thiết kế cốt lõi:
1. **Singleton Pattern**: `AssetManager` là trung tâm điều phối tài nguyên, kiểm soát Cache Registry và vòng đời tài sản.
2. **Flyweight Pattern**: Tách biệt trạng thái nội tại (*Intrinsic State* - `IAsset` chứa mảng pixel texture, đỉnh 3D, âm thanh) và trạng thái ngoại tại (*Extrinsic State* - `GameObject` chứa tọa độ X, Y, Z, góc xoay, vận tốc).
3. **Proxy Pattern (Virtual Proxy)**: `AssetProxy` hoạt động như một đại diện cho tài nguyên thực, cung cấp hình ảnh tạm thời (*Placeholder*) và nạp dữ liệu thật trên luồng ngầm (*Background Thread*) để tránh nghẽn khung hình.

---

## 2. Sơ đồ lớp chi tiết (UML Class Diagram)

```mermaid
classDiagram
    direction TB

    class AssetType {
        <<enumeration>>
        TEXTURE_2D
        MESH_3D
        AUDIO_CLIP
    }

    class IAsset {
        <<interface>>
        +load(string path)* bool
        +unload()* void
        +render(float x, float y, float scale)* void
        +play()* void
        +isLoaded()* bool
        +getSizeBytes()* size_t
        +getType()* AssetType
        +getPath() string
    }

    class TextureAsset {
        -Texture2D textureData
        -size_t memorySize
        -bool loaded
        -string filePath
        +load(string path) bool
        +unload() void
        +render(float x, float y, float scale) void
        +play() void
        +isLoaded() bool
        +getSizeBytes() size_t
        +getType() AssetType
    }

    class MeshAsset {
        -Model model3D
        -size_t memorySize
        -bool loaded
        -string filePath
        +load(string path) bool
        +unload() void
        +render(float x, float y, float scale) void
        +play() void
        +isLoaded() bool
        +getSizeBytes() size_t
        +getType() AssetType
    }

    class AudioAsset {
        -Sound soundData
        -size_t memorySize
        -bool loaded
        -string filePath
        +load(string path) bool
        +unload() void
        +render(float x, float y, float scale) void
        +play() void
        +isLoaded() bool
        +getSizeBytes() size_t
        +getType() AssetType
    }

    class AssetProxy {
        -string assetPath
        -AssetType assetType
        -shared_ptr~IAsset~ realAsset
        -shared_ptr~IAsset~ placeholderAsset
        -atomic~bool~ isLoading
        -AssetLoadState state
        -shared_ptr~AssetLoadTicket~ ticket
        +load(string path) bool
        +unload() void
        +render(float x, float y, float scale) void
        +play() void
        +isLoaded() bool
        +getSizeBytes() size_t
        +getType() AssetType
        +checkAndSwap() void
    }

    class AssetManager {
        -AssetCache cache
        -AssetLoadService loader
        -AssetManager()
        +static getInstance() AssetManager&
        +getAsset(string path, AssetType type, bool useProxy) shared_ptr~IAsset~
        +updateAsyncLoads(size_t maxUploads, double budgetMs) void
        +evictLRU(size_t neededBytes) void
        +collectUnused() size_t
        +shutdown() void
        +getMemoryStats() MemoryStats
    }

    class AssetCache {
        -unordered_map~AssetKey, weak_ptr~IAsset~~ registry
        -unordered_map~AssetKey, shared_ptr~IAsset~~ residentAssets
        -list~AssetKey~ lru
        +find(AssetKey) shared_ptr~IAsset~
        +insert(AssetKey, shared_ptr~IAsset~) void
        +evictLRU(size_t neededBytes) size_t
        +collectUnused() size_t
    }

    class AssetLoadService {
        -deque~WorkItem~ pending
        -vector~thread~ workers
        +submit(string path, AssetType type) AssetLoadTicket
        +pumpMainThreadUploads(size_t max, double budgetMs) size_t
        +shutdown() void
    }

    class AssetTranscoder {
        +transcodeTexture(string rawPath, string targetFormat) bool
        +simulateCompression(size_t rawBytes, float ratio) size_t
    }

    class GameObject {
        +float posX
        +float posY
        +float posZ
        +float rotation
        +float scale
        +float speed
        -shared_ptr~IAsset~ sharedAsset
        +update(float dt) void
        +draw() void
        +playSound() void
    }

    class MemoryProfiler {
        +getProcessRAMBytes() size_t
        +getFormattedMemory(size_t bytes) string
        +getFPS() int
    }

    %% Relationships
    IAsset <|.. TextureAsset : Implements (Flyweight Intrinsic)
    IAsset <|.. MeshAsset : Implements (Flyweight Intrinsic)
    IAsset <|.. AudioAsset : Implements (Flyweight Intrinsic)
    IAsset <|.. AssetProxy : Implements (Proxy Pattern)
    
    AssetProxy o-- IAsset : Wraps (Real & Placeholder)
    AssetManager --> IAsset : Manages Lifecycle & Cache
    AssetManager *-- AssetCache : Owns cache policy
    AssetManager *-- AssetLoadService : Owns worker pool
    AssetProxy --> AssetLoadService : Submits lazy job
    AssetManager --> AssetTranscoder : Invokes for VRAM Optimization
    GameObject o-- IAsset : References (Flyweight Context)
    GameScene --> AssetManager : Requests Assets
    GameScene --> GameObject : Updates & Renders Horde
    GameScene --> MemoryProfiler : Queries Metrics
```

---

## 3. Trách nhiệm các mẫu thiết kế

### 3.1. Singleton Pattern (`AssetManager`)
- **Động cơ**: Chỉ một bộ điều phối tài nguyên duy nhất tồn tại trong suốt vòng đời game để tránh lãng phí RAM/VRAM và xung đột cache.
- **Cơ chế đếm tham chiếu (Reference Counting)**:
  - Cache Registry sử dụng `unordered_map<string, weak_ptr<IAsset>>`.
  - Bộ quản lý không sở hữu độc quyền tài nguyên (không giữ `shared_ptr`), mà trao quyền sở hữu cho các `GameObject`. Khi mọi đối tượng bị tiêu diệt, `weak_ptr.expired()` kích hoạt và giải phóng bộ nhớ tự động.

### 3.2. Flyweight Pattern (`IAsset` & `GameObject`)
- **Động cơ**: Vẽ 1.000 quái vật giống nhau. Nếu mỗi quái vật tạo một bản sao texture 10MB $\to$ tiêu tốn $1.000 \times 10\text{MB} = 10\text{GB}$ RAM (gây tràn bộ nhớ).
- **Giải pháp**:
  - **Intrinsic State** (Nội tại, không đổi): Nằm trong `IAsset` (`TextureAsset` / `MeshAsset`), chỉ cấp phát đúng **1 lần** (~10MB).
  - **Extrinsic State** (Ngoại tại, biến đổi): Vị trí X, Y, vận tốc, góc quay trong từng `GameObject` (~40 bytes/object).
  - Kết quả: $1.000$ quái vật chỉ tốn $\approx 10\text{MB} + 40\text{KB}$, tiết kiệm **99.6%** bộ nhớ.

### 3.3. Proxy Pattern (`AssetProxy`)
- **Động cơ**: Nạp file đồ họa 4K hoặc Mesh lớn từ đĩa/mạng mất 1-2 giây. Nếu nạp đồng bộ (Synchronous), game loop sẽ bị đứng hình (stuttering/lag spike).
- **Giải pháp**:
  - `AssetProxy` implement cùng interface `IAsset`.
  - Khi chưa nạp xong, `render()` sẽ vẽ `placeholderAsset` (ảnh mờ nhẹ hoặc icon loading xoay).
  - `AssetLoadService` dùng worker pool đọc/giải mã CPU; main thread upload theo ngân sách mỗi frame.
  - Khi hoàn thành, con trỏ được tráo đổi (`swap`) trong một thao tác an toàn luồng, game tiếp tục chạy 60 FPS mượt mà.
