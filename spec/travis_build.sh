#/usr/bin/env sh
wget http://downloads.webmproject.org/releases/webp/libwebp-0.4.2.tar.gz
tar xvzf libwebp-0.4.2.tar.gz
cd libwebp-0.4.2
./configure --prefix=$HOME/opt
make && make install
