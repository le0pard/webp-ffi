#/usr/bin/env sh
wget https://webp.googlecode.com/files/libwebp-0.2.1.tar.gz
tar xvzf libwebp-0.2.1.tar.gz
cd libwebp-0.2.1
./configure
make && sudo make install