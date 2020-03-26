ARG AWS_ACCOUNT_ID
FROM ${AWS_ACCOUNT_ID}.dkr.ecr.us-west-2.amazonaws.com/hls-base:latest
ENV PREFIX=/usr/local \
    SRC_DIR=/usr/local/src \
    GCTPLIB=/usr/local/lib \
    HDFLIB=/usr/local/lib \
    ZLIB=/usr/local/lib \
    SZLIB=/usr/local/lib \
    JPGLIB=/usr/local/lib \
    PROJLIB=/usr/local/lib \
    HDFINC=/usr/local/include \
    GCTPINC=/usr/local/include \
    GCTPLINK="-lGctp -lm" \
    HDFLINK=" -lmfhdf -ldf -lm" \
		L8_AUX_DIR=/usr/local/src \
    ECS_ENABLE_TASK_IAM_ROLE=true \
    PYTHONPATH="${PREFIX}/lib/python2.7/site-packages" \
    ACCODE=LaSRCL8V3.5.5

# Move common files to source directory
COPY ./hls_libs/common $SRC_DIR

# Move and compile addFmaskSDS
COPY ./hls_libs/addFmaskSDS ${SRC_DIR}/addFmaskSDS
RUN cd ${SRC_DIR}/addFmaskSDS \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf addFmaskSDS

# Move and compile twohdf2one
COPY ./hls_libs/twohdf2one ${SRC_DIR}/twohdf2one
RUN cd ${SRC_DIR}/twohdf2one \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf twohdf2one

# Move and compile consolidate
COPY ./hls_libs/consolidate ${SRC_DIR}/consolidate
RUN cd ${SRC_DIR}/consolidate \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf consolidate

# Move and compile derive_s2ang
COPY ./hls_libs/derive_s2ang ${SRC_DIR}/derive_s2ang
RUN cd ${SRC_DIR}/derive_s2ang \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf derive_s2ang

# Move and compile derive_s2ang
COPY ./hls_libs/consolidate_s2ang ${SRC_DIR}/consolidate_s2ang
RUN cd ${SRC_DIR}/consolidate_s2ang \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf consolidate_s2ang

# Move and compile create_s2at30m
COPY ./hls_libs/create_s2at30m ${SRC_DIR}/create_s2at30m
RUN cd ${SRC_DIR}/create_s2at30m \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf create_s2at30m

# Move and compile derive_s2nbar
COPY ./hls_libs/derive_s2nbar ${SRC_DIR}/derive_s2nbar
RUN cd ${SRC_DIR}/derive_s2nbar \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf derive_s2nbar

# Move and compile L8like
COPY ./hls_libs/L8like ${SRC_DIR}/L8like
RUN cd ${SRC_DIR}/L8like \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes\
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf L8like

COPY ./hls_libs/L8like/bandpass_parameter.S2A.txt ${PREFIX}/bandpass_parameter.S2A.txt
COPY ./hls_libs/L8like/bandpass_parameter.S2B.txt ${PREFIX}/bandpass_parameter.S2B.txt

RUN pip install --upgrade git+https://github.com/USGS-EROS/espa-python-library.git@v1.1.0#espa

RUN pip install rio-cogeo==1.1.10 --no-binary rasterio --user

RUN pip install git+https://github.com/NASA-IMPACT/hls-thumbnails

RUN pip install git+https://github.com/NASA-IMPACT/hls-metadata-brian

COPY ./python_scripts/* ${PREFIX}/bin/

COPY ./scripts/* ${PREFIX}/bin/

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["sentinel.sh"]

