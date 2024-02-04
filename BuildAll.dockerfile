FROM fyneio/fyne-cross-images:linux as builder
RUN apt-get update && apt-get install -y build-essential cmake yasm nasm pkg-config git gcc-mingw-w64-x86-64 gcc-mingw-w64-i686 mingw-w64

# Copy helpers
COPY toolchains/* /app/

# Clone the libjpeg-turbo repository
RUN git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git /app/libjpeg-turbo

# Build libjpeg-turbo for Linux
WORKDIR /app/libjpeg-turbo/build-linux
RUN cmake .. -G"Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SHARED=OFF \
    -DCMAKE_INSTALL_PREFIX=/opt/libjpeg/linux \
    -DCMAKE_TOOLCHAIN_FILE=/app/toolchain-linux.cmake && \
    make && \
    make install

# Build libjpeg-turbo for Windows
WORKDIR /app/libjpeg-turbo/build-windows
RUN cmake .. -G"Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SHARED=OFF \
    -DCMAKE_INSTALL_PREFIX=/opt/libjpeg/windows \
    -DCMAKE_TOOLCHAIN_FILE=/app/toolchain-windows.cmake && \
    make && \
    make install
