test:
	make -C c4c          test && \
	make -C taocp        test && \
	make -C gan          test && \
	echo "We are good"

clean:
	fd -I configure.mk -X rm
	go run ~/Workspace/y/tools/delete_unused_dirs.go
