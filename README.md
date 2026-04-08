# DroneInspector

開発者: 近藤悠太

DroneInspector は、狭小インフラ空間を対象にした Sony Spresense ベースの点検支援システムです。
機体側で画像取得、IMU 記録、microSD 保存、BLE 通知を行い、軽量なエッジ処理で点検作業を補助します。

## 概要

このリポジトリは、次の 2 つの要素で構成されています。

- Sony Spresense 向けの組み込みファームウェア
- ひび割れ分類モデルの学習 / 変換スクリプト

組み込み側では、画像取得、IMU 読み取り、microSD への証跡保存、BLE クライアントへの通知を扱います。
機械学習側では、MobileNetV2 ベースの 2 値分類モデルを学習し、`.tflite` を `models/model_data.cpp` へ変換します。

## 現在の状態

本リポジトリは、Spresense アプリケーションとして動かしやすい形に整理されており、現時点では次の処理に対応しています。

- JPEG 画像の microSD 保存
- IMU サンプリングと CSV ログ保存
- BLE 通知の土台実装
- TensorFlow Lite Micro モデルの読み込み

重要:
同梱しているモデルは `160x160` 入力で学習されていますが、現在のカメラ経路は JPEG 優先です。
そのため、画像取得 / ログ / 保存の流れは動きますが、完全な機体側推論には、推論用の生画像入力経路を追加する必要があります。

現場テストを止めないため、ファームウェアには保存フォールバックモードを入れています。

- 推論用フレームが取れない場合でも、JPEG と IMU ログは保存する
- BLE 通知は、実際の推論結果がある場合にだけ送る

## 使用ハードウェア

- Sony Spresense メインボード
- Sony HDR Camera Board
- Spresense Multi-IMU Add-on Board `CXD5602PWBIMU1J`
- NUS firmware 書き込み済み BLE1507
- microSD カード

## 使用ソフトウェア

- Arduino for Spresense
- TensorFlow Lite for Microcontrollers
- Camera ライブラリ
- SDHCI ストレージ
- `Serial2` 経由の BLE UART 通信

## ディレクトリ構成

```text
DroneInspector/
|- app/
|  |- App.cpp
|  `- App.h
|- models/
|  |- model_data.cpp
|  `- model_data.h
|- src/
|  |- core/
|  |  |- config.h
|  |  |- pins.h
|  |  |- types.h
|  |  |- utils.cpp
|  |  `- utils.h
|  `- modules/
|     |- ble/
|     |- camera/
|     |- classifier/
|     |- imu/
|     |- logger/
|     `- storage/
|- img/
|- crack_classifier_mobilenetv2.tflite
|- DroneInspector.ino
|- model_create.py
|- mdoel_create.py
|- uav_model.py
`- README.md
```

## ファームウェアの流れ

1. `storage` / `camera` / `IMU` / `BLE` / `classifier` を初期化する
2. JPEG フレームを取得する
3. IMU サンプルを更新する
4. 推論用フレームの取得を試す
5. 画像保存とイベントログ追記を行う
6. 実際の検出結果がある場合だけ BLE 通知を送る

## モデル変換フロー

`uav_model.py` には、Colab ベースの MobileNetV2 学習フローが入っています。
学習済み `.tflite` を C++ 配列へ変換するには、次を実行します。

```bash
python model_create.py
```

`mdoel_create.py` は、古い参照との互換のために残しているエイリアスです。

## Arduino 向けメモ

- Arduino IDE 上では、環境依存で include 警告が出ることがあります
- 重要なのは、Spresense のボード環境と必要ライブラリが正しく入っていることです
- このリポジトリは、`camera` / `BLE` / `IMU` / `storage` を個別に切り分けて確認しやすい構成にしています

## 今後の課題

- 機体側の完全推論に向けて、生画像からモデル入力へつなぐ経路を追加する
- 配備モデルに合わせて tensor arena とメモリ使用量を調整する
- 異常時のみ保存するイベント方針をより明確にする
- Colab 以外でも再現しやすい評価 / 自動化フローを追加する
