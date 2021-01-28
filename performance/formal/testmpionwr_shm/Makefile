#
# windriver parallel computing platform.
#
LDFLAGS = -melf_x86_64 -pthread  -m64 \
                  -static_mpi

ALL = emt1

MPICC = mpiicc

THREAD_SAFE=-parallel

#WRAPILIB=-lwrapi

#WINDRIVER=-D_windriver

YJAPILIB=-lyjapi

YUANJI=-D_yuanji

all : $(ALL)

objects = emt.o shmFunc.o MPI_Comm.o IOComm_Proxy.o #$(WRAPILIB)

emt1: $(objects)
	$(MPICC) -O2 -o $@ $^ $(WITH_DEBUG) -static_mpi $(THREAD_SAFE) $(LDFLAGS)
   
emt.o: emt.c
	$(MPICC) -O2 -D_LINUX $(DPDK_FLAG) -std=c99 -fPIC $(WITH_DEBUG) $(YUANJI) -c -D_GNU_SOURCE $^

shmFunc.o: shmFunc.c
	$(MPICC) -O2 -D_LINUX $(DPDK_FLAG) -O2 -std=c99 -fPIC $(WITH_DEBUG) $(WINDRIVER) -c $^	

MPI_Comm.o: MPI_Comm.cpp
	$(MPICC) -O2 -D_LINUX $(DPDK_FLAG) -fPIC $(WITH_DEBUG) $(WINDRIVER) -c -D_GNU_SOURCE $^

IOComm_Proxy.o: IOComm_Proxy.cpp
	$(MPICC) -O2 -D_LINUX $(DPDK_FLAG) -O2 -fPIC $(WITH_DEBUG) $(WINDRIVER) -c $^		
clean:
	@rm -f $(ALL)  *.o *.map log 

