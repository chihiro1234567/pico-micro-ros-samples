#include <stdio.h>
#include "pico/stdlib.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>
#include <rmw_microros/rmw_microros.h>

#include "pico_uart_transports.h"

const uint LED_PIN = 25;

// サブスクリプションのコールバック関数
void topic1_subscribe(const void * msgin)
{
  const std_msgs__msg__Float32 * msg_float = (const std_msgs__msg__Float32 *)msgin;
  if (msg_float == NULL) {
    return;
  }
  // 受信した float データを表示
  printf("[subscribe topic1] %.2f\n", msg_float->data);
}

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg_int;
// timerコールバック
void topic2_publish(rcl_timer_t *timer, int64_t last_call_time)
{
    // メッセージをパブリッシュ
    rcl_ret_t ret = rcl_publish(&publisher, &msg_int, NULL);
    printf("[publish topic2] %d\n", msg_int.data);
    gpio_put(LED_PIN, msg_int.data % 2);
    // 送信後に msg.data をインクリメント
    msg_int.data++;
}

int main()
{
    stdio_init_all();
    printf("-------------------------\n");
    printf("micro-ros-sample2 start\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    // LEDオフ
    gpio_put(LED_PIN, 0);
    
    rmw_uros_set_custom_transport(
		true,
		NULL,
		pico_serial_transport_open,
		pico_serial_transport_close,
		pico_serial_transport_write,
		pico_serial_transport_read
	);
    // micro-ROSエージェントへの接続が確立できるか確認
    const int timeout_ms = 1000; 
    const uint8_t attempts = 15;
    rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);
    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program.
        printf("ERROR) Unreachable agent, exiting program.\n");
        return ret;
    }
    printf("OK1) Ping has reached the micro-ROS agent.\n");

    // デフォルトのメモリアロケータを取得
    rcl_allocator_t allocator;
    allocator = rcl_get_default_allocator();
    printf("OK2) rcl_get_default_allocator.\n");
    // ROS 2システム全体で使用するサポートオブジェクトを初期化
    rclc_support_t support;
    rclc_support_init(&support, 0, NULL, &allocator);
    printf("OK3) rclc_support_init.\n");
    // "pico_node"ノードを作成
    rcl_node_t node;
    rclc_node_init_default(&node, "pico_node", "", &support);
    printf("OK4) rclc_node_init_default.\n");

    // エグゼキュータの初期化
    // rclc_executor_add_subscription
    // rclc_executor_add_timer
    // 2個のハンドルを管理する
    int num_of_handles = 2;
    rclc_executor_t executor;
    rclc_executor_init(&executor, &support.context, num_of_handles, &allocator);
    printf("OK5) rclc_executor_init.\n");
    //------------------------
    // サブスクリプション
    //------------------------

    // サブスクリプションの作成
    // トピック名: pico_topic1
    // メッセージ型: std_msgs/msg/Float32
    rcl_subscription_t subscriber;
    rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
        "pico_topic1");
    printf("OK6) rclc_subscription_init_default.\n");
    // エグゼキュータにサブスクリプションを登録
    std_msgs__msg__Float32 msg;
    rclc_executor_add_subscription(
        &executor, &subscriber,&msg, topic1_subscribe, ON_NEW_DATA);
    printf("OK7) rclc_executor_add_subscription.\n");
    //------------------------
    // パブリッシャー
    //------------------------
    // std_msgs/msg/Int32 型のメッセージをトピックで発行するパブリッシャーを作成
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "pico_topic2");
    printf("OK8) rclc_publisher_init_default.\n");
    // 1秒（1000ms）ごとに実行されるタイマーを初期化し、
    // そのタイマーのコールバック関数として timer_callback を登録します。
    rcl_timer_t timer;
    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        topic2_publish);
    printf("OK9) rclc_timer_init_default.\n");
    // エグゼキュータにパブリッシャーを登録
    rclc_executor_add_timer(&executor, &timer);
    printf("OK10) rclc_executor_add_timer.\n");

    // メインループ: エグゼキュータを定期的にスピンしてコールバック処理を実行
    while (true) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
    }
    printf("micro-ros-sample2 end.\n");
}
