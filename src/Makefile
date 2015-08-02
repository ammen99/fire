define colorecho
	@tput setaf 123
	@echo $1
	@tput sgr0
endef

CC=g++
CFLAGS= -std=c++11 -c -g -MMD -MP
LDFLAGS= -Wl,--export-dynamic -pthread -g -lX11 -lXmu -lXext -lXcomposite -lXfixes -lXdamage -ldl -lGL -lGLEW -lIL -lILU -lILUT -lutil
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)
EXECUTABLE=../manager

all: $(EXECUTABLE)
	$(call colorecho, "Output executable $(EXECUTABLE) ready")

clean:
	@rm -rf $(OBJECTS) $(DEPS) $(EXECUTABLE)

rebuild: clean all

$(EXECUTABLE): $(OBJECTS)
	$(call colorecho, "Linking...")
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(call colorecho, "Building cpp file " $<)
	@$(CC) $(CFLAGS) $< -o $@
# Let make read the dependency files and handle them.
-include $(DEPS)
