# Webp-ffi

[![Build Status](https://travis-ci.org/le0pard/webp-ffi.png)](https://travis-ci.org/le0pard/webp-ffi) 
[![Code Climate](https://codeclimate.com/github/le0pard/webp-ffi.png)](https://codeclimate.com/github/le0pard/webp-ffi)

Ruby wrapper for libwebp.

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

    2.0.0p0 :001 > require 'webp_ffi'
     => true
    2.0.0p0 :006 > filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    2.0.0p0 :007 > WebpFfi.webp_size(File.open(filename, "rb").read)
     => [2000, 2353]
    
Decode webp image to png:

    2.0.0p0 :008 > filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
     => "/Users/leo/programs/projects/webp-ffi/spec/factories/4.webp" 
    2.0.0p0 :009 > out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.png"))
     => "/Users/leo/programs/projects/webp-ffi/tmp/4.png" 
    2.0.0p0 :010 > WebpFfi.decode(filename, out_filename)
     => 0 

Encode png, jpg or tiff image to webp:

    2.0.0p0 :011 > filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.png"))
     => "/Users/leo/programs/projects/webp-ffi/spec/factories/4.png" 
    2.0.0p0 :012 > out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.webp"))
     => "/Users/leo/programs/projects/webp-ffi/tmp/4.webp" 
    2.0.0p0 :013 > WebpFfi.encode(filename, out_filename)
     => 0

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
