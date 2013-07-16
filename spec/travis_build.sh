#/usr/bin/env sh
wget http://webp.googlecode.com/files/libwebp-0.3.1.tar.gz
tar xvzf libwebp-0.3.1.tar.gz
cd libwebp-0.3.1
./configure
make
sudo make install
sudo ln -fs /usr/local/lib/libwebp.* /usr/lib/
