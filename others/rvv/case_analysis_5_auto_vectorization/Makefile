GNU ?= riscv64-unknown-elf

COPS = -S -O3 -march=rv64gcv -mrvv

all :
	$(GNU)-gcc $(COPS) rvv_test.c

clean:
	rm *.s