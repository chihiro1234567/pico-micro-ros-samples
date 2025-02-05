#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"

#include <uxr/client/profile/transport/custom/custom_transport.h>

#include "hardware/uart.h"
#include "hardware/gpio.h"

/*
micro-ROS <=> ROS2のカスタムデータ転送を実装する
サンプルを元にしているが、デフォルトだとUART0を使っていて、
DebugProbeで利用しているデバッグ出力とバッティングする
そこで、こちらのシリアル通信をUART1（GPIO4、GPIO5）に変更する
*/
#define UART_ID       uart1      // 使用するUART
#define BAUD_RATE     115200     // ボーレート（必要に応じて調整）
#define UART_TX_PIN   4          // UART1のTXに使うGPIOピン（例: 4番ピン）
#define UART_RX_PIN   5          // UART1のRXに使うGPIOピン（例: 5番ピン）
/*
以下をワイヤ接続する
micro-ROS TX(GPIO4) <==> ROS2 RX
micro-ROS RX(GPIO5) <==> ROS2 TX
micro-ROS GND       <==> ROS2 GND ※GNDも忘れずに接続
*/

// UART1の初期化例
void init_uart1(void)
{
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

bool pico_serial_transport_open(struct uxrCustomTransport * transport)
{
    // UART1 初期化
    init_uart1();
    return true;
}

bool pico_serial_transport_close(struct uxrCustomTransport * transport)
{
    return true;
}

size_t pico_serial_transport_write(struct uxrCustomTransport * transport, uint8_t *buf, size_t len, uint8_t *errcode)
{
    uart_write_blocking(UART_ID, buf, len);
    return len;
}

size_t pico_serial_transport_read(struct uxrCustomTransport * transport, uint8_t *buf, size_t len, int timeout, uint8_t *errcode)
{
    size_t count = 0;
    uint64_t start_time_us = time_us_64();
    while (count < len)
    {
        if(uart_is_readable(UART_ID))
        {
            buf[count] = uart_getc(UART_ID);
            count++;
        }
        else if (time_us_64() - start_time_us > timeout * 1000)
        {
            *errcode = 1;
            break;
        }
    }
    return count;
}


void usleep(uint64_t us)
{
    sleep_us(us);
}

int clock_gettime(clockid_t unused, struct timespec *tp)
{
    uint64_t m = time_us_64();
    tp->tv_sec = m / 1000000;
    tp->tv_nsec = (m % 1000000) * 1000;
    return 0;
}
