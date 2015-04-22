define colorecho
	@tput setaf 4
	@echo $1
	@tput sgr0
endef

SRC=./src
all:
	$(call colorecho, "Compiling src")
	@make -C $(SRC)
	$(call colorecho, "Build finished")
rebuild: clean all
clean:
	$(call colorecho, "Cleaning garbage")
	@make -C $(SRC) clean

