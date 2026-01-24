compile:
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn             compile
	make -C v4_dlink                                                       compile
	make -C v4_sat                                                         compile
	make -C v4_7.2.2.algo_b_basic_backtrack                                compile
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                        compile
	make -C v4_7.2.2.algo_w_walker_backtrack                               compile

test:
ifdef ASAN
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn      ASAN=1 test
	make -C v4_dlink                                                ASAN=1 test
	make -C v4_sat                                                  ASAN=1 test
	make -C v4_7.2.2.algo_b_basic_backtrack                         ASAN=1 test
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                 ASAN=1 test
	make -C v4_7.2.2.algo_w_walker_backtrack                        ASAN=1 test
else
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn             test
	make -C v4_dlink                                                       test
	make -C v4_sat                                                         test
	make -C v4_7.2.2.algo_b_basic_backtrack                                test
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                        test
	make -C v4_7.2.2.algo_w_walker_backtrack                               test
endif

fmt:
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn             fmt
	make -C v4_dlink                                                       fmt
	make -C v4_sat                                                         fmt
	make -C v4_7.2.2.algo_b_basic_backtrack                                fmt
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                        fmt
	make -C v4_7.2.2.algo_w_walker_backtrack                               fmt

clean:
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn             clean
	make -C v4_dlink                                                       clean
	make -C v4_sat                                                         clean
	make -C v4_7.2.2.algo_b_basic_backtrack                                clean
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                        clean
	make -C v4_7.2.2.algo_w_walker_backtrack                               clean

release:
	make -C v4_7.1.1.algo_c_core_computation_for_definite_horn             release
	make -C v4_dlink                                                       release
	make -C v4_sat                                                         release
	make -C v4_7.2.2.algo_b_basic_backtrack                                release
	make -C v4_7.2.2.algo_b_basic_backtrack_bit_vec                        release
	make -C v4_7.2.2.algo_w_walker_backtrack                               release
