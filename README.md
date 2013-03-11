# Webp-ffi

[![Build Status](https://travis-ci.org/le0pard/webp-ffi.png)](https://travis-ci.org/le0pard/webp-ffi) 
[![Code Climate](https://codeclimate.com/github/le0pard/webp-ffi.png)](https://codeclimate.com/github/le0pard/webp-ffi)

Ruby wrapper for libwebp. What is WebP?

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

### Encoder end Decoder versions

Basic info about libwebp (encoder and decoder versions):

    $ irb
    2.0.0p0 :001 > require 'webp_ffi'
     => true 
    2.0.0p0 :002 > WebP.decoder_version
     => "0.2.0" 
    2.0.0p0 :003 > WebP.encoder_version
     => "0.2.0"

### WebP size (width and height)
     
Get size (width and height) from webp image:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    WebP.webp_size(File.open(filename, "rb").read)
     => [2000, 2353]
    
### Encode png, jpg or tiff image to WebP image

Encode png, jpg or tiff image to webp:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.png"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.webp"))
    WebP.encode(filename, out_filename)
     
Encode png, jpg or tiff image to webp (with options):

    WebP.encode(filename, out_filename, quality: 50, resize_w: 100, resize_h: 200)
    WebP.encode(filename, out_filename, quality: 75, crop_x: 0, cropt_y: 0, crop_w: 100, crop_h: 100)
    
Possible encode options:

 * **lossless** (int) - Lossless encoding (0=lossy(default), 1=lossless)
 * **quality** (float) - between 0 (smallest file) and 100 (biggest)
 * **method** (int) - quality/speed trade-off (0=fast, 6=slower-better)
 * **target\_size** (int) - if non-zero, set the desired target size in bytes. Takes precedence over the 'compression' parameter
 * **target\_PSNR** (float) - if non-zero, specifies the minimal distortion to try to achieve. Takes precedence over target\_size
 * **segments** (int) - maximum number of segments to use, in [1..4]
 * **sns_strength** (int) - Spatial Noise Shaping. 0=off, 100=maximum
 * **filter\_strength** (int) - range: [0 = off .. 100 = strongest]
 * **filter\_sharpness** (int) - range: [0 = off .. 7 = least sharp]
 * **filter\_type** (int) - filtering type: 0 = simple, 1 = strong (only used if filter\_strength > 0 or autofilter > 0)
 * **autofilter** (int) - Auto adjust filter's strength [0 = off, 1 = on]
 * **alpha\_compression** (int) - Algorithm for encoding the alpha plane (0 = none, 1 = compressed with WebP lossless). Default is 1
 * **alpha\_filtering** (int) - Predictive filtering method for alpha plane. 0: none, 1: fast, 2: best. Default if 1
 * **alpha\_quality** (int) - Between 0 (smallest size) and 100 (lossless). Default is 100
 * **pass** (int) - number of entropy-analysis passes (in [1..10])
 * **show\_compressed** (int) - if true, export the compressed picture back. In-loop filtering is not applied
 * **preprocessing** (int) - preprocessing filter (0=none, 1=segment-smooth)
 * **partitions** (int) - log2(number of token partitions) in [0..3]. Default is set to 0 for easier progressive decoding
 * **partition\_limit** (int) - quality degradation allowed to fit the 512k limit on prediction modes coding (0: no degradation, 100: maximum possible degradation)
 * **width** (int), **height** (int) - Input size (width x height) for YUV
 * **crop\_x** (int), **crop\_y** (int), **crop\_w** (int), **crop\_h** (int) - crop picture with the given rectangle
 * **resize\_w** (int), **resize\_h** (int) - resize picture (after any cropping)
 
### Decode WebP image to png image

Decode webp image to png:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.png"))
    WebP.decode(filename, out_filename)

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
