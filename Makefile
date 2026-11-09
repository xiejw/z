FMT_DIRS := $(sort $(dir $(shell grep -lE '^fmt:' */[Mm]akefile 2>/dev/null)))

.PHONY: fmt clean

fmt:
	@set -e; for d in $(FMT_DIRS); do \
		echo "==> make -C $$d fmt"; \
		$(MAKE) -C $$d fmt; \
	done

clean:
	go run ~/Workspace/y/tools/scripts/delete_unused_dirs.go
