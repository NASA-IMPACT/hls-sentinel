FROM 552819999234.dkr.ecr.us-east-1.amazonaws.com/hls-base:latest

ENV PREFIX=/usr/local \
    SRC_DIR=${PREFIX}/src \
    GCTPLIB=${PREFIX}/lib \
    ZLIB=${PREFIX}/lib \
    SZLIB=${PREFIX}/lib \
    JPGLIB=${PREFIX}/lib \
    PROJLIB=${PREFIX}/lib \
    GCTPINC=${PREFIX}/include \
    GCTPLINK="-lGctp -lm" \
    HDFINC=${PREFIX}/include \
    HDFLIB=${PREFIX}/lib \
    HDFLINK=" -lmfhdf -ldf -lproj -ljpeg -lz -lsz -lm" \
	L8_AUX_DIR=${PREFIX}/src \
    ECS_ENABLE_TASK_IAM_ROLE=true \
    PYTHONPATH="${PREFIX}/lib/python2.7/site-packages" \
    ESPAINC=${PREFIX}/include \
    ESPALIB=${PREFIX}/lib

# Move and compile consolidate
COPY ./hls_libs/ppS10/consolidate ${SRC_DIR}/ppS10/consolidate
RUN cd ${SRC_DIR}/ppS10/consolidate \
    && make clean \
    && make \
    && cp consolidate ${PREFIX}/bin/ \
    && cd $SRC_DIR \
    && rm -rf pps10/consolidate

# Move and compile resamp
COPY ./hls_libs/ppS10/resamp ${SRC_DIR}/ppS10/resamp
RUN cd ${SRC_DIR}/ppS10/resamp \
    && make clean \
    && make \
    && cp resamp_s2 ${PREFIX}/bin/ \
    && cd $SRC_DIR \
    && rm -rf pps10/resamp

# Move AROP
COPY ./hls_libs/ppS10/AROPonS2/run.AROPonS2.sh ${PREFIX}/bin/

COPY lasrc_sentinel_granule.sh ./usr/local/lasrc_sentinel_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/usr/local/lasrc_sentinel_granule.sh"]

