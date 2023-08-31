
## Carla 网页可视化示例

这个例子产生了一辆智能车和另外两辆车，并将一个摄像头和一个激光雷达连接到智能车上。

### 环境配置
1. python3
2. [carla](https://pypi.org/project/carla/)

### 使用
```bash
# 1. 启动 carla 仿真器（或者启动RoadRunner和Carla的协同仿真）
cd CARLA_SIMULATOR_PATH
./CarlaUE4.sh

# 2. 运行 docker 镜像
# Linux
docker run -it --network="host" mjxu96/carlaviz:0.9.14.1 \
  --simulator_host localhost \
  --simulator_port 2000

# Windows/MacOS
docker run -it -p 8080-8081:8080-8081 mjxu96/carlaviz:0.9.14.1 
  --simulator_host host.docker.internal  --simulator_port 2000

# 3. 运行示例脚本（显示RoadRunner和Carla的协同仿真这步可跳过）
python3 example.py

# 4. 打开浏览器并进入 localhost:8080
```
