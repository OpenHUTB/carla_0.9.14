# manual_control.py
手动控制车辆

# generate_traffic.py
动态产生交通

# no_rendering_mode.py
显示红绿灯的变化

# visualize_multiple_sensors.py 
可视化多个传感器的内容

# automatic_control.py 
从客户端进行自动驾驶控制

客户端连接不上服务端的默认2000端口
```shell
netstat -ano | findstr "2000"
taskkill /PID 35320 /F
```


服务端指定端口启动:
```shell
CarlaUE4.exe -carla-rpc-port=3000
```
指定端口连接服务端：
```shell
-p 3000
```
