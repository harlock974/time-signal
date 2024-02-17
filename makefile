PROGRAM=time-signal
$(PROGRAM) : $(PROGRAM).c
	gcc -Wall -O2 -s -o $(PROGRAM) -no-pie *.c
