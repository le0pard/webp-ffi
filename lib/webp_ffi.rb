require 'ffi'
require 'ffi-compiler/loader'

module WebpFfi
  module C
    extend FFI::Library
    ffi_lib FFI::Compiler::Loader.find('webp_ffi')
  end
end

require "webp_ffi/options"
require "webp_ffi/c"
require "webp_ffi/error"
require "webp_ffi/libc" # not used right now
require "webp_ffi/webp_ffi"
require "webp_ffi/version"