require "webp_ffi/c"

module WebpFfi
  class << self
    
    def decoder_version
      C.decoder_version
    end
    
    def encoder_version
      C.encoder_version
    end
    
    # get webp image size
    def webp_size(data)
      return nil if data.nil?
      width_ptr = FFI::MemoryPointer.new(:int, 2)
      height_ptr = FFI::MemoryPointer.new(:int, 2)
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      if C.WebPGetInfo(memBuf, size, width_ptr, height_ptr) == 1
        [width_ptr.null? ? nil : width_ptr.read_int, height_ptr.null? ? nil : height_ptr.read_int]
      else
        raise InvalidImageFormatError, "invalid webp image"
        return nil
      end
    end
    
    def webp_decode_rgba(data)
      return nil if data.nil?
      width_ptr = FFI::MemoryPointer.new(:int, 2)
      height_ptr = FFI::MemoryPointer.new(:int, 2)
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      pointer = C.WebPDecodeRGBA(memBuf, size, width_ptr, height_ptr)
      pointer.null? ? nil : pointer.read_string()
    end
    
  end
end