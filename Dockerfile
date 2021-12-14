FROM 018923174646.dkr.ecr.us-west-2.amazonaws.com/hls-base-3.2.0
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
    ECS_ENABLE_TASK_IAM_ROLE=true \
    PYTHONPATH="${PYTHONPATH}:${PREFIX}/lib/python3.6/site-packages" \
    ACCODE=LaSRCL8V3.5.5 \
    LC_ALL=en_US.utf-8 \
    LANG=en_US.utf-8

# The Python click library requires a set locale
RUN localedef -i en_US -f UTF-8 en_US.UTF-8

# Move common files to source directory
COPY ./hls_libs/common $SRC_DIR

# Move and compile addFmaskSDS
COPY ./hls_libs/addFmaskSDS ${SRC_DIR}/addFmaskSDS
RUN cd ${SRC_DIR}/addFmaskSDS \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf addFmaskSDS

# Move and compile twohdf2one
COPY ./hls_libs/twohdf2one ${SRC_DIR}/twohdf2one
RUN cd ${SRC_DIR}/twohdf2one \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf twohdf2one

# Move and compile consolidate
COPY ./hls_libs/consolidate ${SRC_DIR}/consolidate
RUN cd ${SRC_DIR}/consolidate \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf consolidate

# Move and compile derive_s2ang
COPY ./hls_libs/derive_s2ang ${SRC_DIR}/derive_s2ang
RUN cd ${SRC_DIR}/derive_s2ang \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf derive_s2ang

# Move and compile derive_s2ang
COPY ./hls_libs/consolidate_s2ang ${SRC_DIR}/consolidate_s2ang
RUN cd ${SRC_DIR}/consolidate_s2ang \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf consolidate_s2ang

# Move and compile create_s2at30m
COPY ./hls_libs/create_s2at30m ${SRC_DIR}/create_s2at30m
RUN cd ${SRC_DIR}/create_s2at30m \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf create_s2at30m

# Move and compile derive_s2nbar
COPY ./hls_libs/derive_s2nbar ${SRC_DIR}/derive_s2nbar
RUN cd ${SRC_DIR}/derive_s2nbar \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf derive_s2nbar

# Move and compile L8like
COPY ./hls_libs/L8like ${SRC_DIR}/L8like
RUN cd ${SRC_DIR}/L8like \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf L8like

# Move and compile s2trim
COPY ./hls_libs/trim ${SRC_DIR}/trim
RUN cd ${SRC_DIR}/trim \
    && make \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf trim

COPY ./hls_libs/L8like/bandpass_parameter.S2A.txt ${PREFIX}/bandpass_parameter.S2A.txt
COPY ./hls_libs/L8like/bandpass_parameter.S2B.txt ${PREFIX}/bandpass_parameter.S2B.txt

RUN pip3 install --upgrade awscli
RUN pip3 install click==7.1.2
RUN pip3 install rio-cogeo==1.1.10 --no-binary rasterio --user

RUN pip3 install git+https://github.com/NASA-IMPACT/hls-thumbnails@v1.0

RUN pip3 install git+https://github.com/NASA-IMPACT/hls-metadata@v1.9

RUN pip3 install git+https://github.com/NASA-IMPACT/hls-manifest@v2.0

RUN pip3 install wheel
RUN pip3 install git+https://github.com/NASA-IMPACT/hls-browse_imagery@v1.5
RUN pip3 install libxml2-python3
RUN pip3 install git+https://github.com/NASA-IMPACT/hls-hdf_to_cog@v1.4
RUN pip3 install git+https://github.com/NASA-IMPACT/hls-utilities@v1.8
RUN pip3 install git+https://github.com/NASA-IMPACT/hls-cmr_stac@v1.4

COPY ./scripts/* ${PREFIX}/bin/
ENV OMP_NUM_THREADS=4
ENTRYPOINT ["/bin/sh", "-c"]
CMD ["sentinel.sh"]

