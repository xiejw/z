test:
	make -C intelligence   test && \
	make -C taocp          test && \
	echo "We are good"

fmt:
	make -C intelligence   fmt  && \
	make -C taocp          fmt  && \
	echo "We are good"

clean:
	fd -I configure.mk -X rm
	go run ~/Workspace/y/tools/scripts/delete_unused_dirs.go
