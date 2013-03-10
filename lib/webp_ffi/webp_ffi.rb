require "webp_ffi/c"

module WebpFfi
  class << self
    
    def decoder_version
      pointer = FFI::MemoryPointer.new(:char, 10)
      C.decoder_version(pointer)
      pointer.null? ? nil : pointer.read_string()
    end
    
    def encoder_version
      pointer = FFI::MemoryPointer.new(:char, 10)
      C.encoder_version(pointer)
      pointer.null? ? nil : pointer.read_string()
    end
    
    # get webp image size
    def webp_size(data)
      return nil if data.nil?
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      width_ptr = FFI::MemoryPointer.new(:int, 2)
      height_ptr = FFI::MemoryPointer.new(:int, 2)
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      if C.webp_get_info(memBuf, size, width_ptr, height_ptr) == 1
        [width_ptr.null? ? nil : width_ptr.read_int, height_ptr.null? ? nil : height_ptr.read_int]
      else
        raise InvalidImageFormatError, "invalid webp image"
        return nil
      end
    end
    
    # decode
    def decode(input_file, output_file, options = {})
      C.webp_decode(input_file, output_file)
    end
    
    # encode
    def encode(data, options = {})
      return nil if data.nil?
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      options_pointer = FFI::MemoryPointer.new :char, C::FfiWebpConfig.size, false
      options_struct = C::FfiWebpConfig.new options_pointer
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      output_pointer = FFI::MemoryPointer.new :pointer, 1
      if C.webp_encode(memBuf, size, options_pointer, output_pointer) == 1
        return output_pointer
      else
        #raise InvalidImageFormatError, "invalid webp image"
        return nil
      end
    end
    
  end
end