require "webp_ffi/c"

module WebpFfi
  class << self
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
    
    def webp_info(data)
      return nil if data.nil?
      pointer = FFI::MemoryPointer.new :char, C::WebPBitstreamFeatures.size, false
      info = C::WebPBitstreamFeatures.new pointer
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      result = C.webp_get_features(memBuf, size, pointer)
      if result == :vp8_status_ok
        return { width: info[:width], height: info[:height], has_alpha: (info[:has_alpha] == 1 ? true : false) }
      else
        raise InvalidImageFormatError, "invalid webp image. Error code: #{result}"
        return nil
      end
    end
    
  end
end