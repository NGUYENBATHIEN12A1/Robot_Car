Dự án bao gồm việc thiết kế, lắp ráp và lập trình **2 mô hình xe robot** độc lập với các tính năng chuyên biệt:
1. **Controlled Vehicle (Xe Điều Khiển):** Được trang bị cánh tay robot 6 bậc tự do (6-DOF) có khả năng gắp/thả vật thể và điều khiển từ xa linh hoạt.
2. **Autonomous Vehicle (Xe Tự Hành):** Xe hoạt động hoàn toàn tự động với khả năng bám theo vạch kẻ (line-following), tránh vật cản và vượt bậc thang.

---

## 🏎️ Mô Hình 1: Xe Điều Khiển (Controlled Vehicle)

Mô hình này sử dụng vi điều khiển **ESP32** mạnh mẽ làm bộ não trung tâm, kết hợp module điều khiển động cơ L298N để dẫn động 4 bánh (4WD). Điểm nhấn là cánh tay robot có thể gắp bóng và thả vào mục tiêu thông qua điều khiển không dây (Bluetooth/Web).

### 🔌 Sơ Đồ Mạch (Wiring Diagram)
Hệ thống sử dụng mạch hạ áp LM2596 để phân bổ nguồn điện ổn định từ pin 3x18650 (11.1V) xuống cho ESP32 và dàn động cơ Servo (MG90S, MG996R) của cánh tay robot.

![Sơ đồ mạch xe điều khiển](https://github.com/user-attachments/assets/18e7e84b-5999-459f-a23b-709407de9654)

### 🧠 Lưu Đồ Thuật Toán (Algorithm Flowchart)
Hệ thống kết nối qua cổng Serial/Bluetooth. Khi nhận được tín hiệu điều hướng hoặc lệnh điều chỉnh tốc độ từ thiết bị điều khiển, ESP32 sẽ xử lý và xuất xung PWM tương ứng xuống module L298N để kích hoạt động cơ DC.

![Lưu đồ thuật toán xe điều khiển](https://github.com/user-attachments/assets/e938317a-9afd-4e6f-ae8d-0fc0bf84f144)

---

## 🚙 Mô Hình 2: Xe Tự Hành Dò Line (Autonomous Vehicle)

Được thiết kế để hoạt động hoàn toàn tự động, mô hình này sử dụng vi điều khiển **Arduino UNO R3**. Xe thu thập dữ liệu môi trường thông qua cụm cảm biến hồng ngoại TCRT5000 và cảm biến siêu âm HC-SR04 để đưa ra quyết định di chuyển theo thời gian thực.

### 🔌 Sơ Đồ Mạch (Wiring Diagram)
Hệ thống dẫn động sử dụng 6 động cơ DC (chia làm 2 cụm trái/phải) điều khiển qua L298N, giúp xe có đủ lực kéo (torque) để leo bậc thang. Các cảm biến được đấu nối trực tiếp vào các chân tín hiệu (Analog/Digital) của Arduino.

![Sơ đồ mạch xe dò line](https://github.com/user-attachments/assets/047c8307-8c5a-4258-a098-e94694a4bdd1)

### 🧠 Lưu Đồ Thuật Toán (Algorithm Flowchart)
Thuật toán ưu tiên xử lý vật cản: Xe liên tục đo khoảng cách bằng cảm biến siêu âm. Nếu phát hiện vật cản gần, xe sẽ kích hoạt chuỗi lệnh rẽ hướng (kiểm tra trái/phải). Nếu không có vật cản, xe tiếp tục bám theo vạch chỉ đường (Line Tracking) thông qua cảm biến IR.

![Lưu đồ thuật toán xe dò line](https://github.com/user-attachments/assets/be272d25-6e0c-4993-9198-e726925a3e27)

---

## 🚀 Định Hướng Phát Triển Tương Lai (Future Development)

Trong tương lai, dự án hướng tới việc kết hợp cả hai mô hình vào một hệ thống tác chiến sa bàn đa nhiệm (Multi-task Competition System):
- **Giao tiếp giữa 2 xe:** Sử dụng mạng LAN/MQTT để hai xe trao đổi dữ liệu cho nhau.
- **Phân chia nhiệm vụ:** Xe tự hành đóng vai trò hỗ trợ vận chuyển và dò đường, trong khi Xe điều khiển sẽ thực hiện nhiệm vụ thao tác phức tạp như gắp và bắn bóng vào mục tiêu.

---
