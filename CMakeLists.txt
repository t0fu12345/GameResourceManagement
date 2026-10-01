cmake_minimum_required(VERSION 3.15)
project(GameAssetPipeline VERSION 1.0.0 DESCRIPTION "Cross-platform Game Asset Pipeline")

# Thiết lập chuẩn C++17 bắt buộc
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Cấu hình cờ biên dịch khắt khe (Clean code & Memory safety)
if(MSVC)
    add_compile_options(/W4 /WX) # Cảnh báo ở mức cao nhất, biến cảnh báo thành lỗi
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Chỉ định thư mục chứa file Header (.h, .hpp)
include_directories(include)

# Tạm thời gom các file mã nguồn. Các class cụ thể sẽ được thêm ở các bước sau.
# Tạo file src/main.cpp rỗng trước để CMake không báo lỗi.
file(WRITE ${CMAKE_SOURCE_DIR}/src/main.cpp "int main() { return 0; }")

# Khai báo file thực thi
add_executable(${PROJECT_NAME} 
    src/main.cpp
)
