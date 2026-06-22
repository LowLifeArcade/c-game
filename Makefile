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

.PHONY: all run tune check-animations analyze-walk observe-walk analyze-idle analyze-jump analyze-proportions clean

all: $(APP)

$(APP): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: $(APP)
	./$(APP)

tune: $(APP)
	PILGRIM_TUNE=1 ./$(APP)

check-animations: $(APP)
	./scripts/check_animations.sh

analyze-walk: $(APP)
	./scripts/analyze_walk.sh

observe-walk: $(APP)
	./scripts/observe_walk.sh

analyze-idle: $(APP)
	./scripts/analyze_idle.sh

analyze-jump: $(APP)
	./scripts/analyze_jump.sh

analyze-proportions: $(APP)
	./scripts/analyze_proportions.sh

clean:
	rm -rf build
