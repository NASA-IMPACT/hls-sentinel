### The file dependency of my code is crazy:
### cubic_conv is not needed in this code, but just because
### a header file is included which requires cubic_conv defintion.

TGT = derive_s2nbar		# Directory names begins with capital L; avoid replicate.
OBJ = 	derive_s2nbar.o\
	s2at30m.o \
	s2ang.o \
	hls_projection.o\
	mean_solarzen.o \
	local_solar.o \
	rtls.o \
	hdfutility.o\
	util.o \
	cubic_conv.o \
	cfactor.o

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB)  -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK)  $(HDFLINK) 

derive_s2nbar.o: derive_s2nbar.c 
	$(CC) $(CFLAGS) -c derive_s2nbar.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

s2at30m.o: ${SRC_DIR}/s2at30m.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/s2at30m.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

s2ang.o: ${SRC_DIR}/s2ang.c 
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/s2ang.c -I$(HDFINC) -I$(SRC_DIR)

hls_projection.o: ${SRC_DIR}/hls_projection.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/hls_projection.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

mean_solarzen.o: ${SRC_DIR}/mean_solarzen.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/mean_solarzen.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

local_solar.o: ${SRC_DIR}/local_solar.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/local_solar.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

rtls.o: ${SRC_DIR}/rtls.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/rtls.c -I$(SRC_DIR)

hdfutility.o: ${SRC_DIR}/hdfutility.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/hdfutility.c -I$(HDFINC) -I$(SRC_DIR)

util.o: ${SRC_DIR}/util.c 
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/util.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

cubic_conv.o: ${SRC_DIR}/cubic_conv.c 
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/cubic_conv.c -I$(HDFINC) -I$(SRC_DIR)

cfactor.o: ${SRC_DIR}/cfactor.c 
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/cfactor.c -I$(GCTPINC) -I$(HDFINC) -I$(SRC_DIR)

install:
	install -m 755 $(TGT) /usr/bin

clean:
	rm -f *.o
