import serial
import time

# UART 送信プログラム
# メッセージを定期的に送信し続ける

# Windows PCの場合
# python -m venv venv
# venv/Scripts/activate.bat
# pip install pyserial
# python sample_serial_tx.py

# ※ COMポートはデバイスマネージャーで確認したものに置き換える
COM_PORT = "COM6"
# 受信側のBaud Rateと合わせる
BAUD_RATE = 230400

try:
    # シリアルポートをオープン
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    # ポートが安定するまで少し待つ
    time.sleep(1)
except Exception as e:
    print(f"シリアルポート {COM_PORT} をオープンできません: {e}")
    exit(1)


# プログラムはCtrl+Cで停止する
while True:
    # 現在のUnix時間を1sec間隔で送信する
    message = "serial message test. time=%s\r\n" % (time.time())
    # メッセージを送信
    ser.write(message.encode("utf-8"))
    print("send message. %s" % (message))
    time.sleep(1)

# ポートを閉じる
ser.close()
