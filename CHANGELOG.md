## v1.0.0

Breaking changes:

* Require Ruby 3.2 or newer
* The source gem requires libwebp 1.0.0 or newer
* Raise `ArgumentError` for unknown options (they were ignored), with a suggestion for typos
* Raise `ArgumentError` for an unknown `output_format` instead of silently writing PNG
* Raise `ArgumentError` for option values that are not numbers
* `false` now means 0 for integer options, instead of the libwebp default
* `near_lossless` now works, so it produces lossless (larger) files; it was ignored before
* Decode raises `WebP::DecoderError` when the output file cannot be written; it reported success before
* Multi-page TIFF files use the first page instead of the last
* Remove the unused `WebP::LibC` module and `WebP::C.test_c`
* The functions in `WebP::C` changed; use the `WebP` methods instead

New:

* `WebP.encode_data` and `WebP.decode_data` encode and decode binary strings in memory
* `WebP.info` returns the size, format (lossy, lossless or mixed), alpha, animation and metadata of WebP data
* `WebP.encode_rgba` and `WebP.decode_rgba` work with raw RGBA pixels
* Copy ICC, EXIF and XMP metadata from PNG, JPEG and TIFF input with the `metadata` option, byte for byte like `cwebp -metadata`
* New encode options: `preset`, `level` (lossless preset), `image_hint`, `exact`, `use_sharp_yuv`, `thread_level`, `low_memory` and `emulate_jpeg_size`
* The in-memory, raw pixel and info methods also accept an IO
* `WebP::Error` is the base class of every error; encoder and decoder errors carry the error `code` and libwebp's `reason`, which is also in the message
* RBS type signatures in `sig/webp.rbs`
* Prebuilt gems for Linux (glibc and musl, x86_64 and aarch64) and macOS 11+ (arm64 and x86_64) with libwebp 1.6.0, libpng, libjpeg-turbo, libtiff and zlib included
* Release the Global VM Lock during encode and decode, so other Ruby threads keep running
* Accept `Pathname` and other objects with `#to_path` as file names
* Accept `true` and `false` for integer options, and the decode `output_format` as a string
* Include the file path in error messages about input and output files

Fixes:

* `target_size` and `target_PSNR` had no effect, because libwebp needs more than one pass; like cwebp, 6 passes are used now
* A failed encode left an empty output file, which the README's Rails task would then keep forever; the output is now written only after encoding succeeds
* `output_format: :yuv` wrote the same PGM file as `:pgm`; it now writes the raw YUV planes
* TIFF input with alpha lost color, because libtiff returns premultiplied pixels; they are now converted back
* Reading a PNG relied on undefined behavior (a signed left shift)
* Only regular files are read as input; on Linux a folder made the reader ask for a huge allocation
* JPEG and TIFF input was converted to YUV before lossless or sharp YUV encoding, so lossless output was not exact
* Check image sizes before allocating memory in the PNG, JPEG and TIFF readers
* Fix memory leaks when reading broken PNG and JPEG files
* Fix wrong error message when the decode input file cannot be read
* Detect write errors when closing output files
* Stop printing errors and warnings to stderr (libtiff older than 4.5.0 still prints its own)
* Always initialize every decode option passed to C

Build and release:

* Find libraries from Homebrew on Apple Silicon and from MacPorts without extra flags
* `C_FLAGS` and `LD_FLAGS` now take priority over the default library folders
* Stop the build early with install instructions when libwebp, libpng, libjpeg or libtiff is missing
* Ship a smaller gem without specs and test images
* Add rake as a runtime dependency, because gem install uses it to build the extension
* Release from a `v*` tag through GitHub Actions and RubyGems.org trusted publishing
* Pin GitHub Actions and Docker images to digests; Dependabot keeps the actions updated
* CI runs the specs with AddressSanitizer and UndefinedBehaviorSanitizer, and compares the output with libwebp's cwebp

## v0.4.0

* Update CI
* Fix C code deprecations warnings
* Update test Ruby versions

## v0.2.7

* Fix build for old libwebp versions

## v0.2.6

* Tested on webp 1.0.2
* Added near_lossless option

## v0.2.5

* Fix build for Travis CI docker containers

## v0.2.4

* DRY C code and split into files each read/write formats

## v0.2.3

* Fix jpg read in C code

## v0.2.2

* Fix png lossless

## v0.2.1

* Fix output for bmp, tiff and yuv

## v0.2.0

* Checked and fixed for webp 0.4.0

## v0.1.8

* Fix encoder options (didn't work fully)

## v0.1.7

* Bump FFI dependency to ~> 1.9.0

## v0.1.6

* Support libwebp 0.3.1

## v0.1.5

* Code cleanup

## v0.1.4

* Added decode options
* Added error class for decode errors

## v0.1.3

* Rename module to WebP

## v0.1.2

* Added encode options
* Added error class for encode errors

## v0.1.1

* Added encode options

## v0.1.0

* First release
