define colorecho
	@tput setaf 42
	@echo $1
	@tput sgr0
endef

SRC=./src/
PLUGINS=./plugins

all:
	$(call colorecho, "Compiling core source...")
	@make -j4 -C $(SRC)
	$(call colorecho, "Compiling plugins...")
	@make -j4 -C $(PLUGINS)
	@#TODO: fix building of plugins
	@cp $(PLUGINS)/cube/cube.so $(PLUGINS)/
	$(call colorecho, "Build finished")

rebuild: clean all
clean:
	$(call colorecho, "Cleaning garbage")
	@make -C $(SRC) clean
	@make -C $(PLUGINS) clean

install:
	$(call colorecho "Installing fireman executable in /usr/bin")
	@sudo cp src/manager /usr/bin/fireman

