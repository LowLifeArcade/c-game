APP := build/pilgrim
SRC := src/main.c

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	CFLAGS := -std=c99 -Wall -Wextra -pedantic -O2 -DGL_SILENCE_DEPRECATION
	LDFLAGS := -framework GLUT -framework OpenGL -framework Cocoa -framework CoreGraphics -framework ImageIO -framework CoreFoundation -lm
else
	CFLAGS := -std=c99 -Wall -Wextra -pedantic -O2
	LDFLAGS := -lglut -lGL -lGLU -lm
endif

.PHONY: all run check-animations analyze-walk clean

all: $(APP)

$(APP): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: $(APP)
	./$(APP)

check-animations: $(APP)
	./scripts/check_animations.sh

analyze-walk: $(APP)
	./scripts/analyze_walk.sh

clean:
	rm -rf build
