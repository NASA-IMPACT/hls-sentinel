# Note: before run make, do in the shell:
# source /home/jju/code/share/makeenv/makeenv.sh

TGT = consolidate
OBJ = 	consolidate.o \
	s2r.o \
	hdfutility.o \
	util.o \
	hls_hdfeos.o
	
$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK) $(HDFLINK)

consolidate.o: consolidate.c 
	$(CC) $(CFLAGS) -c consolidate.c -I$(HDFINC) -I$(GCTPINC)

s2r.o: s2r.c
	$(CC) $(CFLAGS) -c s2r.c -I$(HDFINC) -I$(GCTPINC)

hdfutility.o: hdfutility.c
	$(CC) $(CFLAGS) -c  hdfutility.c -I$(HDFINC)

util.o: util.c
	$(CC) $(CFLAGS) -c  util.c -I$(HDFINC)

hls_hdfeos.o: hls_hdfeos.c
	$(CC) $(CFLAGS) -c  hls_hdfeos.c -I$(HDFINC)

install:
	install -m 755 $(TGT) $(PREFIX)/bin

clean:
	rm -f *.o
