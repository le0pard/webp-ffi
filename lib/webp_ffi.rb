require 'ffi'
require 'ffi-compiler/loader'

module WebP
  module C
    extend FFI::Library
    ffi_lib FFI::Compiler::Loader.find('webp_ffi')
  end
end

require "webp/options"
require "webp/c"
require "webp/error"
require "webp/info"
require "webp/webp"
require "webp/version"