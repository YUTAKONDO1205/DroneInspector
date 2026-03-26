#include "imu_module.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <nuttx/sensors/cxd5602pwbimu.h>

#include "../../core/config.h"
#include "../../core/utils.h"

// ============================================================
// Spresense Multi-IMU Add-on board (CXD5602PWBIMU1J)
// Arduino-side access via NuttX device driver
//
// Based on the known Arduino-side usage pattern:
//   board_cxd5602pwbimu_initialize(5);
//   open("/dev/imu0", O_RDONLY);
//   ioctl(...) for sample rate / range / fifo / enable
//   poll() + read()
//
// Notes:
// - Driver support for IMU Add-on board is available in the
//   Spresense Arduino environment from v3.4.0.
// - This implementation keeps one "latest sample" and returns it
//   even when no fresh sample arrives in that call.
// ============================================================

extern "C" int board_cxd5602pwbimu_initialize(int bus);

namespace {
  bool g_imuReady = false;
  int g_imuFd = -1;
  ImuSample g_latestSample = {};

  // ----------------------------------------------------------
  // Initial configuration
  // ----------------------------------------------------------
  constexpr int kImuBus = 5;

  // Community Arduino example uses:
  //   rate = 60 Hz
  //   accel range = 4 g
  //   gyro range = 500 dps
  //   fifo threshold = 1
  constexpr int kImuRateHz = 60;
  constexpr int kAccelRangeG = 4;
  constexpr int kGyroRangeDps = 500;
  constexpr int kFifoThreshold = 1;

  // Small timeout keeps loop responsive
  constexpr int kPollTimeoutMs = 5;

  void logImuError(const char* msg, int code = 0) {
#if ENABLE_SERIAL_DEBUG
    Serial.print("[imu] ");
    Serial.print(msg);
    if (code != 0) {
      Serial.print(" code=");
      Serial.print(code);
    }
    Serial.println();
#else
    (void)msg;
    (void)code;
#endif
  }

  bool configureImu(int fd) {
    int ret = 0;

    ret = ioctl(fd, SNIOC_SSAMPRATE, kImuRateHz);
    if (ret) {
      logImuError("SNIOC_SSAMPRATE failed", errno);
      return false;
    }

    cxd5602pwbimu_range_t range;
    range.accel = kAccelRangeG;
    range.gyro  = kGyroRangeDps;

    ret = ioctl(fd, SNIOC_SDRANGE, (unsigned long)(uintptr_t)&range);
    if (ret) {
      logImuError("SNIOC_SDRANGE failed", errno);
      return false;
    }

    ret = ioctl(fd, SNIOC_SFIFOTHRESH, kFifoThreshold);
    if (ret) {
      logImuError("SNIOC_SFIFOTHRESH failed", errno);
      return false;
    }

    ret = ioctl(fd, SNIOC_ENABLE, 1);
    if (ret) {
      logImuError("SNIOC_ENABLE failed", errno);
      return false;
    }

    return true;
  }
}

void initImu() {
  if (g_imuReady) {
    return;
  }

  int ret = board_cxd5602pwbimu_initialize(kImuBus);
  if (ret < 0) {
    logImuError("board_cxd5602pwbimu_initialize failed", ret);
    g_imuReady = false;
    return;
  }

  g_imuFd = open("/dev/imu0", O_RDONLY);
  if (g_imuFd < 0) {
    logImuError("open /dev/imu0 failed", errno);
    g_imuReady = false;
    return;
  }

  if (!configureImu(g_imuFd)) {
    close(g_imuFd);
    g_imuFd = -1;
    g_imuReady = false;
    return;
  }

  // Initialize with a stable default sample
  g_latestSample = {};
  g_latestSample.az = 1.0f;
  g_latestSample.acc_norm = calcNorm3(
    g_latestSample.ax,
    g_latestSample.ay,
    g_latestSample.az
  );
  g_latestSample.gyro_norm = calcNorm3(
    g_latestSample.gx,
    g_latestSample.gy,
    g_latestSample.gz
  );
  g_latestSample.ts_ms = millis();

#if ENABLE_SERIAL_DEBUG
  Serial.println("[imu] ready");
  Serial.print("[imu] rate=");
  Serial.print(kImuRateHz);
  Serial.print("Hz accel=");
  Serial.print(kAccelRangeG);
  Serial.print("g gyro=");
  Serial.print(kGyroRangeDps);
  Serial.println("dps");
#endif

  g_imuReady = true;
}

bool isImuReady() {
  return g_imuReady;
}

ImuSample readImuSample() {
  if (!g_imuReady || g_imuFd < 0) {
    return g_latestSample;
  }

  struct pollfd fds[1];
  fds[0].fd = g_imuFd;
  fds[0].events = POLLIN;

  int pret = poll(fds, 1, kPollTimeoutMs);
  if (pret <= 0) {
    // No fresh data right now -> return latest known sample
    return g_latestSample;
  }

  cxd5602pwbimu_data_t data;
  int rret = read(g_imuFd, &data, sizeof(data));
  if (rret != static_cast<int>(sizeof(data))) {
    logImuError("read imu failed", errno);
    return g_latestSample;
  }

  ImuSample s = {};
  s.ts_ms = millis();

  s.ax = data.ax;
  s.ay = data.ay;
  s.az = data.az;

  s.gx = data.gx;
  s.gy = data.gy;
  s.gz = data.gz;

  s.acc_norm = calcNorm3(s.ax, s.ay, s.az);
  s.gyro_norm = calcNorm3(s.gx, s.gy, s.gz);

  g_latestSample = s;

#if ENABLE_SERIAL_DEBUG
  // コメントアウトを外すと毎回出るので重いときはOFF推奨
  /*
  Serial.print("[imu] ax=");
  Serial.print(s.ax, 6);
  Serial.print(" ay=");
  Serial.print(s.ay, 6);
  Serial.print(" az=");
  Serial.print(s.az, 6);
  Serial.print(" gx=");
  Serial.print(s.gx, 6);
  Serial.print(" gy=");
  Serial.print(s.gy, 6);
  Serial.print(" gz=");
  Serial.println(s.gz, 6);
  */
#endif

  return g_latestSample;
}

void endImu() {
  if (g_imuFd >= 0) {
    // Disable sensor before close
    ioctl(g_imuFd, SNIOC_ENABLE, 0);
    close(g_imuFd);
    g_imuFd = -1;
  }

  g_imuReady = false;

#if ENABLE_SERIAL_DEBUG
  Serial.println("[imu] ended");
#endif
}
