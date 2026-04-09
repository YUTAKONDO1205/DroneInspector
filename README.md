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

## 現在の対応機能

本リポジトリは、Spresense アプリケーションとして動かしやすい形に整理されており、現時点では次の処理に対応しています。

- JPEG 画像の microSD 保存
- IMU サンプリングと CSV ログ保存
- BLE 通知の土台実装
- TensorFlow Lite Micro モデルの読み込み

## 現時点の制約

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

## Quick Start

初見の方向けに、まず動作確認まで進むための最短手順をまとめます。

1. Arduino IDE に `Arduino for Spresense` を導入する
2. このスケッチで使うライブラリ (`Camera`, `SDHCI`, `TensorFlow Lite for Microcontrollers`) を利用可能な状態にする
3. Spresense 本体にカメラボードと Multi-IMU Add-on Board を接続し、microSD カードと NUS firmware 書き込み済み BLE1507 を準備する
4. `DroneInspector.ino` を Arduino IDE で開く
5. Spresense のボード設定とシリアルポートを選び、スケッチを書き込む
6. シリアルモニタを開き、ストレージ / カメラ / IMU / BLE / classifier の初期化ログを確認する

補足:

- リポジトリには、変換済みの `models/model_data.cpp` / `models/model_data.h` が含まれているため、既定構成の動作確認だけであれば Python スクリプトの実行は必須ではありません
- モデルを差し替えた場合のみ、後述の `model_create.py` を使って C++ 配列を再生成してください

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
学習からファームウェア組み込みまでの流れは次のとおりです。

1. `uav_model.py` で学習し、`.tflite` モデルを出力する
2. 生成した `.tflite` をリポジトリ直下の `crack_classifier_mobilenetv2.tflite` として配置する
3. 次のコマンドで `models/model_data.h` と `models/model_data.cpp` を再生成する

```bash
python model_create.py
```

通常は `model_create.py` を使ってください。
`mdoel_create.py` は typo ではなく、古い参照や手元スクリプトとの互換のために残しているエイリアスで、内部では `model_create.py` を呼び出します。

## Arduino 向けメモ

- Arduino IDE 上では、環境依存で include 警告が出ることがあります
- 重要なのは、Spresense のボード環境と必要ライブラリが正しく入っていることです
- このリポジトリは、`camera` / `BLE` / `IMU` / `storage` を個別に切り分けて確認しやすい構成にしています

## 今後の課題

- 機体側の完全推論に向けて、生画像からモデル入力へつなぐ経路を追加する
- 配備モデルに合わせて tensor arena とメモリ使用量を調整する
- 異常時のみ保存するイベント方針をより明確にする
- Colab 以外でも再現しやすい評価 / 自動化フローを追加する
