CC=g++
CFLAGS= -std=c++11 -c -Wno-narrowing -MMD -MP
LDFLAGS= -g -lX11 -lXmu -lXcomposite -lXfixes -lXdamage -ldl -lGL -lGLEW -lIL -lILU -lILUT -lglog
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)
EXECUTABLE=manager

all: $(EXECUTABLE)

clean:
	rm -rf $(OBJECTS) $(DEPS) $(EXECUTABLE)

rebuild: clean all

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@
# Let make read the dependency files and handle them.
-include $(DEPS)
