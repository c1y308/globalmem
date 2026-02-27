ARCH := x86_64
GCC := /usr/bin/gcc
FLAGS := 
CROSS_COMPILE :=

test_app: test_app.c
	$(CROSS_COMPILE)$(GCC) $(FLAGS) -o test_app test_app.c

clean:
	rm -f test_app