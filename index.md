# Webp-ffi

[![Build](https://github.com/le0pard/webp-ffi/actions/workflows/build.yml/badge.svg)](https://github.com/le0pard/webp-ffi/actions/workflows/build.yml)

Ruby wrapper for libwebp. What is WebP?

WebP is a new image format that provides lossless and lossy compression for images on the web. WebP lossless images are 26% smaller in size compared to PNGs. WebP lossy images are 25-34% smaller in size compared to JPEG images at equivalent SSIM index. WebP supports lossless transparency (also known as alpha channel) with just 22% additional bytes. Transparency is also supported with lossy compression and typically provides 3x smaller file sizes compared to PNG when lossy compression is acceptable for the red/green/blue color channels.

## What's new in 1.0

* **Prebuilt gems** for Linux (glibc and musl, x86_64 and arm64) and macOS (Apple Silicon and Intel): no compiler or system libraries needed
* **Encode and decode in memory** with `WebP.encode_data` and `WebP.decode_data`, and **raw RGBA pixels** with `WebP.encode_rgba` and `WebP.decode_rgba`
* **`WebP.info`** for size, format, alpha, animation and metadata
* **Metadata**: keep ICC color profiles, EXIF and XMP when encoding, byte for byte like libwebp's `cwebp`
* **More encoder options**: presets, lossless levels, sharp YUV, exact transparency and more
* **Other threads keep running** while images are encoded or decoded
* **Clearer errors** with libwebp's reason, and `ArgumentError` for unknown options
* **Fixes** for `near_lossless`, `target_size`, lossless encoding of JPEG and TIFF input, TIFF transparency and more

Version 1.0 needs Ruby 3.2 or newer and, for the source gem, libwebp 1.0.0 or newer. Some behavior changed: see the [changelog](https://github.com/le0pard/webp-ffi/blob/master/CHANGELOG.md) before upgrading.

## [WebP Gallery](https://developers.google.com/speed/webp/gallery)

## Installation

### Prebuilt gems

On these platforms `gem install webp-ffi` (or Bundler) picks a prebuilt gem, so nothing is compiled and no system libraries are needed:

 * Linux with glibc 2.28 or newer: `x86_64-linux-gnu`, `aarch64-linux-gnu`
 * Linux with musl (Alpine): `x86_64-linux-musl`, `aarch64-linux-musl`
 * macOS 11 or newer: `arm64-darwin` (Apple Silicon), `x86_64-darwin` (Intel)

The prebuilt gems include libwebp 1.6.0 together with libpng, libjpeg-turbo, libtiff and zlib. Their versions and licenses are in the gem, next to the library (`ext/webp_ffi/<platform>/versions.txt` and `licenses/`).

If your `Gemfile.lock` lists only the `ruby` platform, add yours, for example:

    bundle lock --add-platform x86_64-linux-gnu

To build against the system libraries instead, install the source gem:

    gem install webp-ffi --platform ruby

or in a `Gemfile`:

    gem "webp-ffi", force_ruby_platform: true

The rest of this section applies to the source gem.

### Requirements

First of all you should have install libraries: libpng, libjpeg and libtiff.

For Ubuntu, Debian:

    sudo apt-get install libjpeg-dev libpng-dev libtiff-dev libwebp-dev

For Fedora, CentOS:

    sudo dnf install libjpeg-devel libpng-devel libtiff-devel libwebp-devel

For Mac OS:

    brew install webp libpng jpeg-turbo libtiff

or (for MacPorts):

    sudo port install webp jpeg libpng tiff

The build finds libraries installed by Homebrew (`/opt/homebrew` or `/usr/local`) and MacPorts (`/opt/local`). For any other location, pass the paths with `C_FLAGS` and `LD_FLAGS`:

    C_FLAGS="-I/path/to/include" LD_FLAGS="-L/path/to/lib" gem install webp-ffi

Next, you should [install libwebp](https://developers.google.com/speed/webp/docs/compiling) (if you didn't install it by `brew` in Mac OS or by `apt-get` in Ubuntu or Debian). The source gem needs libwebp 1.0.0 or newer. This gem does not support Windows.

### Final part

Add this line to your application's Gemfile:

    gem 'webp-ffi'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install webp-ffi

## Usage

### Encoder and Decoder versions

Basic info about libwebp (encoder and decoder versions):

    require 'webp_ffi'
    WebP.decoder_version # => "1.6.0"
    WebP.encoder_version # => "1.6.0"


### WebP size and details

Get the size (width and height) of a WebP image:

    WebP.webp_size(File.binread("photo.webp")) # => [1024, 772]

`WebP.info` returns more details as a `WebP::Info`:

    info = WebP.info(File.binread("photo.webp"))
    info.width      # => 1024
    info.height     # => 772
    info.format     # => :lossy, :lossless or :mixed (animations)
    info.alpha?     # => false
    info.animated?  # => false
    info.metadata   # => [:icc, :exif] (metadata chunks in the file)

Both accept a binary String or an IO, and raise `WebP::InvalidImageFormatError` for data that is not WebP.


### Encode WebP image

Encode png, jpg or tiff image to webp (only the first page of a multi-page tiff is used):

    filename = File.expand_path(File.join(File.dirname(__FILE__), "spec/factories/4.png"))
    out_filename = File.expand_path(File.join(File.dirname(__FILE__), "tmp/4.webp"))
    WebP.encode(filename, out_filename)


Encode png, jpg or tiff image to webp with options:

    WebP.encode(filename, out_filename, quality: 50, resize_w: 100, resize_h: 200)
    WebP.encode(filename, out_filename, quality: 75, crop_x: 0, crop_y: 0, crop_w: 100, crop_h: 100)


File names can be strings or `Pathname` objects.

Integer options also accept `true` and `false` (as 1 and 0). Unknown option names and values that are not numbers raise `ArgumentError`, so typos are easy to spot.

Possible encode options:

 * **preset** (symbol) - start from libwebp's settings for a kind of image: `:default`, `:picture`, `:photo`, `:drawing`, `:icon` or `:text`. Other options still apply on top
 * **lossless** (int) - Lossless encoding (0=lossy(default), 1=lossless)
 * **level** (int) - lossless preset from 0 (fastest) to 9 (smallest), like `cwebp -z`. Turns on lossless encoding and picks its own quality and method unless you set them
 * **near_lossless** (int) - Use near-lossless image preprocessing (0=maximum preprocessing, 100=no preprocessing(default)). Turns on lossless encoding
 * **quality** (float) - between 0 (smallest file) and 100 (biggest). Default is 100
 * **method** (int) - quality/speed trade-off (0=fast, 6=slower-better)
 * **target\_size** (int) - if non-zero, set the desired target size in bytes. Takes precedence over the 'compression' parameter. Like cwebp, 6 passes are used unless **pass** is set
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
 * **partitions** (int) - log2(number of token partitions) in [0..3]. Default is set to 0 for easier progressive decoding. libwebp only uses it together with **low\_memory**
 * **partition\_limit** (int) - quality degradation allowed to fit the 512k limit on prediction modes coding (0: no degradation, 100: maximum possible degradation)
 * **image\_hint** (symbol) - kind of image for lossless encoding: `:default`, `:picture`, `:photo` or `:graph`
 * **exact** (bool) - keep the RGB values under transparent areas, which libwebp otherwise changes to compress better
 * **use\_sharp\_yuv** (bool) - sharper (and slower) RGB to YUV conversion for lossy encoding
 * **thread\_level** (bool) - use more than one thread where libwebp can
 * **low\_memory** (bool) - use less memory and more CPU
 * **emulate\_jpeg\_size** (bool) - pick settings that give about the size of a JPEG of the same quality
 * **metadata** - copy metadata from the input: `:all`, `:none` (default) or some of `:icc`, `:exif` and `:xmp` (see below)
 * **width** (int), **height** (int) - Input size (width x height) for YUV
 * **crop\_x** (int), **crop\_y** (int), **crop\_w** (int), **crop\_h** (int) - crop picture with the given rectangle
 * **resize\_w** (int), **resize\_h** (int) - resize picture (after any cropping)

### Metadata

By default, like `cwebp`, no metadata is copied. The **metadata** option copies the ICC color profile, EXIF and XMP data from the input into the WebP file:

    WebP.encode("photo.jpg", "photo.webp", metadata: :all)
    WebP.encode_data(File.binread("photo.png"), metadata: [:icc])

PNG and JPEG files can carry all three. In PNG files the gem reads the iCCP and eXIf chunks, XMP text and the "raw profile" text chunks that ImageMagick and ExifTool write. Like `cwebp`, only the ICC profile and XMP data are read from TIFF files, and broken metadata fails the encode only when it was requested.

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

`:yuv` writes the raw planes (Y, then U and V at half size, then alpha when the image has it), which `WebP.encode_data` reads back with the `width` and `height` options. `:pgm` writes the same planes as a grayscale PGM image.

The output file is written only after decoding succeeds, and encoding reads the whole input first, so the input and output can be the same file. When writing fails, a file the gem created is removed again.


Decode webp image with options:

    WebP.decode(filename, out_filename, resize_w: 100, resize_h: 200)
    WebP.decode(filename, out_filename, crop_x: 0, crop_y: 0, crop_w: 100, crop_h: 100)


Possible decode options (unknown options and an unknown `output_format` raise `ArgumentError`):

 * **bypass\_filtering** (bool) - disable in-loop filtering
 * **no\_fancy\_upsampling** (bool) - don't use the fancy YUV420 upscaler
 * **use\_threads** (bool) - use multi-threading
 * **crop\_x** (int), **crop\_y** (int), **crop\_w** (int), **crop\_h** (int) - crop picture with the given rectangle
 * **resize\_w** (int), **resize\_h** (int) - resize picture (after any cropping)

### Encode and decode in memory

`WebP.encode_data` and `WebP.decode_data` work with binary strings instead of files, for example for uploads or images stored in S3. They take the same options as `WebP.encode` and `WebP.decode`.

    webp = WebP.encode_data(File.binread("photo.jpg"), quality: 80)
    WebP.webp_size(webp) # => [1024, 772]

    png = WebP.decode_data(webp)
    tiff = WebP.decode_data(webp, output_format: :tiff, resize_w: 200, resize_h: 150)

`encode_data` reads PNG, JPEG and TIFF data, or raw YUV 4:2:0 data when `width` and `height` are given. Both methods also accept an IO, such as an open file or a `StringIO`, and read it from its current position.

### Raw pixels

`WebP.encode_rgba` encodes raw RGBA pixels (4 bytes per pixel, row after row), and `WebP.decode_rgba` returns them, for example to work with ChunkyPNG, libvips or OpenCV:

    webp = WebP.encode_rgba(pixels, width, height, lossless: true)
    image = WebP.decode_rgba(webp, resize_w: 200, resize_h: 150)
    image.rgba    # => binary String of 200 * 150 * 4 bytes
    image.width   # => 200
    image.height  # => 150

They take the same options as `encode_data` and `decode_data`, except the ones for files: **width**, **height**, **metadata** and **output\_format**.

### Errors

Every error the gem raises inherits from `WebP::Error`. `WebP::EncoderError` and `WebP::DecoderError` also carry the gem's error `code` and libwebp's `reason`, when it gives one:

    begin
      WebP.encode("huge.png", "huge.webp")
    rescue WebP::EncoderError => e
      e.message # => "Cannot encode picture as WebP (the picture size is not supported, WebP allows up to 16383 x 16383 pixels)"
      e.reason  # => :bad_dimension
    end

Invalid arguments, like unknown options, raise `ArgumentError` or `TypeError`.

### Threads

`WebP.encode`, `WebP.decode` and the in-memory and raw pixel methods release Ruby's Global VM Lock while libwebp works, so other threads (for example in Puma or Sidekiq) keep running during long conversions.

### Type signatures

The gem ships RBS type signatures in [`sig/webp.rbs`](https://github.com/le0pard/webp-ffi/blob/master/sig/webp.rbs) for Steep and editors with RBS support.

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
