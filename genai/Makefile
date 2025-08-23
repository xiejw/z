compile:
	make -C gan  compile

test:
ifdef ASAN
	make -C gan  ASAN=1 test
else
	make -C gan  test
endif

fmt:
	make -C gan  fmt

clean:
	make -C gan  clean

release:
	make -C gan  release
