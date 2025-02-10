/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "pico/async_context_threadsafe_background.h"

#include "hardware/pio.h"
#include "hardware/uart.h"
#include "uart_rx.pio.h"

/*
    Raspberry Pi Pico PIO UART RX
    PIOでUART RXを作成して指定したピンに割り当てる
    受信したデータはキューに保存して非同期で後続処理を実行する
    現状では、後続処理では文字列をprintfで表示しているだけ
*/
#define MAX_COUNTER 10

static PIO pio;
static uint sm;
static int8_t pio_irq;
static queue_t fifo;
static uint offset;
static uint32_t counter;
static bool work_done;

static void async_worker_func(async_context_t *async_context, async_when_pending_worker_t *worker);
static async_context_threadsafe_background_t async_context;
static async_when_pending_worker_t worker = { .do_work = async_worker_func };

// pio fifoが空でないとき、つまりUART上に文字があるときに呼び出されるIRQ。
// 割込みハンドラは、できるだけ短い時間で終了する必要があります。
// もしハンドラ内で printf() のような比較的時間のかかる処理（I/O処理など）を行うと、
// ハンドラの実行時間が延び、他の割込みやリアルタイム性が求められる処理に悪影響を及ぼす可能性があります。
// そこで、IRQハンドラでは単に受信データをキューに詰めるだけにして、後でメインループや別のコンテキストで処理するようにしています。
static void pio_irq_func(void) {
    while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
        char c = uart_rx_program_getc(pio, sm);
        if (!queue_try_add(&fifo, &c)) {
            panic("fifo full");
        }
    }
    // スレッドセーフなバックグラウンド async コンテキスト）に対して「作業が保留中である」ことを通知する
    async_context_set_work_pending(&async_context.core, &worker);
}

// 非同期処理 キューからデータを取り出して標準出力
static void async_worker_func(
    __unused async_context_t *async_context,
    __unused async_when_pending_worker_t *worker) {
    
    work_done = true;
    while(!queue_is_empty(&fifo)) {
        char c;
        if (!queue_try_remove(&fifo, &c)) {
            panic("fifo empty");
        }
        //putchar(c);
        printf("%c",c);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(100);
    printf("1) pico-pio-uart-rx start.\n");

    // キューバッファの初期化
    queue_init(&fifo, 1, 128);

    // スレッドセーフなバックグラウンド用の非同期コンテキスト（async context）を、
    // デフォルトの設定で初期化します。
    // 割込みハンドラなどからも安全にこのコンテキストを操作できるようになる。
    // この初期化により、後続の処理で「作業が保留状態（pending）」となったときに、
    // 非同期に登録されたワーカー関数が実行される仕組みが確立される。
    if (!async_context_threadsafe_background_init_with_defaults(&async_context)) {
        panic("failed to setup context");
    }
    // 初期化済みの非同期コンテキストの「コア部分」に、作業が保留状態になったときに実行すべきワーカー関数（worker）を登録します。
    // ここで登録される worker は、割込みなどで非同期処理の必要性が発生したときに、
    // async_context_set_work_pending() で通知され、後から非同期コンテキスト内で呼び出されます。
    //  IRQ ハンドラ内で受信した文字をキューに格納した後、async_context_set_work_pending() により
    // 非同期コンテキストへ「作業あり」のシグナルを送信する。
    printf("2) async_context_add_when_pending_worker\n");
    async_context_add_when_pending_worker(&async_context.core, &worker);

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
    printf("3) pio_claim_free_sm_and_add_program_for_gpio_range\n");
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&uart_rx_program, &pio, &sm, &offset, uart_rx_gpio, 1, true);
    hard_assert(success);

    // uart_rx_program
    printf("4) uart_rx_program_init\n");
    uart_rx_program_init(pio, sm, offset, uart_rx_gpio, uart_rx_band);

    // Find a free irq
    printf("5) Find a free irq\n");
    pio_irq = pio_get_irq_num(pio, 0);
    if (irq_get_exclusive_handler(pio_irq)) {
        pio_irq++;
        if (irq_get_exclusive_handler(pio_irq)) {
            panic("All IRQs are in use");
        }
    }
    // pio_irq_funcを割込みハンドラとして登録
    printf("6) irq_add_shared_handler\n");
    irq_add_shared_handler(pio_irq, pio_irq_func, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    // 割込みを有効にする
    irq_set_enabled(pio_irq, true);
    const uint irq_index = pio_irq - pio_get_irq_num(pio, 0);
    // PIO（Programmable I/O）に対して
    // RX FIFO が空でなくなったとき（つまりデータが受信されたとき）の割込み
    // を有効にする設定
    printf("7) pio_set_irqn_source_enabled\n");
    pio_set_irqn_source_enabled(pio, irq_index, pio_get_rx_fifo_not_empty_interrupt_source(sm), true);

    printf("8) enter while loop\n");
    while (counter < MAX_COUNTER || work_done) {
        // 実際の worker 関数の実行は、メインループ内で async_context_poll() や 
        // async_context_wait_for_work_ms() などを通じて、非同期コンテキストが処理する際に呼ばれます。
        // つまり、IRQ で即座に worker が実行されるのではなく、割込みコンテキスト外で非同期に実行されます。
        work_done = false;
        async_context_poll(&async_context.core);
        async_context_wait_for_work_ms(&async_context.core, 2000);
    }
    printf("9) exit while loop\n");
    // Disable interrupt
    pio_set_irqn_source_enabled(pio, irq_index, pio_get_rx_fifo_not_empty_interrupt_source(sm), false);
    irq_set_enabled(pio_irq, false);
    irq_remove_handler(pio_irq, pio_irq_func);

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&uart_rx_program, pio, sm, offset);

    async_context_remove_when_pending_worker(&async_context.core, &worker);
    async_context_deinit(&async_context.core);
    queue_free(&fifo);

    printf("10) pico-pio-uart-rx end.\n");
    return 0;
}
