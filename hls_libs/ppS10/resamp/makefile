# Note: before run make, do in the shell:
# source $HOME/code/share/makeenv/makeenv.sh

TGT = resamp_s2
OBJ = 	resamp_s2.o \
	s2r.o \
	hdfutility.o \
	util.o \
	read_arop_output.o \
	cubic_conv.o \
	hls_hdfeos.o
	
$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK) $(HDFLINK) 

resamp_s2.o: resamp_s2.c 
	$(CC) $(CFLAGS) -c resamp_s2.c -I$(HDFINC) -I$(GCTPINC)

s2r.o: s2r.c
	$(CC) $(CFLAGS) -c s2r.c -I$(HDFINC) -I$(GCTPINC)

hdfutility.o: hdfutility.c
	$(CC) $(CFLAGS) -c  hdfutility.c -I$(HDFINC)

util.o: util.c
	$(CC) $(CFLAGS) -c  util.c -I$(HDFINC)

read_arop_output.o: read_arop_output.c
	$(CC) $(CFLAGS) -c read_arop_output.c -I$(HDFINC)

cubic_conv.o: cubic_conv.c
	$(CC) $(CFLAGS) -c cubic_conv.c -I$(HDFINC)

hls_hdfeos.o: hls_hdfeos.c
	$(CC) $(CFLAGS) -c  hls_hdfeos.c -I$(HDFINC)

install:
	install -m 755 $(TGT) $(PREFIX)/bin

clean:
	rm -f *.o
