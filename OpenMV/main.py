# OpenMV 主程序
# 2026 电赛 H 题 —— 钢球位置检测
# 该版本的OpenMV代码在比赛时由队友负责

import sensor, image, time, math, gc
from machine import UART

# ====== 初始化摄像头 ======
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QVGA)

sensor.set_auto_gain(True)
sensor.set_auto_whitebal(False)
sensor.set_auto_exposure(True)
sensor.skip_frames(time=500)
sensor.set_auto_gain(False)
sensor.set_auto_exposure(False)

uart = UART(3, baudrate=115200, bits=8, parity=None, stop=1)


# ====== 目标区域与物理标定 ======
ROI      = (162, 0, 45, 240)
height   = 240
center_y = height / 2
length_mm = 162
pixel_mm = length_mm / height
MAX_POS_MM = 75

LEFT_LIMIT  = 165
RIGHT_LIMIT = 225

Y_MIN = 15
Y_MAX = 225

# 圆心稳定性校验阈值（像素）
PRED_GATE_PX_NORMAL  = 60   # 正常跟踪时
PRED_GATE_PX_RECOVER = 120  # 丢失恢复时（更宽松，但仍校验）


# ====== 卡尔曼滤波器 ======
class KalmanFilter1D:
    def __init__(self, R_meas=3.0, Q_pos=0.3, Q_vel=2.0):
        self.R = R_meas
        self.Q_pos = Q_pos
        self.Q_vel = Q_vel
        self.x_pos = 0.0
        self.x_vel = 0.0
        self.p11 = 100.0
        self.p12 = 0.0
        self.p21 = 0.0
        self.p22 = 100.0
        self.initialized = False
        self.last_time_us = 0

    def reset(self, init_pos):
        self.x_pos = init_pos
        self.x_vel = 0.0
        self.p11 = 100.0
        self.p12 = 0.0
        self.p21 = 0.0
        self.p22 = 100.0
        self.initialized = True
        self.last_time_us = time.ticks_us()

    def soft_reset(self, init_pos):
        self.x_pos = init_pos
        self.p11 = 50.0
        self.p12 = 0.0
        self.p21 = 0.0
        self.last_time_us = time.ticks_us()

    def update(self, z_meas, residual_thresh=40.0):
        now_us = time.ticks_us()
        dt = time.ticks_diff(now_us, self.last_time_us) / 1_000_000.0
        self.last_time_us = now_us
        if dt <= 0.0 or dt > 0.2:
            dt = 0.017

        pos_pred = self.x_pos + self.x_vel * dt
        vel_pred = self.x_vel

        p11_pred = self.p11 + 2 * dt * self.p12 + dt * dt * self.p22 + self.Q_pos
        p12_pred = self.p12 + dt * self.p22
        p21_pred = p12_pred
        p22_pred = self.p22 + self.Q_vel

        y = z_meas - pos_pred

        # 【新增】自适应残差阈值：卡尔曼位置越接近极限，阈值越紧
        # 原理：球在75mm处丢失后，40mm处的虚假圆残差=35mm，
        #       原固定阈值40会放行。改为自适应后阈值收紧到~22mm，直接拒绝。
        adaptive_thresh = residual_thresh
        if abs(pos_pred) > MAX_POS_MM * 0.7:
            # 位置在52.5mm以上时，阈值随位置收紧
            adaptive_thresh = max(15.0, residual_thresh * (1.0 - abs(pos_pred) / MAX_POS_MM * 0.6))

        if abs(y) > adaptive_thresh:
            self.x_pos = pos_pred
            self.p11 = p11_pred
            self.p12 = p12_pred
            self.p21 = p21_pred
            self.p22 = p22_pred
            return self.x_pos

        S = p11_pred + self.R
        K1 = p11_pred / S
        K2 = p21_pred / S

        self.x_pos = pos_pred + K1 * y
        self.x_vel = vel_pred + K2 * y

        MAX_VEL = 300.0
        if self.x_vel > MAX_VEL:
            self.x_vel = MAX_VEL
        elif self.x_vel < -MAX_VEL:
            self.x_vel = -MAX_VEL

        self.p11 = (1 - K1) * p11_pred
        self.p12 = (1 - K1) * p12_pred
        self.p21 = p21_pred - K2 * p11_pred
        self.p22 = p22_pred - K2 * p21_pred
        return self.x_pos

    def predict_only(self, decay=0.97):
        now_us = time.ticks_us()
        dt = time.ticks_diff(now_us, self.last_time_us) / 1_000_000.0
        self.last_time_us = now_us
        if dt <= 0.0 or dt > 0.2:
            dt = 0.017

        self.x_pos = self.x_pos + self.x_vel * dt
        self.x_vel *= decay

        # 不钳位，由调用者判断超限
        self.p11 = self.p11 + 2 * dt * self.p12 + dt * dt * self.p22 + self.Q_pos
        self.p12 = self.p12 + dt * self.p22
        self.p21 = self.p12
        self.p22 = self.p22 + self.Q_vel
        return self.x_pos


kf = KalmanFilter1D(R_meas=3.0, Q_pos=0.3, Q_vel=2.0)

filtered_mm = 0.0
first_detect = True
send_counter = 0

# ====== 丢失处理状态 ======
MAX_SHORT_LOST = 100
MAX_MID_LOST   = 300

lost_count = 0
last_valid_mm = None

# ====== 方向状态机（仅辅助）======
direction_sign = None
direction_candidate = None
direction_candidate_cnt = 0
DIR_FLIP_THRESH = 3

# ====== 冻结方向 & 死区 ======
frozen_direction = None
DEADZONE_MM = 2.0
last_filtered_sign = None

# 【新增】溢出标志：记录是否因超出极限而进入丢失
overflow_lost = False

# ====== 霍夫圆参数 ======
CIRCLE_THRESHOLD = 2150
R_MIN = 7
R_MAX = 8
R_STEP = 2
X_MARGIN = 15
Y_MARGIN = 15
R_MARGIN = 15

clock = time.clock()


# ============================ 主循环 ============================
while True:
    clock.tick()
    send_counter += 1
    img = sensor.snapshot()

    if send_counter % 30 == 0:
        gc.collect()

    img.draw_rectangle(ROI, color=(255, 0, 0))
    img.gaussian(1)
    img.gamma(gamma=1.0, contrast=1.15)

    circles = img.find_circles(
        roi=ROI,
        threshold=CIRCLE_THRESHOLD,
        x_margin=X_MARGIN,
        y_margin=Y_MARGIN,
        r_margin=R_MARGIN,
        r_min=R_MIN,
        r_max=R_MAX,
        r_step=R_STEP
    )

    best_circle = None
    if circles:
        best_mag = 0
        for c in circles:
            cx, cy, cr = c.x(), c.y(), c.r()
            mag = c.magnitude()
            if LEFT_LIMIT <= cx <= RIGHT_LIMIT and Y_MIN <= cy <= Y_MAX:
                if mag > best_mag:
                    best_mag = mag
                    best_circle = (cx, cy, cr)

    # ====== 【修复B】圆心稳定性校验 —— 始终生效 ======
    # 原Bug：lost_count == 0 条件导致溢出后第1帧 PRED_GATE 被绕过
    # 修复：移除 lost_count == 0 条件，始终校验，但恢复时使用更宽松的阈值
    if best_circle is not None and kf.initialized:
        cx, cy, cr = best_circle
        pred_y = center_y + (kf.x_pos / pixel_mm)
        gate = PRED_GATE_PX_RECOVER if lost_count > 0 else PRED_GATE_PX_NORMAL
        if abs(cy - pred_y) > gate:
            best_circle = None  # 拒绝虚假检测

    # ====== 【新增】溢出恢复校验 ======
    # 当因溢出进入丢失后，卡尔曼位置在极端值（>75mm）
    # 此时若检测到圆心对应位置远离极端值，判为虚假检测
    if best_circle is not None and overflow_lost:
        cx, cy, cr = best_circle
        offset_mm_temp = (cy - center_y) * pixel_mm
        # 卡尔曼在极端位置，但检测到的圆在中段 → 虚假
        if abs(offset_mm_temp) < MAX_POS_MM * 0.6 and abs(kf.x_pos) > MAX_POS_MM * 0.8:
            best_circle = None

    # ====== 有有效圆 ======
    if best_circle is not None:
        was_lost = (lost_count > 0)
        prev_lost_count = lost_count

        lost_count = 0
        overflow_lost = False  # 清除溢出标志
        cx, cy, cr = best_circle
        img.draw_circle((cx, cy, cr), color=(0, 255, 0))
        offset_mm = (cy - center_y) * pixel_mm

        # ====== 方向判定（辅助用途）======
        meas_sign = '+' if offset_mm >= 0 else '-'

        if abs(offset_mm) >= DEADZONE_MM:
            if direction_candidate == meas_sign:
                direction_candidate_cnt += 1
            else:
                direction_candidate = meas_sign
                direction_candidate_cnt = 1
            if direction_candidate_cnt >= DIR_FLIP_THRESH:
                direction_sign = direction_candidate

        if was_lost:
            if abs(offset_mm) >= DEADZONE_MM:
                direction_sign = meas_sign
                direction_candidate = meas_sign
                direction_candidate_cnt = DIR_FLIP_THRESH

        # ====== 卡尔曼滤波与恢复策略 ======
        if first_detect:
            kf.reset(offset_mm)
            filtered_mm = offset_mm
            first_detect = False
        elif was_lost and prev_lost_count > MAX_SHORT_LOST:
            kf.soft_reset(offset_mm)
            filtered_mm = offset_mm
        else:
            filtered_mm = kf.update(offset_mm)

        # ====== 超出物理极限 → 输出对应符号的 lost ======
        if abs(filtered_mm) >= MAX_POS_MM:
            sign = '+' if filtered_mm > 0 else '-'
            msg = "X{}LOST\r\n".format(sign)
            uart.write(msg)
            print(msg)

            lost_count = 1
            overflow_lost = True   # 【新增】标记溢出丢失
            frozen_direction = sign
            last_filtered_sign = sign
            continue

        # 正常输出
        last_valid_mm = filtered_mm
        last_filtered_sign = '+' if filtered_mm >= 0 else '-'
        frozen_direction = None

        sign = '+' if filtered_mm >= 0 else '-'
        int_mm = int(round(abs(filtered_mm)))
        if int_mm > 9999:
            int_mm = 76
        msg = "X{}{:04d}\r\n".format(sign, int_mm)
        uart.write(msg)
        #print(msg)

    # ====== 丢失小球 ======
    # 【修复A】删除全部重复的 Phase 1/2/3 代码块，只保留一套
    else:
        lost_count += 1

        # ====== 丢失第1帧：冻结方向 ======
        if lost_count == 1:
            if last_filtered_sign is not None:
                frozen_direction = last_filtered_sign
            elif direction_sign is not None:
                frozen_direction = direction_sign
            else:
                frozen_direction = '+'

        # ====== 阶段1：短期丢失（1 ~ MAX_SHORT_LOST 帧）======
        if last_valid_mm is not None and lost_count <= MAX_SHORT_LOST:
            raw_pred = kf.predict_only()

            # 预测超出物理极限时，输出对应符号的 lost
            if abs(raw_pred) > MAX_POS_MM:
                sign = '+' if raw_pred > 0 else '-'
                msg = "X{}LOST\r\n".format(sign)
                uart.write(msg)
                #print(msg)
            else:
                sign = frozen_direction if frozen_direction is not None else '+'
                abs_mm = abs(raw_pred)
                int_mm = int(round(abs_mm))
                if int_mm > 9999:
                    int_mm = 76
                msg = "X{}{:04d}LOST\r\n".format(sign, int_mm)
                uart.write(msg)
                #print(msg)

            last_valid_mm = raw_pred

        # ====== 阶段2：中期丢失（MAX_SHORT_LOST+1 ~ MAX_MID_LOST 帧）======
        elif last_valid_mm is not None and lost_count <= MAX_MID_LOST:
            raw_pred = kf.predict_only()

            if abs(raw_pred) > MAX_POS_MM:
                sign = '+' if raw_pred > 0 else '-'
                msg = "X{}LOST\r\n".format(sign)
            else:
                sign = frozen_direction if frozen_direction is not None else '+'
                msg = "X{}LOST\r\n".format(sign)
            uart.write(msg)
            #print(msg)

        # ====== 阶段3：彻底丢失（> MAX_MID_LOST 帧）======
        else:
            uart.write("lost\r\n")
            print("lost")
            if last_valid_mm is not None:
                last_valid_mm = None
                direction_sign = None
                direction_candidate = None
                direction_candidate_cnt = 0
                frozen_direction = None
                last_filtered_sign = None
                first_detect = True
                overflow_lost = False
            if lost_count > 200:
                lost_count = MAX_MID_LOST + 1
