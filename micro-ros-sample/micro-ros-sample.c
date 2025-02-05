#include <stdio.h>
#include "pico/stdlib.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#include "pico_uart_transports.h"

/* micro-ROSの主要コンポーネント
    rcl (ROS Client Library): ROS 2の基本的なクライアントライブラリです。
        ノード、パブリッシャー、サブスクライバー、タイマーなどの基本要素を提供します。

    RCLC (ROS Client Library for C): rclをさらに扱いやすくするためのラッパーライブラリで、
        マイコン向けに簡略化されたAPIを提供します。
    
    rmw (ROS Middleware): ROS 2の通信部分（DDSやその他のミドルウェア）とrclとの間の抽象化レイヤーです。
        micro-ROSでは、リソース制約のある環境向けにカスタムなトランスポート層
        （シリアル通信など）を利用するために、rmw_microrosが使用されます。
*/

const uint LED_PIN = 25;

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;

// timerコールバック
void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    // 指定されたパブリッシャーを通して、メッセージ（ここでは std_msgs__msg__Int32 型）を発行します
    rcl_ret_t ret = rcl_publish(&publisher, &msg, NULL);
    // 送信後に msg.data をインクリメント
    msg.data++;
    printf("timer callback msg.data = %d\n", msg.data);
    gpio_put(LED_PIN, msg.data % 2);
    //gpio_put(LED_PIN, 1);
}

int main()
{
    // 標準入出力（シリアルコンソールなど）の初期化を行う
    stdio_init_all();
    
    printf("micro-ros-sample start.\n");
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // LEDオフ
    gpio_put(LED_PIN, 0);

    // シリアル通信に必要なオープン、クローズ、ライト、リードの各関数を登録します。
    // これにより、micro-ROSエージェントと通信するための物理層が確立されます。
    rmw_uros_set_custom_transport(
		true,
		NULL,
		pico_serial_transport_open,
		pico_serial_transport_close,
		pico_serial_transport_write,
		pico_serial_transport_read
	);
    
    // micro-ROSエージェントへの接続が確立できるか確認します。
    // 接続できなければ、プログラムは終了します。(15sec)
    const int timeout_ms = 1000; 
    const uint8_t attempts = 15;
    rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);
    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program.
        printf("ERROR) Unreachable agent, exiting program.\n");
        return ret;
    }
    printf("OK) Ping has reached the micro-ROS agent.\n");
    gpio_put(LED_PIN, 1);

    // デフォルトのメモリアロケータを取得します。
    rcl_allocator_t allocator;
    allocator = rcl_get_default_allocator();
    // ROS 2システム全体で使用するサポートオブジェクトを初期化します。
    // これは、後続のノードやタイマーの初期化に利用されます。
    rclc_support_t support;
    rclc_support_init(&support, 0, NULL, &allocator);
    
    // "pico_node" という名前のノードを作成します。
    // ノードはROS 2における基本の実行単位です。
    rcl_node_t node;
    rclc_node_init_default(&node, "pico_node", "", &support);
    
    // std_msgs/msg/Int32 型のメッセージを 
    // "pico_publish_topic" というトピックで発行するパブリッシャーを初期化します。
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "pico_publish_topic");
    
    // 1秒（1000ms）ごとに実行されるタイマーを初期化し、
    // そのタイマーのコールバック関数として timer_callback を登録します。
    rcl_timer_t timer;
    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        timer_callback);
    
    // エグゼキューター（実行ループ管理オブジェクト）を初期化し、
    // 先ほど作成したタイマーを rclc_executor_add_timer() で登録します。
    // エグゼキューターは、登録されたタイマーやサブスクライバーの
    // コールバックを管理し、定期的に実行します。
    rclc_executor_t executor;
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_timer(&executor, &timer);

    printf("executor start.\n");
    while (true) {
        // 登録されたコールバック（タイマー、サブスクライバー、サービスなど）を
        //「順次」実行するための関数です。つまり、複数のコールバックがエグゼキューターに
        // 登録されていた場合、同時並行（並列）に実行されるのではなく、一つずつシーケンシャルに実行されます。
        // 100ミリ秒をナノ秒に変換した値を指定しており、呼び出しでコールバックを
        // チェックまたは実行する際のタイムアウトの上限時間を指定する

        // エグゼキューターは、100ミリ秒分の時間（ナノ秒単位）だけ待機し、
        // その間に到着した（または実行可能になった）コールバックを順次処理する。
        // もし100ミリ秒の間に実行すべきコールバックが存在しない場合、
        // タイムアウトとなり、処理を抜ける。

        // while(true)ループ内で何度も実行する必要がある（都度コールバックするため）
        // タイムアウト値が設定されているので、「最大100ms」ウェイトされる。
        // 調整の為、スリープを入れる必要がない。
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        
    }
}
