compile:
	make -C v4_horn  compile
	make -C v4_dlink compile

test:
ifdef ASAN
	make -C v4_horn  ASAN=1 test
	make -C v4_dlink ASAN=1 test
	make -C v4_sat   ASAN=1 test
else
	make -C v4_horn  test
	make -C v4_dlink test
	make -C v4_sat   test
endif

fmt:
	make -C v4_horn  fmt
	make -C v4_dlink fmt

clean:
	make -C v4_horn  clean
	make -C v4_dlink clean

release:
	make -C v4_horn  release
	make -C v4_dlink release
