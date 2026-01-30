test:
	make -C intelligence   test && \
	make -C taocp          test && \
	echo "We are good"

clean:
	fd -I configure.mk -X rm
	go run ~/Workspace/y/tools/delete_unused_dirs.go
