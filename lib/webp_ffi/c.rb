module WebpFfi
  module C
    extend FFI::Library
    ffi_lib FFI::Compiler::Loader.find('webp_ffi')
  
    attach_function :WebPGetInfo, [:pointer, :size_t, :pointer, :pointer], :int

    attach_function :test, [:int], :int
  end
end
