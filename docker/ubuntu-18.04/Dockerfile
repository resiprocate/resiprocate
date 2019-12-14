FROM  ubuntu:18.04
LABEL maintainer "Gemfield <gemfield@civilnet.cn>"

RUN apt-get update && \
        DEBIAN_FRONTEND="noninteractive" \
        apt-get install -y vim git cmake g++ autoconf libtool gperf pkg-config binutils tzdata && \
        apt-get autoremove -y && \
        apt-get clean -y

ENV RESIP_DIR=/opt/resiprocate
WORKDIR /workspace
RUN git clone https://github.com/resiprocate/resiprocate
RUN cd resiprocate && \
          git submodule update --init --recursive && \
          autoreconf -fi && \
          ./configure --prefix=${RESIP_DIR} && \
          make -j && \
          make check && \
          make install

ENV C_INCLUDE_PATH ${RESIP_DIR}/include/:${C_INCLUDE_PATH}
ENV CPLUS_INCLUDE_PATH ${RESIP_DIR}/include/:${CPLUS_INCLUDE_PATH}
ENV LIBRARY_PATH ${RESIP_DIR}/lib/:${LIBRARY_PATH}
ENV LD_LIBRARY_PATH ${RESIP_DIR}/lib/:${LD_LIBRARY_PATH}