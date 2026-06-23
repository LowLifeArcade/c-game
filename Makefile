APP := build/pilgrim
SRC := src/main.c
VERSION_FILE := VERSION
VERSION := $(shell tr -d '[:space:]' < $(VERSION_FILE))

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	CFLAGS := -std=c99 -Wall -Wextra -pedantic -O2 -DGL_SILENCE_DEPRECATION
	LDFLAGS := -framework GLUT -framework OpenGL -framework Cocoa -framework CoreGraphics -framework ImageIO -framework CoreFoundation -lm
else
	CFLAGS := -std=c99 -Wall -Wextra -pedantic -O2
	LDFLAGS := -lglut -lGL -lGLU -lm
endif

.PHONY: all run tune version package-macos check-animations analyze-walk observe-walk review-walk analyze-idle analyze-jump analyze-proportions clean

all: $(APP)

$(APP): $(SRC) $(VERSION_FILE)
	@mkdir -p build
	$(CC) $(CFLAGS) -DPILGRIM_VERSION=\"$(VERSION)\" $< -o $@ $(LDFLAGS)

run: $(APP)
	./$(APP)

tune: $(APP)
	PILGRIM_TUNE=1 ./$(APP)

version:
	@printf '%s\n' "$(VERSION)"

package-macos: $(APP)
	./scripts/package_macos.sh

check-animations: $(APP)
	./scripts/check_animations.sh

analyze-walk: $(APP)
	./scripts/analyze_walk.sh

observe-walk: $(APP)
	./scripts/observe_walk.sh

review-walk: $(APP)
	./scripts/review_walk.sh

analyze-idle: $(APP)
	./scripts/analyze_idle.sh

analyze-jump: $(APP)
	./scripts/analyze_jump.sh

analyze-proportions: $(APP)
	./scripts/analyze_proportions.sh

clean:
	rm -rf build
