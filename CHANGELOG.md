## Unreleased

* Fix `near_lossless` option being ignored with every libwebp version
* Fix decode reporting success when the output file cannot be written
* Fix wrong error message when the decode input file cannot be read
* Include the file path in error messages about input and output files
* Check image sizes before allocating memory in the PNG, JPEG and TIFF readers
* Fix memory leaks when reading broken PNG and JPEG files
* Fix multi-page TIFF files using the last page instead of the first
* Stop printing errors and warnings to stderr (libtiff older than 4.5.0 still prints its own)
* Always initialize every decode option passed to C
* Find libraries from Homebrew on Apple Silicon and from MacPorts without extra flags
* `C_FLAGS` and `LD_FLAGS` now take priority over the default library folders
* Stop the build early with install instructions when libwebp, libpng, libjpeg or libtiff is missing
* Ship a smaller gem without specs and test images
* Add rake as a runtime dependency, because gem install uses it to build the extension
* Release the Global VM Lock during encode and decode, so other Ruby threads keep running
* Accept `Pathname` and other objects with `#to_path` as file names
* Warn about unknown options, with a suggestion for typos
* Raise `ArgumentError` for an unknown `output_format` instead of silently writing PNG
* Raise `ArgumentError` for option values that are not numbers
* Accept `true` and `false` for integer options; `false` now means 0 instead of the libwebp default
* Accept the decode `output_format` as a string

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
