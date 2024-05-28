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

game:
	go run cmd/bot/main.go game

test:
	go test g5/...

clean:

release: clean compile

fmt: fmt_go fmt_cc

fmt_go:
	go fmt g5/...

compile_cc:
	make --no-print-directory -C cc compile

fmt_cc:
	make --no-print-directory -C cc fmt

