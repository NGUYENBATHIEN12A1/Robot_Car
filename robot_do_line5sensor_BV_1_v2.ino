
//=======================================================
#include <SoftwareSerial.h>
SoftwareSerial BT(10, 11);  // RX = 10, TX = 11
#include <NewPing.h>        //Ultrasonic sensor function library. You must install this library
#define trig_pin 13         //analog input 1
#define echo_pin 12         //analog input 2
#define maximum_distance 200
boolean goesForward = false;
int distance = 100;
NewPing sonar(trig_pin, echo_pin, maximum_distance);  //sensor function
//=======================================================
#include <PID_v1.h>
uint8_t speed_robot = 180;  // set (tốc độ của robot) speed_robot (pwm value), 0 < speed_robot < 256
int8_t check_out = 0;
double Setpoint = 0, Input, Output;
uint8_t flag_zero = 0;
// Tăng Kp để tăng phản ứng với lỗi line lớn
// double Kp = 35, Ki = 0.04555555, Kd = 11.898989;
double Kp = 40, Ki = 0.04555555, Kd = 11.898989;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
int sensor;
#define Line_0 A0  //14
#define Line_1 A1  //15
#define Line_2 A2  //16
#define Line_3 A3  //17
#define Line_4 A4  //17
byte ss0, ss1, ss2, ss3, ss4;
double error = 0;
int initial_motor_speed = 150, PID_value;
double P, I, D, previous_error;
//=======================================================
int step_quay_truc = 0;
int step_nang_duoi = 0;
int step_nang_tren = 0;
const int relay = 13;
const int motorA1 = 3;  // Pin  6 of L298.
const int motorA2 = 5;  // Pin  7 of L298.
const int motorB1 = 6;  // Pin 9 of L298.
const int motorB2 = 9;  // Pin 10 of L298.

const int buzzer = 4;

const int led = 13;

int i = 0;
int j = 0;
int state_rec;
int vSpeed = 200;
char state;
int DOF_C = 0;
int mode_Run = 0;


void setup() {
  // BT.begin(9600);  // Bắt đầu giao tiếp Bluetooth
  // BT.println("Bluetooth Ready");
  //---------------------------
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  analogWrite(motorB1, 0);
  analogWrite(motorA1, 0);
  analogWrite(motorA2, 0);
  analogWrite(motorB2, 0);
  //--------------------------
  pinMode(Line_0, INPUT);  //Set chân cảm biến 1 là input
  pinMode(Line_1, INPUT);  //Set chân cảm biến 2 là input
  pinMode(Line_2, INPUT);  //Set chân cảm biến 3 là input
  pinMode(Line_3, INPUT);  //Set chân cảm biến 4 là input
  pinMode(Line_4, INPUT);  //Set chân cảm biến 4 là input
  myPID.SetSampleTime(1);  // thời gian lấy mẫu phụ thuộc tốc độ xe, lấy mẫu càng nhanh càng tốt
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-speed_robot, speed_robot);  // giá trị tốc độ, -speed tức bánh bên trái quay max, bên phải ngừng quay
  //--------------------------
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(9600);
  Serial.println("OK");
  vSpeed = 255;
  previous_error = 0;
  error = 0;
}

enum ClimbState { IDLE,
                  APPROACH,
                  PAUSE,
                  CLIMB,
                  FINISHED,
                  TAMDUNG,
                  CHOKHOIDONG,
                  KHOIDONGLAI,
};
ClimbState climbState = IDLE;
unsigned long stateStart = 0;
const unsigned long APPROACH_TIME = 600;  // ms
const unsigned long PAUSE_TIME = 500;     // ms
const unsigned long CLIMB_TIME = 600;     // ms
const int STEP_THRESHOLD = 6;             // cm
const int CLIMB_SPEED = 160;
const int SLOW_SPEED = 80;
int prevDist;                      // khoảng cách đo trước khi bắt đầu leo
const int SUCCESS_THRESHOLD = 15;  // cm: khoảng cách phải tăng lên để tính là “leo qua”
int solandaleo = 0;
const unsigned long PULL_TIME = 800;  // ms kéo chậm ổn định trước khi quay về line
const int PULL_SPEED = 150;
bool hasStopped = false;
const unsigned long TAMDUNG_TIME = 1000;
const unsigned long CHOKHOIDONG_TIME = 1000;
const unsigned long KHOIDONGLAI_TIME = 600;
int solantamdung = 0;


void loop() {

  if (hasStopped) return;



  // 1) Dò siêu âm
  int dist = getDistance();


  // Chuyển sang bắt đầu leo khi đủ điều kiện
  if (climbState == IDLE && dist < STEP_THRESHOLD) {
    prevDist = dist;  // ghi nhớ khoảng cách ban đầu
    climbState = APPROACH;
    stateStart = millis();
    // khởi động chạy nhẹ
    setMotorA(SLOW_SPEED);
    setMotorB(SLOW_SPEED);
  }

  unsigned long now = millis();

  switch (climbState) {
    case APPROACH:
      if (now - stateStart >= APPROACH_TIME) {
        stopMotors();
        climbState = PAUSE;
        stateStart = now;
      }
      break;

    case PAUSE:
      if (now - stateStart >= PAUSE_TIME) {
        climbState = CLIMB;
        stateStart = now;
        // tung sức leo
        setMotorA(CLIMB_SPEED);
        setMotorB(CLIMB_SPEED);
      }
      break;

    case CLIMB:
      if (now - stateStart >= CLIMB_TIME) {
        stopMotors();
        int distAfter = getDistance();
        // Nếu chưa leo qua bậc (tăng chưa đủ), thử lại
        if (distAfter < prevDist + SUCCESS_THRESHOLD) {
          // đặt lại thời gian và quay về APPROACH để lấy đà lần nữa
          climbState = APPROACH;
          stateStart = now;
          setMotorA(SLOW_SPEED);
          setMotorB(SLOW_SPEED);
        } else {
          // Leo thành công
          solandaleo++;
          prevDist = distAfter;  // cập nhật khoảng cách cơ sở
          climbState = FINISHED;
          stateStart = now;
          setMotorA(PULL_SPEED);
          setMotorB(PULL_SPEED);
        }
      }
      break;


    case FINISHED:
      // 1) Nếu chưa leo đủ 3 bậc, chuyển về dò line + tiến tiếp
      if (now - stateStart >= PULL_TIME) {
        do_line();
        Input = error;
        myPID.Compute();
        motorControl(Output);
        climbState = IDLE;
      }
      break;
    case TAMDUNG:
      // int kc = getDistance();
      Serial.println(dist);  // In ra để kiểm tra
      if (now - stateStart >= TAMDUNG_TIME && dist <= 15) {
        stopMotors();
        climbState = CHOKHOIDONG;
        stateStart = now;
      }
      break;
    case CHOKHOIDONG:
      if (now - stateStart >= CHOKHOIDONG_TIME) {
        setMotorA(SLOW_SPEED);
        setMotorB(SLOW_SPEED);
        climbState = KHOIDONGLAI;
        stateStart = now;
        error = -1;
      }
      break;
    case KHOIDONGLAI:
      if (now - stateStart >= KHOIDONGLAI_TIME) {
        do_line();
        if (error != -1) {
          Input = error;
          myPID.Compute();
          motorControl(Output);
          climbState = IDLE;
          stateStart = now;
        }
      }
      break;
    case IDLE:
      // đang dò line trước khi leo
      if (solandaleo >= 0) {
        // đọc line
        ss0 = digitalRead(Line_0);
        ss1 = digitalRead(Line_1);
        ss2 = digitalRead(Line_2);
        ss3 = digitalRead(Line_3);
        ss4 = digitalRead(Line_4);
        // nếu toàn 5 line == 0 thì stop
        if (ss0 == 0 && ss1 == 0 && ss2 == 0 && ss3 == 0 && ss4 == 0) {
          if (solantamdung > 0) {
            stopRobot();
          } else {
            setMotorA(0);
            setMotorB(0);
            climbState = TAMDUNG;
            stateStart = now;
            solantamdung += 1;
            Serial.println("Dung robot");  // In ra để kiểm tra
          }
        } else {
          do_line();
          Input = error;
          myPID.Compute();
          motorControl(Output);
        }
      } else {
        do_line();
        Input = error;
        myPID.Compute();
        motorControl(Output);
      }
      break;
    default:
      stopRobot();
      break;
  }

  //  Setpoint = 0;
  //   do_line();
  //   Input = error;
  //   myPID.Compute();
  //   motorControl(Output);
}


void stopMotors() {
  setMotorA(0);
  setMotorB(0);
}

// Hàm đọc siêu âm (trả về cm)
int getDistance() {
  delay(1);
  int cm = sonar.ping_cm();
  return cm == 0 ? 100 : cm;
}

void stopRobot() {
  // Dừng hoàn toàn hai động cơ
  setMotorA(0);
  setMotorB(0);
  hasStopped = true;
  // Nếu muốn, có thể khóa chương trình luôn:
  while (true) {
    // chờ vô hạn, robot không làm gì thêm
  }
}




////////////////////////////////////////////////////////////////////////
void do_line(void) {
  ss0 = digitalRead(Line_0);
  ss1 = digitalRead(Line_1);
  ss2 = digitalRead(Line_2);
  ss3 = digitalRead(Line_3);
  ss4 = digitalRead(Line_4);
  if ((ss0 == 0) && (ss1 == 0) && (ss2 == 0) && (ss3 == 0) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 0) && (ss1 == 1) && (ss2 == 0) && (ss3 == 1) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 0) && (ss1 == 1) && (ss2 == 1) && (ss3 == 1) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 0) && (ss1 == 0) && (ss2 == 1) && (ss3 == 0) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 0) && (ss1 == 0) && (ss2 == 1) && (ss3 == 1) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 0) && (ss1 == 0) && (ss2 == 0) && (ss3 == 1) && (ss4 == 1)) {
    error = 4;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 1) && (ss3 == 1) && (ss4 == 1)) {
    error = 3;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 0) && (ss3 == 1) && (ss4 == 1)) {
    error = 0.2;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 0) && (ss3 == 1) && (ss4 == 1)) {
    if (error != 0) { previous_error = error; }
    error = 0;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 1) && (ss3 == 0) && (ss4 == 1)) {
    if (error != 0) { previous_error = error; }
    error = 0;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 0) && (ss3 == 0) && (ss4 == 1)) {
    if (error != 0) { previous_error = error; }
    error = 0;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 0) && (ss3 == 0) && (ss4 == 1)) {
    error = -0.2;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 1) && (ss3 == 0) && (ss4 == 1)) {
    error = -3;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 0) && (ss3 == 0) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 0) && (ss3 == 1) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 0) && (ss2 == 1) && (ss3 == 0) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 1) && (ss3 == 0) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 0) && (ss3 == 0) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 1) && (ss3 == 1) && (ss4 == 0)) {
    error = -4;
  } else if ((ss0 == 1) && (ss1 == 1) && (ss2 == 1) && (ss3 == 1) && (ss4 == 1)) {
    if (error == 0) {
      error = previous_error;
    }
    if (error < 0) error = -4;
    else if (error > 0) error = 4;
    else error = 0;
  } else if ((ss0 == 0) && (ss1 == 0) && (ss2 == 0) && (ss3 == 0) && (ss4 == 0)) {

    return;
  }

  // Serial.print(error);
  // Serial.println(" error");
}


void setMotorA(int16_t pwm) {
  pwm = constrain(pwm, -255, 255);  // Giới hạn giá trị trong phạm vi hợp lệ
  if (pwm >= 0) {
    analogWrite(motorA1, pwm);
    analogWrite(motorA2, 0);
  } else {
    analogWrite(motorA1, 0);
    analogWrite(motorA2, -pwm);
  }
}

void setMotorB(int16_t pwm) {
  pwm = constrain(pwm, -255, 255);  // Giới hạn giá trị trong phạm vi hợp lệ
  if (pwm >= 0) {
    analogWrite(motorB1, pwm);
    analogWrite(motorB2, 0);
  } else {
    analogWrite(motorB1, 0);
    analogWrite(motorB2, -pwm);
  }
}


void motorControl(int16_t duty) {
  // int16_t base = speed_robot / 2;  // ~127 khi speed_robot=255
  int16_t base = speed_robot * 0.6;
  int16_t maxDuty = speed_robot;  // ~255
  int16_t d = constrain(duty, -maxDuty, +maxDuty);
  int16_t absd = abs(d);
  int16_t spin_speed = base;

  if (absd <= base) {
    // — differential turn —
    int16_t left = base + d;
    int16_t right = base - d;
    setMotorA(left);
    setMotorB(right);
  } else {
    // — spin tại chỗ, tốc độ spin fade-in từ base→speed_robot —
    float t = float(absd - base) / float(maxDuty - base);
    int16_t spin_speed = base + t * (speed_robot * 0.8 - base);
    if (d > 0) {
      setMotorA(-spin_speed);
      setMotorB(+spin_speed);
    } else {
      setMotorA(+spin_speed);
      setMotorB(-spin_speed);
    }
  }
  // Serial.print("duty=");
  // Serial.print(d);
  // Serial.print("  L=");
  // Serial.print((absd <= base ? base + d : (d > 0 ? -spin_speed : +spin_speed)));
  // Serial.print("  R=");
  // Serial.println((absd <= base ? base - d : (d > 0 ? +spin_speed : -spin_speed)));
}
