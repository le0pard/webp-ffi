require "webp/options"
require "webp/c"

module WebP
  # WebPBitstreamFeatures format values
  FORMATS = { 0 => :mixed, 1 => :lossy, 2 => :lossless }.freeze
  # flags in the VP8X header of extended WebP files
  METADATA_FLAGS = { icc: 0x20, exif: 0x08, xmp: 0x04 }.freeze

  class << self

    def decoder_version
      format_version(C.decoder_version)
    end

    def encoder_version
      format_version(C.encoder_version)
    end

    # Width and height of WebP image data (a String or an IO), or nil for nil.
    def webp_size(data)
      return nil if data.nil?
      data = data_string(data)
      width, height = FFI::MemoryPointer.new(:int), FFI::MemoryPointer.new(:int)
      unless 0 == C.webp_get_info(data, data.bytesize, width, height)
        raise InvalidImageFormatError, "invalid WebP image data"
      end
      [width.read_int, height.read_int]
    end

    # Details about WebP image data (a String or an IO) as a WebP::Info, or nil
    # for nil: size, format (:lossy, :lossless or :mixed), alpha, animation and
    # the metadata it carries (:icc, :exif, :xmp).
    def info(data)
      return nil if data.nil?
      data = data_string(data)
      features = C::FfiWebpFeatures.new
      status = C.webp_get_features(data, data.bytesize, features)
      unless status.zero?
        reason, text = DECODER_REASONS[status]
        raise InvalidImageFormatError.new(["invalid WebP image data", text].compact.join(": "), reason: reason)
      end
      Info.new(width: features[:width], height: features[:height],
               format: FORMATS.fetch(features[:format], :mixed),
               alpha: features[:has_alpha] == 1, animated: features[:has_animation] == 1,
               metadata: metadata_in(data))
    end

    # encode
    def encode(input_file, output_file, options = {})
      input_file, output_file = file_path(input_file), file_path(output_file)
      options_pointer = init_options_object(options).encode_pointer
      detail = FFI::MemoryPointer.new(:int)
      code = C.webp_encode(input_file, output_file, options_pointer, detail)
      unless code.zero?
        raise build_error(EncoderError, ENCODER_ERRORS, code, detail.read_int, ENCODER_FILE_ERRORS, input_file, output_file)
      end
      true
    end

    # decode
    def decode(input_file, output_file, options = {})
      input_file, output_file = file_path(input_file), file_path(output_file)
      options_pointer = init_options_object(options).decode_pointer
      detail = FFI::MemoryPointer.new(:int)
      code = C.webp_decode(input_file, output_file, options_pointer, detail)
      unless code.zero?
        raise build_error(DecoderError, DECODER_ERRORS, code, detail.read_int, DECODER_FILE_ERRORS, input_file, output_file)
      end
      true
    end

    # Encodes PNG, JPEG or TIFF image data (or raw YUV data, with the :width and
    # :height options) to WebP. 'data' is a String or an IO. Returns the WebP
    # image as a binary String.
    def encode_data(data, options = {})
      options_pointer = init_options_object(options).encode_pointer
      with_output(EncoderError, ENCODER_DATA_ERRORS, data) do |input, size, output, output_size, detail|
        C.webp_encode_data(input, size, output, output_size, options_pointer, detail)
      end
    end

    # Decodes WebP image data (a String or an IO). Returns the image in the
    # :output_format (PNG by default) as a binary String.
    def decode_data(data, options = {})
      options_pointer = init_options_object(options).decode_pointer
      with_output(DecoderError, DECODER_ERRORS, data) do |input, size, output, output_size, detail|
        C.webp_decode_data(input, size, output, output_size, options_pointer, detail)
      end
    end

    # Encodes raw RGBA pixels: 'pixels' holds width * height * 4 bytes, row
    # after row. Returns the WebP image as a binary String.
    def encode_rgba(pixels, width, height, options = {})
      width, height = Integer(width), Integer(height)
      pixels = data_string(pixels)
      unless width.positive? && height.positive? && pixels.bytesize == width * height * 4
        raise ArgumentError, "webp-ffi: expected #{width} x #{height} x 4 bytes of RGBA pixels, got #{pixels.bytesize}"
      end
      options_pointer = init_options_object(options).encode_pointer(Options::ENCODE_RGBA_OPTIONS)
      with_output(EncoderError, ENCODER_DATA_ERRORS, pixels) do |input, _size, output, output_size, detail|
        C.webp_encode_rgba(input, width, height, output, output_size, options_pointer, detail)
      end
    end

    # Decodes WebP image data (a String or an IO) to raw RGBA pixels. Returns a
    # WebP::RGBAImage with the pixels, width and height.
    def decode_rgba(data, options = {})
      options_pointer = init_options_object(options).decode_pointer(Options::DECODE_RGBA_OPTIONS)
      width, height = FFI::MemoryPointer.new(:int), FFI::MemoryPointer.new(:int)
      rgba = with_output(DecoderError, DECODER_ERRORS, data) do |input, size, output, output_size, detail|
        C.webp_decode_rgba(input, size, output, output_size, width, height, options_pointer, detail)
      end
      RGBAImage.new(rgba: rgba, width: width.read_int, height: height.read_int)
    end

    private

    def format_version(version)
      [version >> 16, version >> 8, version].map { |part| part & 0xff }.join(".")
    end

    # Accepts a String, a Pathname or anything with #to_path. Returns a new
    # string, because C reads it while other Ruby threads keep running.
    def file_path(file)
      String.new(File.path(file))
    end

    # Accepts a String (or anything with #to_str) or an IO to read from.
    def data_string(data)
      data = data.read if data.respond_to?(:read) && !data.is_a?(String)
      String.try_convert(data) or raise TypeError, "webp-ffi: expected image data as a String or an IO, got #{data.class}"
    end

    def metadata_in(data)
      return [] unless data.bytesize >= 21 && data.byteslice(12, 4) == "VP8X"
      flags = data.getbyte(20)
      METADATA_FLAGS.select { |_name, flag| flags & flag != 0 }.keys
    end

    # Copies the input to native memory (C reads it while other Ruby threads
    # keep running), yields the arguments for the C call, and returns the output
    # bytes. Raises error_class when C reports an error.
    def with_output(error_class, messages, data)
      data = data_string(data)
      input = FFI::MemoryPointer.new(:uint8, [data.bytesize, 1].max)
      input.put_bytes(0, data)
      output = FFI::MemoryPointer.new(:pointer)
      output_size = FFI::MemoryPointer.new(:size_t)
      detail = FFI::MemoryPointer.new(:int)
      code = yield(input, data.bytesize, output, output_size, detail)
      raise build_error(error_class, messages, code, detail.read_int) unless code.zero?

      buffer = output.read_pointer
      begin
        buffer.read_bytes(output_size.read(:size_t))
      ensure
        C.webp_free_data(buffer)
      end
    end

    def build_error(error_class, messages, code, detail, file_errors = {}, input_file = nil, output_file = nil)
      message = code.between?(1, messages.size) ? messages[code - 1] : "Unknown error (code #{code})"
      case file_errors[code]
      when :input then message = "#{message}: #{input_file}"
      when :output then message = "#{message}: #{output_file}"
      end
      reasons = error_class == EncoderError ? ENCODER_REASONS : DECODER_REASONS
      reason, text = reasons[detail]
      message = "#{message} (#{text})" if text
      error_class.new(message, code: code, reason: reason)
    end

    def init_options_object(options)
      Options.new options
    end

  end
end
