require "webp_ffi/options"
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
      width_ptr, height_ptr = FFI::MemoryPointer.new(:int, 2), FFI::MemoryPointer.new(:int, 2)
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      if 0 == C.webp_get_info(memBuf, size, width_ptr, height_ptr)
        [width_ptr.null? ? nil : width_ptr.read_int, height_ptr.null? ? nil : height_ptr.read_int]
      else
        raise InvalidImageFormatError, "invalid WebP image data"
      end
    end
    
    # decode
    def decode(input_file, output_file, options = {})
      res = C.webp_decode(input_file, output_file)
      return true
    end
    
    # encode
    def encode(input_file, output_file, options = {})
      options_obj = Options.new options
      res = C.webp_encode(input_file, output_file, options_obj.encode_pointer)
      raise EncoderError, ENCODER_ERRORS[res - 1] unless 0 == res
      return true
    end
    
  end
end