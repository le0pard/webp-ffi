require "webp/options"
require "webp/c"

module WebP
  class << self

    def decoder_version
      format_version(C.decoder_version)
    end

    def encoder_version
      format_version(C.encoder_version)
    end

    # get webp image size
    def webp_size(data)
      return nil if data.nil?
      data = data_string(data)
      width, height = FFI::MemoryPointer.new(:int), FFI::MemoryPointer.new(:int)
      unless 0 == C.webp_get_info(data, data.bytesize, width, height)
        raise InvalidImageFormatError, "invalid WebP image data"
      end
      [width.read_int, height.read_int]
    end

    # encode
    def encode(input_file, output_file, options = {})
      input_file, output_file = file_path(input_file), file_path(output_file)
      unless 0 == (encode_res = C.webp_encode(input_file, output_file, init_options_object(options).encode_pointer))
        raise EncoderError, error_message(ENCODER_ERRORS, encode_res, ENCODER_FILE_ERRORS, input_file, output_file)
      end
      true
    end

    # decode
    def decode(input_file, output_file, options = {})
      input_file, output_file = file_path(input_file), file_path(output_file)
      unless 0 == (decode_res = C.webp_decode(input_file, output_file, init_options_object(options).decode_pointer))
        raise DecoderError, error_message(DECODER_ERRORS, decode_res, DECODER_FILE_ERRORS, input_file, output_file)
      end
      true
    end

    # Encodes PNG, JPEG or TIFF image data (or raw YUV data, with the :width and
    # :height options) to WebP. Returns the WebP image as a binary String.
    def encode_data(data, options = {})
      convert_data(:webp_encode_data, data, init_options_object(options).encode_pointer) do |code|
        raise EncoderError, error_message(ENCODER_DATA_ERRORS, code)
      end
    end

    # Decodes WebP image data. Returns the image in the :output_format (PNG by
    # default) as a binary String.
    def decode_data(data, options = {})
      convert_data(:webp_decode_data, data, init_options_object(options).decode_pointer) do |code|
        raise DecoderError, error_message(DECODER_ERRORS, code)
      end
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

    def data_string(data)
      String.try_convert(data) or raise TypeError, "webp-ffi: expected image data as a String, got #{data.class}"
    end

    # Runs an in-memory C function. The input is copied to native memory first,
    # because C reads it while other Ruby threads keep running. Yields the
    # error code on failure.
    def convert_data(function, data, options_pointer)
      data = data_string(data)
      input = FFI::MemoryPointer.new(:uint8, [data.bytesize, 1].max)
      input.put_bytes(0, data)
      output = FFI::MemoryPointer.new(:pointer)
      output_size = FFI::MemoryPointer.new(:size_t)
      code = C.public_send(function, input, data.bytesize, output, output_size, options_pointer)
      return yield(code) unless code.zero?

      buffer = output.read_pointer
      begin
        buffer.read_bytes(output_size.read(:size_t))
      ensure
        C.webp_free_data(buffer)
      end
    end

    def error_message(messages, code, file_errors = {}, input_file = nil, output_file = nil)
      message = code.between?(1, messages.size) ? messages[code - 1] : "Unknown error (code #{code})"
      case file_errors[code]
      when :input then "#{message}: #{input_file}"
      when :output then "#{message}: #{output_file}"
      else message
      end
    end

    def init_options_object(options)
      Options.new options
    end

  end
end
