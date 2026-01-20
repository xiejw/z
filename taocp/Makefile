compile:
	make -C v4_horn                           compile
	make -C v4_dlink                          compile
	make -C v4_sat                            compile
	make -C v4_7.2.2.algo_b_basic_backtrack         compile
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec compile

test:
ifdef ASAN
	make -C v4_horn                   ASAN=1 test
	make -C v4_dlink                  ASAN=1 test
	make -C v4_sat                    ASAN=1 test
	make -C v4_7.2.2.algo_b_basic_backtrack ASAN=1 test
else
	make -C v4_horn                           test
	make -C v4_dlink                          test
	make -C v4_sat                            test
	make -C v4_7.2.2.algo_b_basic_backtrack         test
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec test
endif

fmt:
	make -C v4_horn                           fmt
	make -C v4_dlink                          fmt
	make -C v4_sat                            fmt
	make -C v4_7.2.2.algo_b_basic_backtrack         fmt
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec fmt

clean:
	make -C v4_horn                           clean
	make -C v4_dlink                          clean
	make -C v4_sat                            clean
	make -C v4_7.2.2.algo_b_basic_backtrack         clean
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec clean

release:
	make -C v4_horn                           release
	make -C v4_dlink                          release
	make -C v4_sat                            release
	make -C v4_7.2.2.algo_b_basic_backtrack         release
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec release
