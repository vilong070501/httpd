SRC_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

config/libconfig.a:
	$(MAKE) -C $(SRC_DIR)config

utils/libutils.a:
	$(MAKE) -C $(SRC_DIR)utils

daemon/libdaemon.a:
	$(MAKE) -C $(SRC_DIR)daemon

server/libserver.a:
	$(MAKE) -C $(SRC_DIR)server

logger/liblogger.a:
	$(MAKE) -C $(SRC_DIR)logger

http/libhttp.a:
	$(MAKE) -C $(SRC_DIR)http
