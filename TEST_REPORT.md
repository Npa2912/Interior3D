# Báo cáo kiểm thử Interior3D

Kiểm tra ngày 19/11/2026 trên Debian 13, GCC 14.2, CMake 3.31, Qt 6.8.2, Xvfb và Mesa llvmpipe. Bản build Debug dùng Ninja. Context trên máy thử hỗ trợ OpenGL Core 4.5 và ứng dụng yêu cầu từ 3.3.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
xvfb-run -a -s '-screen 0 1680x960x24 +extension GLX' ctest --test-dir build --output-on-failure
```

CTest chạy **2/2 mục thành công**. Bốn hàm Qt Test thực thi gồm `sceneRoundTrip`, `viewportWorkflow`, `editorWorkflow` và `projectActions`. Ứng dụng độc lập đã mở cửa sổ 1600 × 900 trên Xvfb. GDB đã dừng tại `main` của bản Debug.

| Hạng mục | Trạng thái | Bằng chứng |
|---|---|---|
| CMake và build | PASS | Configure và link ứng dụng cùng bộ kiểm thử thành công |
| 1. Mở ứng dụng | PASS | Cửa sổ xuất hiện, giữ tiến trình trong lần chạy độc lập. Qt Test tạo được cửa sổ và context |
| 2. Giao diện đúng ảnh mẫu | FAIL | Đã đối chiếu ảnh chụp 1600 × 900. Bố cục, tông màu và các panel theo mẫu. Cảnh dựng hình khối, icon, font, khoảng cách và chi tiết nội thất vẫn khác rõ rệt. Mốc 95% chưa đạt |
| 3. Cảnh OpenGL | PASS | Shader hoạt động, framebuffer có nội dung và thay đổi theo cảnh |
| 4. Xoay camera, pan | PASS | Sự kiện kéo chuột thay đổi góc nhìn và tâm camera |
| 5. Zoom | PASS | Sự kiện con lăn thay đổi khoảng cách |
| 6. Chọn nội thất | PASS | Ray picking và nhấp chuột ở mặt bằng trả về đúng ID bàn |
| 7. Di chuyển | PASS | Kéo trên mặt bằng và nhập X thay đổi tọa độ |
| 8. Xoay đối tượng | PASS | Kéo làm đổi Y. Nhập số đặt đúng góc 42° |
| 9. Tỷ lệ | PASS | Kéo giữ tỷ lệ và nhập tỷ lệ riêng theo Z |
| 10. Thêm nội thất | PASS | Tìm cây trong thư viện, nhấp mục và kiểm tra đối tượng mới |
| 11. Xóa và khóa | PASS | Khóa vô hiệu hóa lệnh xóa. Bỏ khóa và nhấn Delete xóa được bản sao |
| 12. Sao chép | PASS | Ctrl+D thêm một đối tượng có ID riêng |
| 13. Đổi màu | PASS | Mở hộp thoại màu, đặt màu và xác nhận framebuffer thay đổi |
| 14. Đổi vật liệu | PASS | Chọn gỗ, kiểm tra thuộc tính và framebuffer thay đổi |
| 15. Ánh sáng | PASS | Đổi cường độ môi trường trong hộp thoại và xác nhận ảnh thay đổi. Test viewport cũng xác nhận ảnh khác sau khi giảm ba cường độ xuống 0 |
| 16. Mặt bằng 2D và phối cảnh | PASS | Đổi chế độ, kiểm tra trạng thái camera và chọn vật trong mặt bằng |
| 17. Tham quan FPS | PASS | W di chuyển, chuột đổi hướng, camera giữ cao độ, Esc về phối cảnh |
| 18. Hoàn tác và làm lại | PASS | Khôi phục tọa độ qua cả hai chiều và hoàn tác xóa |
| 19. Lưu | PASS | Ghi JSON trong thư mục tạm |
| 20. Mở | PASS | Khôi phục tọa độ, tỷ lệ, góc, màu, vật liệu, khóa, ánh sáng và camera. Tệp sai định dạng bị từ chối |
| 21. Render PNG và JPEG | PASS | Xuất framebuffer rồi đọc lại cả hai định dạng bằng QImage |
| 22. Resize | PASS | Hạ cửa sổ còn 1020 × 700, kiểm tra viewport giữ kích thước tối thiểu và còn dựng hình |
| 23. Lệnh và hộp thoại UI | PASS | Đã mở và xác nhận các hộp thoại tệp. Đã mở xem trước, cài đặt, trợ giúp, trang chủ, danh sách đối tượng, camera, render và quản lý dự án |
| Mọi tổ hợp điều khiển trong các panel | NOT TESTED | Chưa thao tác từng nút con trong từng hộp thoại và từng biến thể khóa, vật liệu và tay nắm gizmo |
| 24. Phím tắt | PASS | Thực thi Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Z, Ctrl+D, Ctrl+E, Delete, F11, Esc và W. Làm lại được kiểm tra bằng nút toolbar |
| Mục tiêu 60 FPS trên GPU | BLOCKED | Xvfb dùng kết xuất phần mềm. Chỉ báo đọc 39 FPS ở lần chụp bằng Qt Test, chưa có kết quả trên GPU thật |
| F5 trực tiếp trong VS Code | BLOCKED | Máy kiểm thử không có VS Code. Đã xác nhận debug binary bằng GDB và kiểm tra cú pháp JSON cấu hình |

## Bổ sung remote TV

Kiểm thử `remoteGreeting` chạy trên Xvfb và Mesa llvmpipe. CTest đạt **2/2** sau khi bổ sung. Bài kiểm thử nhấp remote trên bàn, so sánh framebuffer trước và sau khi bật TV bằng M, kiểm tra chữ thay đổi theo thời gian, bật/tắt bằng M, thả bằng X và nhặt lại sau khi remote rơi.

## Bổ sung kiểm thử chế độ tham quan

- PASS trên Qt 6/Xvfb. Chế độ tham quan giữ chuột, đưa con trỏ về tâm sau khi xoay và trả con trỏ khi nhấn **T**. Esc và Tab không thoát khỏi chế độ tham quan.
- PASS trên Qt 6/Xvfb. Camera dừng khi nhả W, bỏ qua sự kiện lặp của phím đã nhả và không tự đi tiếp khi chuyển sang cửa sổ khác rồi quay lại.
- PASS trên Qt 6/Xvfb. Cảnh tĩnh ở mặt bằng phát sinh tối đa một lần đổi khung hình trong 220 ms. Các bài kiểm thử tương tác remote và hoạt ảnh TV vẫn chạy qua.
- NOT TESTED. Giới hạn con trỏ native qua Win32 và độ mượt trên Windows chưa được kiểm tra trên máy Windows.
