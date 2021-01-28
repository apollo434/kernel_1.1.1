#
# windriver parallel computing platform.
#
LDFLAGS = -melf_x86_64 -pthread  -m64 \
                  -static_mpi

ALL = emt1

MPICC = mpiicc

THREAD_SAFE=-parallel

WRAPILIB=-lwrapi

YJAPILIB=-lyjapi

YUANJI=-D_yuanji

#WINDRIVER=-D_windriver

all : $(ALL)

objects = emt.o MPI_Comm.o IOComm_Proxy.o $(YJAPILIB)

emt1: $(objects)
	$(MPICC)   -g -o $@ $^ $(WITH_DEBUG) -static_mpi $(THREAD_SAFE) $(LDFLAGS)
   
emt.o: emt.c
	$(MPICC) -g  -O2 -D_LINUX $(DPDK_FLAG) -std=c99 -fPIC $(WITH_DEBUG) $(YUANJI) $(WINDRIVER) -c -D_GNU_SOURCE $^

MPI_Comm.o: MPI_Comm.cpp
	$(MPICC) -g  -O2 -D_LINUX $(DPDK_FLAG) -fPIC $(WITH_DEBUG) $(WINDRIVER) -c -D_GNU_SOURCE $^

IOComm_Proxy.o: IOComm_Proxy.cpp
	$(MPICC) -g -D_LINUX $(DPDK_FLAG) -O2 -fPIC $(WITH_DEBUG) $(WINDRIVER) -c $^		

clean:
	@rm -f $(ALL)  *.o *.map log 

