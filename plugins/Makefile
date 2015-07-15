define colorecho
	@tput setaf 10
	@echo $1
	@tput sgr0
endef

CC=g++
CFLAGS=-c -fPIC -std=c++11 -g -MP -MMD
LDFLAGS=-Wl,-soname,move.so -shared -fPIC

SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
SOFILES=$(SOURCES:.cpp=.so)
DEPS=$(OBJECTS:.o=.d)


all: $(SOFILES)
	$(call colorecho, "Plugins ready")

%.so: %.o
	@$(CC) -Wl,-soname,$@ $(LDFLAGS) $< -o $@

%.o : %.cpp
	$(call colorecho, "Building cpp file " $<)
	@$(CC) $(CFLAGS) $< -o $@

clean:
	@rm -rf $(OBJECTS) $(DEPS) $(SOFILES)

-include $(DEPS)


