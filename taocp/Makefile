MODS     += v4_7.1.1.algo_c_core_computation_for_definite_horn
MODS     += v4_7.1.1.them_k_2sat_krom_clauses

MODS     += v4_7.2.2.algo_b_basic_backtrack
MODS     += v4_7.2.2.algo_b_basic_backtrack_bit_vec
MODS     += v4_7.2.2.algo_w_walker_backtrack

MODS     += v4_7.2.2.4.algo_h_all_hamiltonian_cycles

MODS     += v4_7.4.1.2.algo_t_strong_components

# MODS     += v4_dlink
# MODS     += v4_sat

compile:
	@for m in $(MODS); do        \
		make -C $$m compile; \
		done

test:
ifdef ASAN
	@for m in $(MODS); do        \
		make -C $$m ASAN=1 test; \
		done
else
	@for m in $(MODS); do        \
		make -C $$m test;    \
		done
endif

fmt:
	@for m in $(MODS); do        \
		make -C $$m fmt; \
		done

clean:
	@for m in $(MODS); do        \
		make -C $$m clean; \
		done

release:
	@for m in $(MODS); do        \
		make -C $$m release; \
		done
