# ------------------------------------------------------------------------------
# configs from configure.py
#

-include configure.mk

ifndef G5_CONFIGURE_DONE
$(error "Project is not configured. Run 'python configure.py' in top level dir")
endif

compile: compile_cc
	go build -o /dev/null cmd/bot/main.go

run:
	go run cmd/bot/main.go

test:
	go test g5/...

clean:

release: clean compile

fmt: fmt_cc
	go fmt g5/...

compile_cc:
	make -C cc compile

fmt_cc:
	make -C cc fmt

