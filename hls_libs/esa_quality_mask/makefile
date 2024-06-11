# Note: before run make, do in the shell:
# source $HOME/code.toa/share/makeenv/makeenv.sh local

TGT = esa_quality_mask
OBJ = 	esa_quality_mask.o \
	hls_projection.o \
	s2r.o \
	util.o \
	hdfutility.o 
	
$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB)  $(GCTPLINK) $(HDFLINK) 

esa_quality_mask.o: esa_quality_mask.c 
	$(CC) $(CFLAGS) -c esa_quality_mask.c -I$(HDFINC) -I$(GCTPINC)

hls_projection.o: hls_projection.c
	$(CC) $(CFLAGS) -c hls_projection.c -I$(HDFINC) -I$(GCTPINC) 

s2r.o: s2r.c
	$(CC) $(CFLAGS) -c s2r.c -I$(HDFINC) -I$(GCTPINC)

util.o: util.c
	$(CC) $(CFLAGS) -c  util.c -I$(HDFINC)

hdfutility.o: hdfutility.c
	$(CC) $(CFLAGS) -c  hdfutility.c -I$(HDFINC)

link:
	ln -sf ../../common/s2r.h
	ln -sf ../../common/s2r.c
	ln -sf ../../common/s2def.h
	ln -sf ../../common/s2at30m.h		# included by hls_hdfeos.h 	
	ln -sf ../../../L8/common/lsat.h
	ln -sf ../../../share/common/cubic_conv.h 	# included by a2at30m.h
	ln -sf ../../../share/common/util.h
	ln -sf ../../../share/common/util.c
	ln -sf ../../../share/common/hdfutility.h
	ln -sf ../../../share/common/hdfutility.c
	ln -sf ../../../share/common/hls_projection.h
	ln -sf ../../../share/common/hls_projection.c
	ln -sf ../../../share/common/hls_hdfeos.h
	ln -sf ../../../share/common/hls_hdfeos.c
	ln -sf ../../../share/common/fillval.h
	ln -sf ../../../share/common/hls_commondef.h

clean:
	rm -f *.o

