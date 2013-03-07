require 'ffi'
require 'ffi-compiler/loader'

require "webp_ffi/version"

module WebpFfi
  extend FFI::Library
  ffi_lib FFI::Compiler::Loader.find('webp_ffi')

  attach_function :test, [:int], :int
end
