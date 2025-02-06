## micro-ros-sample2

ROS2 (Raspberry Pi 4, Zero2)

Launch agent

```
> source install/local_setup.sh
> ros2 run micro_ros_agent micro_ros_agent serial -b 115200 --dev /dev/ttyAMA0
```

Publish

```
> source install/local_setup.sh
> ros2 topic pub /pico_topic1 std_msgs/msg/Float32 "{data: 12.3}" --once
```

Subscribe

```
> source install/local_setup.sh
> ros2 topic echo /pico_topic2
data: 200
---
data: 201
---
:
```
