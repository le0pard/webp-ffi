require 'ffi'
require 'ffi-compiler/loader'

require "webp_ffi/c"
require "webp_ffi/version"

module WebpFfi
  
  class << self
    
    def webp_size(data)
      return nil if data.nil?
      width_ptr = FFI::MemoryPointer.new(:int, 2)
      height_ptr = FFI::MemoryPointer.new(:int, 2)
      size = data.respond_to?(:bytesize) ? data.bytesize : data.size
      memBuf = FFI::MemoryPointer.new(:char, size)
      memBuf.put_bytes(0, data)
      if WebpFfi::C.WebPGetInfo(memBuf, size, width_ptr, height_ptr) == 1
        [width_ptr.read_int, height_ptr.read_int]
      else
        return nil
      end
    end
    
  end
  
end
