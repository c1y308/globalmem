ARCH := x86_64
GCC := /usr/bin/gcc
FLAGS := 
CROSS_COMPILE :=

epoll_test: globalfifo_epoll.c
	$(CROSS_COMPILE)$(GCC) $(FLAGS) -o epoll_test globalfifo_epoll.c

poll_test: globalfifo_poll.c
	$(CROSS_COMPILE)$(GCC) $(FLAGS) -o poll_test globalfifo_poll.c

sigio_test: sigio_test.c
	$(CROSS_COMPILE)$(GCC) $(FLAGS) -o sigio_test sigio_test.c


clean:
	rm -f epoll_test poll_test