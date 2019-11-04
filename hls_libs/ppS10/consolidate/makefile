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

link:
	ln -sf ../../../share/common/fillval.h
	ln -sf ../../../share/common/hls_commondef.h
	ln -sf ../../../share/common/hls_projection.h
	ln -sf ../../common/s2r.h
	ln -sf ../../common/s2r.c
	ln -sf ../../common/s2def.h
	ln -sf ../../common/s2at30m.h
	ln -sf ../../../share/common/hdfutility.h
	ln -sf ../../../share/common/hdfutility.c
	ln -sf ../../../share/common/util.h
	ln -sf ../../../share/common/util.c
	ln -sf ../../../share/common/hls_hdfeos.h
	ln -sf ../../../share/common/hls_hdfeos.c
	ln -sf ../../../L8/common/lsat.h

clean:
	rm -f *.o
