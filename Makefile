BUILD_DIR  = build
BUILD_TYPE ?= Debug

.PHONY: all configure build run test clean distclean debug release

all: build

# ── Configuration ─────────────────────────────────────────────────────────────
# CMakeCache.txt is used as a stamp: cmake only runs when the build directory
# doesn't exist yet. After that, `cmake --build` detects CMakeLists.txt changes
# and re-runs cmake automatically as part of the incremental build.

$(BUILD_DIR)/CMakeCache.txt:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

configure: $(BUILD_DIR)/CMakeCache.txt

# ── Build (incremental) ───────────────────────────────────────────────────────

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) --parallel

run: build
	./$(BUILD_DIR)/ptb

# ── Tests ─────────────────────────────────────────────────────────────────────

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

# ── Clean ─────────────────────────────────────────────────────────────────────

# Remove compiled artifacts while keeping the CMake cache (fast incremental rebuilds)
clean:
	cmake --build $(BUILD_DIR) --target clean 2>/dev/null || true

# Full wipe — use when switching BUILD_TYPE or after toolchain changes
distclean:
	rm -rf $(BUILD_DIR)

# ── Build-type presets ────────────────────────────────────────────────────────

debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --parallel

release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --parallel
