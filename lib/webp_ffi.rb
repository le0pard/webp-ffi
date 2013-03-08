require 'ffi'
require 'ffi-compiler/loader'

require "webp_ffi/c"
require "webp_ffi/webp"
require "webp_ffi/version"

module WebpFfi
  class << self
    include Webp
  end
end
