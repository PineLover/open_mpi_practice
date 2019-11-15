all: pb4_mpi_scan pb5_serial pb5_mpi

pb4_mpi_scan: pb4_mpi_scan_and_sendrecv.c
	mpicc pb4_mpi_scan_and_sendrecv.c -o pb4_mpi_scan

pb5_mpi: pb5_mpi.c
	mpicc pb5_mpi.c -o pb5_mpi

pb5_serial: pb5_serial.c
	gcc  pb5_serial.c -o pb5_serial

clean:
	rm pb4_mpi_scan pb5_serial pb5_mpi
