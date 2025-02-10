# Pico PIO UART RX

* PIOでUART RXを作成して指定したピンに割り当てる
* 受信したデータはキューに保存して非同期で後続処理を実行する
* 現状では、後続処理では文字列をprintfで表示しているだけ

以下のソースをコピーして受信処理以外を削除してコメントを追記しただけです。

オリジナルは以下にあります。

https://github.com/raspberrypi/pico-examples/tree/master/pio/uart_rx

## 手順

1. ビルドしてPicoにFlashする
2. リセットボタン押してプログラムスタート

    GPIO26、Baud Rate=230400、8bitパリティ無しでシリアル通信可能になる

3. 以下をワイヤで繋ぐ

    |Pico(受信)|相手(送信)|
    |--|--|
    |RX(GPIO26)|TX|
    |GND|GND|

4. 送信側でシリアル送信する。一応サンプルPythonプログラムsample_serial_tx.pyがあるので実行しても確認できる

## sample_serial_tx.py

**pyserial**が必要なのでインストールして、プログラムを実行する。（Baud Rateなど合わせる）

```
python -m venv venv
venv/Scripts/activate.bat
pip install pyserial
python sample_serial_tx.py
```

## 通信内容

Picoを電源ONしてリセットすると以下のようになり受信待ち状態になる

```
1) pico-pio-uart-rx start.
2) async_context_add_when_pending_worker
3) pio_claim_free_sm_and_add_program_for_gpio_range
4) uart_rx_program_init
5) Find a free irq
6) irq_add_shared_handler
7) pio_set_irqn_source_enabled
8) enter while loop
```

サンプルのPythonプログラムをホストPC等で実行する

```
> python serial_tx.py
send message. serial message test. time=1739151109.8969576

send message. serial message test. time=1739151110.8978689

send message. serial message test. time=1739151111.898867

send message. serial message test. time=1739151112.8999104
:
```

Pico側で同一内容のメッセージが表示できる
```
serial message test. time=1739151109.8969576
serial message test. time=1739151110.8978689
serial message test. time=1739151111.898867
serial message test. time=1739151112.8999104
```
