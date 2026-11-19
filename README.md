# Interior3D

Đồ án thiết kế và bố trí nội thất tương tác bằng C++17, Qt 6 Widgets và OpenGL 3.3 Core. Cảnh phòng khách sử dụng mô hình dựng bằng hình khối và shader vật liệu. Đơn vị vị trí trong cảnh là mét.

## Cài đặt công cụ

- **Debian/Ubuntu** — Cài `build-essential cmake ninja-build gdb qt6-base-dev libqt6opengl6-dev libgl1-mesa-dri` bằng APT. Ubuntu cần bản có Qt 6.4 trở lên. Cài thêm `xvfb xauth` để chạy bài kiểm thử GUI khi thiếu màn hình.
- **Windows** — Cài MSVC thông qua Visual Studio Build Tools, chọn **Desktop development with C++**. Cài CMake và Qt 6 cho đúng ABI của compiler. Với Qt MinGW, dùng bộ MinGW đi kèm Qt và thêm thư mục compiler/GDB vào PATH. Thêm thư mục `bin` của Qt đã chọn vào PATH để Windows tìm được DLL khi chạy.
- **macOS** — Cài Xcode Command Line Tools và Qt 6 cùng CMake. Dùng cấu hình LLDB với extension CodeLLDB. Ứng dụng yêu cầu context OpenGL Core và tính tương thích thực tế cần được xác minh trên máy đích.

Dự án dùng các lớp vector và ma trận của Qt. Không cần GLM hay Assimp.

## Chạy từ Visual Studio Code

1. Mở thư mục chứa `CMakeLists.txt`. Cài extension **CMake Tools** và **C/C++**.
2. Chạy **CMake: Select a Kit** và chọn compiler phù hợp với Qt. Nếu Qt nằm ngoài các đường dẫn mặc định, đặt biến môi trường `CMAKE_PREFIX_PATH` trỏ đến thư mục kit Qt trước khi mở VS Code, hoặc đặt biến này trong `cmake.configureSettings` của phần cài đặt cá nhân. Chạy **CMake: Configure**.
3. Chạy **CMake: Set Debug Target** và chọn `Interior3D`. Chọn cấu hình Debug. Trong **Run and Debug**, chọn GDB, MSVC hoặc LLDB theo công cụ đã cài.
4. Nhấn **F5** để build và debug. Có thể dùng **CMake: Run Without Debugging** để chạy.

### Dùng dòng lệnh

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/Interior3D
ctest --test-dir build --output-on-failure
```

Với generator nhiều cấu hình, thêm `--config Debug` khi build và `-C Debug` khi chạy CTest. Trên máy Linux không có hiển thị đồ họa, chạy `xvfb-run -a ctest --test-dir build --output-on-failure`.

## Điều khiển

| Thao tác | Điều khiển |
|---|---|
| Chọn | Nhấn trái lên đối tượng hoặc chọn trong danh sách **Bố trí nội thất** |
| Camera 3D | Dùng WASD để dịch chuyển góc nhìn theo mặt sàn, bốn phím mũi tên khi bỏ chọn vật hoặc kéo chuột phải để xoay, kéo chuột giữa để pan, lăn chuột để zoom. Nhấp vào vùng 3D nếu con trỏ đang ở ô nhập liệu |
| Di chuyển | Kéo vật trên mặt sàn bằng công cụ **Chọn** hoặc **Di chuyển**. Trong **Di chuyển**, có thể kéo tay nắm đỏ, xanh lá, xanh dương theo X, Y, Z. Giữ **Shift** và kéo lên/xuống để đổi độ cao. Giữ **Ctrl** khi thả chuột để đặt đáy vật lên mặt đồ nội thất ngay bên dưới |
| Xoay và tỷ lệ | Trong Xem 3D, nhấp vật rồi giữ ←/→ để xoay quanh trục Y, ↑/↓ để xoay quanh trục X. Nhấp phải để bỏ chọn. Có thể nhập vị trí, góc xoay và tỷ lệ XYZ trong bảng thuộc tính rồi nhấn Enter |
| Thêm đồ | Nhấn một mục trong thư viện. Có thể lọc danh mục hoặc tìm theo tên |
| Cảnh nhìn từ trên | Nhấn **Xem 2D (Mặt bằng)**, kéo chuột phải để pan |
| Tham quan | Nhấn **Tham quan (FPS)** để khóa chuột vào vùng 3D. WASD để đi, di chuột để nhìn, **T** để thoát và trả lại con trỏ |
| Dự án | Ctrl+N, Ctrl+O, Ctrl+S. Tệp `.int3d` dùng JSON |
| Hoàn tác và làm lại | Ctrl+Z và phím tắt làm lại mặc định của hệ điều hành |
| Sao chép và xóa | Ctrl+D và Delete khi viewport có focus |
| Xuất ảnh | Ctrl+E, chọn PNG hoặc JPEG |
| Toàn màn hình | F11 |

### Remote TV

Trong phòng khách mặc định, nhấp vào remote trên bàn trà để cầm lên và hướng camera về TV. Nhấn **M** để bật hoặc tắt TV. Khi bật, TV hiện bó hoa và lần lượt hiện từng ký tự của lời chúc **“Chúc thầy Hiển 20/11 vui vẻ”**. Trong chế độ tham quan, nhấn **X** để nhặt remote từ bàn hoặc nhặt lại sau khi thả. Nếu đang cầm remote, nhấn **X** để thả. Không cần hướng tâm ngắm vào remote khi dùng phím **X**.

## Mã nguồn

- `Scene` chứa mô hình dự án, hình học nội thất, camera và đọc/ghi JSON.
- `Viewport` dựng cảnh bằng VAO/VBO và GLSL, xử lý ray picking, biến đổi đối tượng và điều khiển camera.
- `Editor` liên kết giao diện Qt, thư viện, bảng thuộc tính và lịch sử chỉnh sửa.
- `tests/EditorTest.cpp` chạy kiểm thử dữ liệu và tương tác giao diện bằng Qt Test. Xem kết quả đã xác minh trong [TEST_REPORT.md](TEST_REPORT.md).
