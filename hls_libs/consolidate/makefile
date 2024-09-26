TGT = consolidate
OBJ = 	consolidate.o \
	s2r.o \
	hdfutility.o \
	util.o 

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB)  $(GCTPLINK) $(HDFLINK)

consolidate.o: consolidate.c
	$(CC) $(CFLAGS) -c consolidate.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

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

