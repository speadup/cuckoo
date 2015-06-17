#test makefile

all:main.o
	gcc -o test_public main.o ../mempool.o ../cpu_info.o ../stdyeetec.o ../pthread_affinity.o \
	../../lib/yt_lib.o -L ../../dpdk/usr/lib64 -lintel_dpdk -lrt -ldl -lm -lpthread  \
	-Wl,-rpath,../../dpdk/usr/lib64

main.o:
	gcc -c main.c -I ../

clean:
	rm -f *.o test_public
