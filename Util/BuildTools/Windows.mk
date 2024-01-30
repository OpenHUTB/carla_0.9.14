ARGS=--all

default: help

# 工程的根目录(makefile 所在的目录)
export ROOT_PATH=$(CURDIR)/

# dependecy install/build directory (rpclib, gtest, boost)
export INSTALLATION_DIR=$(ROOT_PATH)Build/

help:
	@type "${CARLA_BUILD_TOOLS_FOLDER}\Windows.mk.help"

# 使用 PHONY 强制下一行作为命令，避免与同名文件夹冲突
.PHONY: import
import: server
	@"${CARLA_BUILD_TOOLS_FOLDER}/Import.py" $(ARGS)

CarlaUE4Editor: LibCarla
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildCarlaUE4.bat" --build $(ARGS)

# 启动虚幻编辑器：make launch
launch: CarlaUE4Editor
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildCarlaUE4.bat" --launch $(ARGS)

launch-only:
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildCarlaUE4.bat" --launch $(ARGS)

# 构建 CARLA 并创建用于分发的打包版本（第一次编译花3个多小时，后面只编译改变的场景）
package: PythonAPI
	@"${CARLA_BUILD_TOOLS_FOLDER}/Package.bat" --ue-version 4.26 $(ARGS)

.PHONY: docs
docs:
	@doxygen
	@echo "Documentation index at ./Doxygen/html/index.html"

PythonAPI.docs:
	python PythonAPI/docs/doc_gen.py
	cd PythonAPI/docs && python bp_doc_gen.py

clean:
	@"${CARLA_BUILD_TOOLS_FOLDER}/Package.bat" --clean --ue-version 4.26
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildCarlaUE4.bat" --clean
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildPythonAPI.bat" --clean
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildLibCarla.bat" --clean
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildOSM2ODR.bat" --clean

rebuild: setup
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildCarlaUE4.bat" --rebuild
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildLibCarla.bat" --rebuild
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildOSM2ODR.bat" --rebuild
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildPythonAPI.bat" --rebuild

check: PythonAPI
	@echo "Not implemented!"

benchmark: LibCarla
	@echo "Not implemented!"

.PHONY: PythonAPI
PythonAPI: LibCarla osm2odr
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildPythonAPI.bat" --py3

server: setup
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildLibCarla.bat" --server

client: setup
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildLibCarla.bat" --client

.PHONY: LibCarla
LibCarla: setup
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildLibCarla.bat" --server --client

setup:
	@"${CARLA_BUILD_TOOLS_FOLDER}/Setup.bat" --boost-toolset msvc-14.2 $(ARGS)

.PHONY: Plugins
plugins:
	@"${CARLA_BUILD_TOOLS_FOLDER}/Plugins.bat" $(ARGS)

deploy:
	@"${CARLA_BUILD_TOOLS_FOLDER}/Deploy.bat" $(ARGS)

osm2odr:
	@"${CARLA_BUILD_TOOLS_FOLDER}/BuildOSM2ODR.bat" --build $(ARGS)
