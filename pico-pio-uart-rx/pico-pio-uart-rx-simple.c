/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "uart_rx.pio.h"

int main() {
    stdio_init_all();
    sleep_ms(100);
    printf("start pico-pio-uart-rx-simple\n");
    
    PIO pio;
    uint sm;
    uint offset;

    // PIO UART RX GPIO
    const uint uart_rx_gpio = 26;
    // UART RX Baud Rate
    const uint uart_rx_band = 230400;

    // 空きのステートマシンの自動割り当て
    // 指定されたPIOインスタンス（pio）から、まだ使われていないステートマシンを探し、その番号を変数 sm に設定します。
    // プログラムのロード
    // 第一引数の &uart_rx_program で渡されたPIOプログラムを、PIOの命令メモリにロードします。
    // ロードされた位置（開始オフセット）が変数 offset に格納され、後続の初期化でこのオフセットを参照してプログラムを実行できるようにします。
    // GPIOピンの範囲に対応
    // この関数は "for_gpio_range" というバリアントで、特定のGPIOピン（ここでは PIO_RX_PIN）から指定した数（この例では1ピン）
    // を使う場合に適したPIOインスタンスを選びます。たとえば、GPIO番号が32以上のピンを使う場合でも、ハードウェアが対応していれば
    // 正しいPIOインスタンスを自動的に選択できるようになっています。
    // 追加のオプション
    // 最後の引数 true は、必要に応じてプログラムを自動でロードする（追加する）かどうかを指定するフラグです。
    // true により、もしプログラムがまだロードされていなければこのタイミングでロードされます。
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&uart_rx_program, &pio, &sm, &offset, uart_rx_gpio, 1, true);
    hard_assert(success);

    // uart_rx_program
    uart_rx_program_init(pio, sm, offset, uart_rx_gpio, uart_rx_band);
    // uart_rx_mini_program
    //uart_rx_mini_program_init(pio, sm, offset, uart_rx_gpio, SERIAL_BAUD);
    
    while (true) {
        char c = uart_rx_program_getc(pio, sm);
        printf("%c",c);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&uart_rx_program, pio, sm, offset);
}
