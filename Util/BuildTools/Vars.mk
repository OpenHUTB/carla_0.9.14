# 这里，CURDIR 假设为工程的根目录。
# 设置编译所需的一些目录变量

# CURDIR 是 makefile 的内嵌变量，显示当前路径
CARLA_ROOT_FOLDER=${CURDIR}
# 构建的临时目录
CARLA_BUILD_FOLDER=${CURDIR}/Build
# 发布的目录
CARLA_DIST_FOLDER=${CURDIR}/Dist
# 工具目录
CARLA_UTIL_FOLDER=${CURDIR}/Util
# 容器工具目录
CARLA_DOCKER_UTILS_FOLDER=${CARLA_UTIL_FOLDER}/DockerUtils
# 构建工具目录
CARLA_BUILD_TOOLS_FOLDER=${CARLA_UTIL_FOLDER}/BuildTools
# 测试结果目录
CARLA_TEST_RESULTS_FOLDER=${CARLA_BUILD_FOLDER}/test-results

# Carla所对应虚幻引擎根目录
CARLAUE4_ROOT_FOLDER=${CURDIR}/Unreal/CarlaUE4
# 虚幻引擎 Carla 插件的目录
CARLAUE4_PLUGIN_ROOT_FOLDER=${CURDIR}/Unreal/CarlaUE4/Plugins/Carla
# Python 接口目录
CARLA_PYTHONAPI_ROOT_FOLDER=${CURDIR}/PythonAPI
# Python 接口的源代码目录
# Carla: PythonAPI/carla/source/carla/command.py
# boost->客户端->rpc:client : PythonAPI/carla/source/libcarla/*.cpp
CARLA_PYTHONAPI_SOURCE_FOLDER=${CARLA_PYTHONAPI_ROOT_FOLDER}/carla

# Carla库的目录
LIBCARLA_ROOT_FOLDER=${CURDIR}/LibCarla
# Carla 库的服务端构建目录
LIBCARLA_BUILD_SERVER_FOLDER=${CARLA_BUILD_FOLDER}/libcarla-server-build
# Carla 库的 Pytorch 构建目录
LIBCARLA_BUILD_PYTORCH_FOLDER=${CARLA_BUILD_FOLDER}/libcarla-pytorch-build
# Carla 库的客户端构建目录
LIBCARLA_BUILD_CLIENT_FOLDER=${CARLA_BUILD_FOLDER}/libcarla-client-build
# Carla 插件的依赖目录（服务端）
LIBCARLA_INSTALL_SERVER_FOLDER=${CARLAUE4_PLUGIN_ROOT_FOLDER}/CarlaDependencies
# Python 的依赖目录
LIBCARLA_INSTALL_CLIENT_FOLDER=${CARLA_PYTHONAPI_SOURCE_FOLDER}/dependencies

OSM2ODR_BUILD_FOLDER=${CARLA_BUILD_FOLDER}/libosm2dr-build
# 地图转驾驶文件的源代码（sumo）
OSM2ODR_SOURCE_FOLDER=${CARLA_BUILD_FOLDER}/libosm2dr-source

# Carla 插件的依赖目录
CARLAUE4_PLUGIN_DEPS_FOLDER=${CARLAUE4_PLUGIN_ROOT_FOLDER}/CarlaDependencies

LIBSTDCPP_TOOLCHAIN_FILE=${CARLA_BUILD_FOLDER}/LibStdCppToolChain.cmake
LIBCPP_TOOLCHAIN_FILE=${CARLA_BUILD_FOLDER}/LibCppToolChain.cmake
# Cmake 配置文件
CMAKE_CONFIG_FILE=${CARLA_BUILD_FOLDER}/CMakeLists.txt.in

LIBCARLA_TEST_CONTENT_FOLDER=${CARLA_BUILD_FOLDER}/test-content
CARLA_EXAMPLES_FOLDER=${CURDIR}/Examples
