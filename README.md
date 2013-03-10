# Webp-ffi

[![Build Status](https://travis-ci.org/le0pard/webp-ffi.png)](https://travis-ci.org/le0pard/webp-ffi) 
[![Code Climate](https://codeclimate.com/github/le0pard/webp-ffi.png)](https://codeclimate.com/github/le0pard/webp-ffi)

Ruby wrapper for libwebp. What is web-p?

WebP is a new image format that provides lossless and lossy compression for images on the web. WebP lossless images are 26% smaller in size compared to PNGs. WebP lossy images are 25-34% smaller in size compared to JPEG images at equivalent SSIM index. WebP supports lossless transparency (also known as alpha channel) with just 22% additional bytes. Transparency is also supported with lossy compression and typically provides 3x smaller file sizes compared to PNG when lossy compression is acceptable for the red/green/blue color channels.

## [WebP Gallery](https://developers.google.com/speed/webp/gallery)

## Installation

### Requirements

First of all you should have install libraries: libpng, libjpeg and libwebp (for libwebp need libpng and libjpeg).

For ubuntu, debian:

    sudo apt-get install libjpeg62-dev libpng12-dev libtiff4-dev
    
For Mac OS:

    sudo port install jpeg libpng tiff
    
or:

    brew install libjpg libpng libtiff
    
Next, you should [install libwebp](https://developers.google.com/speed/webp/docs/compiling).

### Final part

Add this line to your application's Gemfile:

    gem 'webp-ffi'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install webp-ffi

## Usage

Basic info about libwebp (encoder and decoder versions):

    $ irb
    2.0.0p0 :001 > require 'webp_ffi'
     => true 
    2.0.0p0 :002 > WebpFfi.decoder_version
     => "0.2.0" 
    2.0.0p0 :003 > WebpFfi.encoder_version
     => "0.2.0"
     
Get size (width and height) from webp image:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    WebpFfi.webp_size(File.open(filename, "rb").read)
     => [2000, 2353]
    
Decode webp image to png:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.png"))
    WebpFfi.decode(filename, out_filename)

Encode png, jpg or tiff image to webp:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.png"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.webp"))
    WebpFfi.encode(filename, out_filename)
     
Encode png, jpg or tiff image to webp (with options):

    WebpFfi.encode(filename, out_filename, quality: 50, resize_w: 100, resize_h: 200)
    WebpFfi.encode(filename, out_filename, quality: 75, crop_x: 0, cropt_y: 0, crop_w: 100, crop_h: 100)

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
