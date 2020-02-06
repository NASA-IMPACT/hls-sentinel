TGT = derive_s2ang
OBJ = derive_s2ang.o \
	s2mapinfo.o \
	s2detfoo.o \
	s2ang.o \
	pnpoly.o \
	util.o \
	hdfutility.o

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(HDFLIB) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK) $(HDFLINK)

derive_s2ang.o: derive_s2ang.c
	$(CC) $(CFLAGS) -c derive_s2ang.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

s2mapinfo.o: ${SRC_DIR}/s2mapinfo.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/s2mapinfo.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

s2detfoo.o: ${SRC_DIR}/s2detfoo.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/s2detfoo.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

pnpoly.o: ${SRC_DIR}/pnpoly.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/pnpoly.c  -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

s2ang.o: ${SRC_DIR}/s2ang.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/s2ang.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

hdfutility.o: ${SRC_DIR}/hdfutility.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/hdfutility.c -I$(HDFINC) -I$(SRC_DIR)

util.o: ${SRC_DIR}/util.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/util.c -I$(HDFINC) -I$(SRC_DIR)

install:
	install -m 755 $(TGT) /usr/bin

clean:
	rm -f *.o

