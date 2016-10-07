#/usr/bin/env sh
wget http://downloads.webmproject.org/releases/webp/libwebp-0.5.1.tar.gz
tar xvzf libwebp-0.5.1.tar.gz
cd libwebp-0.5.1
./configure --prefix=$HOME/opt
make && make install