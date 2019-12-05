TGT = twohdf2one
OBJ = 	twohdf2one.o \
	hls_projection.o \
	s2r.o \
	util.o \
	hdfutility.o

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB)  $(GCTPLINK) $(HDFLINK) -g

twohdf2one.o: twohdf2one.c
	$(CC) $(CFLAGS) -c twohdf2one.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

hls_projection.o: ${SRC_DIR}/hls_projection.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/hls_projection.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

s2r.o: ${SRC_DIR}/s2r.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/s2r.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

util.o: ${SRC_DIR}/util.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/util.c -I$(HDFINC) -I$(SRC_DIR)

hdfutility.o: ${SRC_DIR}/hdfutility.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/hdfutility.c -I$(HDFINC) -I$(SRC_DIR)

install:
	install -m 755 $(TGT) /usr/bin

clean:
	rm -f *.o

