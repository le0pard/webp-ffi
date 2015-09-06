#/usr/bin/env sh
wget http://downloads.webmproject.org/releases/webp/libwebp-0.4.3.tar.gz
tar xvzf libwebp-0.4.3.tar.gz
cd libwebp-0.4.3
./configure --prefix=$HOME/opt
make && make install
