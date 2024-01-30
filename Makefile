# 运行另一个一个设置环境变量的脚本
include Util/BuildTools/Vars.mk
ifeq ($(OS),Windows_NT)
include Util/BuildTools/Windows.mk
else
include Util/BuildTools/Linux.mk
endif
