# Webp-ffi

[![Build](https://github.com/le0pard/webp-ffi/actions/workflows/build.yml/badge.svg)](https://github.com/le0pard/webp-ffi/actions/workflows/build.yml)

Ruby wrapper for libwebp. What is WebP?

WebP is a new image format that provides lossless and lossy compression for images on the web. WebP lossless images are 26% smaller in size compared to PNGs. WebP lossy images are 25-34% smaller in size compared to JPEG images at equivalent SSIM index. WebP supports lossless transparency (also known as alpha channel) with just 22% additional bytes. Transparency is also supported with lossy compression and typically provides 3x smaller file sizes compared to PNG when lossy compression is acceptable for the red/green/blue color channels.

## [WebP Gallery](https://developers.google.com/speed/webp/gallery)

## Installation

### Requirements

First of all you should have install libraries: libpng, libjpeg and libtiff.

For Ubuntu, Debian:

    sudo apt-get install libjpeg-dev libpng-dev libtiff-dev libwebp-dev

For Fedora, CentOS:

    sudo dnf install libjpeg-devel libpng-devel libtiff-devel libwebp-devel

For Mac OS:

    brew install libjpg libpng libtiff webp

or (for MacPorts):

    sudo port install jpeg libpng tiff

Next, you should [install libwebp](https://developers.google.com/speed/webp/docs/compiling) (if you didn't install it by `brew` in Mac OS or by `apt-get` in Ubuntu or Debian). Webp library version should be >= 0.3.0. This gem is not support Windows systems.

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
     => "0.3.1"
    2.0.0p0 :003 > WebP.encoder_version
     => "0.3.1"


### WebP size (width and height)

Get size (width and height) from webp image:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    WebP.webp_size(File.open(filename, "rb").read)
     => [2000, 2353]


### Encode WebP image

Encode png, jpg or tiff image to webp:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.png"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.webp"))
    WebP.encode(filename, out_filename)


Encode png, jpg or tiff image to webp with options:

    WebP.encode(filename, out_filename, quality: 50, resize_w: 100, resize_h: 200)
    WebP.encode(filename, out_filename, quality: 75, crop_x: 0, cropt_y: 0, crop_w: 100, crop_h: 100)


Possible encode options:

 * **lossless** (int) - Lossless encoding (0=lossy(default), 1=lossless)
 * **near_lossless** (int) - Use near-lossless image preprocessing (0=maximum preprocessing, 100=no preprocessing(default))
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

### Decode WebP image

Decode webp image (default format is png):

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.png"))
    WebP.decode(filename, out_filename)


Decode webp image to pam, ppm, pgm, bmp, tiff or yuv format of image:

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.webp"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.png"))
    WebP.decode(filename, out_filename, output_format: :pam)
    WebP.decode(filename, out_filename, output_format: :ppm)
    WebP.decode(filename, out_filename, output_format: :pgm)
    WebP.decode(filename, out_filename, output_format: :bmp)
    WebP.decode(filename, out_filename, output_format: :tiff)
    WebP.decode(filename, out_filename, output_format: :yuv)


Decode webp image with options:

    WebP.encode(filename, out_filename, resize_w: 100, resize_h: 200)
    WebP.encode(filename, out_filename, crop_x: 0, cropt_y: 0, crop_w: 100, crop_h: 100)


Possible decode options:

 * **bypass\_filtering** (bool) - disable in-loop filtering
 * **no\_fancy\_upsampling** (bool) - don't use the fancy YUV420 upscaler
 * **use\_threads** (bool) - use multi-threading
 * **crop\_x** (int), **crop\_y** (int), **crop\_w** (int), **crop\_h** (int) - crop picture with the given rectangle
 * **resize\_w** (int), **resize\_h** (int) - resize picture (after any cropping)

## Rails assets pipeline integration

For integration with Rails 3+ you can use very simple rake task:

```ruby
# Place this code in lib/tasks/assets.rake
require 'webp-ffi'

namespace :assets do
  desc "Create .webp versions of assets"
  task :webp => :environment do
    image_types = /\.(?:png|jpe?g)$/

    public_assets = File.join(
      Rails.root,
      "public",
      Rails.application.config.assets.prefix)

    Dir["#{public_assets}/**/*"].each do |filename|
      next unless filename =~ image_types

      mtime = File.mtime(filename)
      webp_file = "#{filename}.webp"
      next if File.exist?(webp_file) && File.mtime(webp_file) >= mtime
      begin
        WebP.encode(filename, webp_file)
        File.utime(mtime, mtime, webp_file)
        puts "Webp converted image #{webp_file}"
      rescue => e
        puts "Webp convertion error of image #{webp_file}. Error info: #{e.message}"
      end
    end
  end

  # Hook into existing assets:precompile task
  Rake::Task["assets:precompile"].enhance do
    Rake::Task["assets:webp"].invoke
  end
end
```

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
