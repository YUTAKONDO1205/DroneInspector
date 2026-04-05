# DroneInspector

開発者: 近藤悠太 (Kondo Yuta)

狭小空間インフラを対象とした、**Sony Spresense ベースのドローン搭載点検支援システム**です。  
本システムは、カメラ画像を機体側で処理して損傷候補を一次検出し、必要な情報のみを **microSD に保存**、**BLE1507（NUS firmware）経由で通知**することを目的としています。  
また、**SpresenseマルチIMU Add-onボード（CXD5602PWBIMU1J）** を用いて、画像取得時の機体状態も記録します。

---

## 目的

本プロジェクトの目的は、トンネルや水道管などの狭小空間インフラにおいて、

- 危険空間に人が直接入る負担を減らす
- 機体側で一次判断を行う
- 必要な情報だけを保存・通知する
- AIと人間の実務分担を明確にする

ことです。

本システムでは、AIが損傷候補の一次検出を担い、人間が最終確認と判断を担う構成を目指しています。

---

## 使用ハードウェア

- **Sony Spresense**
- **Spresense HDRカメラボード**
- **SpresenseマルチIMU Add-onボード**
  - 型番: `CXD5602PWBIMU1J`
- **BLE1507**
  - NUS firmware 使用
- **microSDカード**

---

## 使用技術

- Arduino
- TensorFlow Lite for Microcontrollers
- BLE UART通信（BLE1507 NUS firmware）
- microSD logging
- IMU取得
- 画像分類モデル（TFLite）

---

## ディレクトリ構成

```text
DroneInspector/
├─ app/
│  ├─ App.cpp
│  └─ App.h
├─ models/
│  ├─ model_data.cpp
│  └─ model_data.h
├─ src/
│  ├─ core/
│  │  ├─ config.h
│  │  ├─ pins.h
│  │  ├─ types.h
│  │  ├─ utils.cpp
│  │  └─ utils.h
│  └─ modules/
│     ├─ ble/
│     │  ├─ ble_module.cpp
│     │  └─ ble_module.h
│     ├─ camera/
│     │  ├─ camera_module.cpp
│     │  └─ camera_module.h
│     ├─ classifier/
│     │  ├─ classifier_module.cpp
│     │  └─ classifier_module.h
│     ├─ imu/
│     │  ├─ imu_module.cpp
│     │  └─ imu_module.h
│     ├─ logger/
│     │  ├─ logger_module.cpp
│     │  └─ logger_module.h
│     └─ storage/
│        ├─ storage_module.cpp
│        └─ storage_module.h
├─ .gitignore
├─ crack_classifier_mobilenetv2.tflite
├─ DroneInspector.ino
├─ model_create.py
├─ README.md
└─ uav_model.py
