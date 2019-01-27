#/usr/bin/env sh
wget http://downloads.webmproject.org/releases/webp/libwebp-1.0.2.tar.gz
tar xvzf libwebp-1.0.2.tar.gz
cd libwebp-1.0.2
./configure --prefix=$HOME/opt
make && make install
