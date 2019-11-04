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

link:
	ln -sf ../../../share/common/fillval.h
	ln -sf ../../../share/common/hls_commondef.h
	ln -sf ../../../share/common/hls_projection.h
	ln -sf ../../../share/common/read_arop_output.h
	ln -sf ../../../share/common/read_arop_output.c
	ln -sf ../../../share/common/cubic_conv.h
	ln -sf ../../../share/common/cubic_conv.c
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
