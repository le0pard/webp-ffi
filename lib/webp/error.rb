module WebP

  ENCODER_ERRORS = [
    "Version mismatch",
    "Invalid configuration",
    "Cannot read input picture file",
    "Cannot open output file",
    "Cannot crop picture",
    "Cannot resize picture",
    "Cannot encode picture as WebP",
    "Cannot allocate memory"]

  DECODER_ERRORS = [
    "Version mismatch",
    "Invalid webp image",
    "Invalid output format",
    "Decoding failed",
    "Cannot read input file",
    "Cannot write output file",
    "Cannot allocate memory"]

  # encode_data and encode_rgba have no input file, so the read error says "data"
  ENCODER_DATA_ERRORS = ENCODER_ERRORS.dup.tap { |messages| messages[2] = "Cannot read input picture data" }.freeze

  # error codes that point at the input or the output file
  ENCODER_FILE_ERRORS = { 3 => :input, 4 => :output }.freeze
  DECODER_FILE_ERRORS = { 2 => :input, 4 => :input, 5 => :input, 6 => :output }.freeze

  # libwebp's own reasons: WebPEncodingError values when encoding
  ENCODER_REASONS = {
    1 => [:out_of_memory, "out of memory"],
    2 => [:bitstream_out_of_memory, "out of memory while writing the bitstream"],
    3 => [:null_parameter, "a required parameter is missing"],
    4 => [:invalid_configuration, "the configuration is invalid"],
    5 => [:bad_dimension, "the picture size is not supported, WebP allows up to 16383 x 16383 pixels"],
    6 => [:partition0_overflow, "partition 0 is bigger than 512 KiB, try fewer segments or a lower partition_limit"],
    7 => [:partition_overflow, "a partition is bigger than 16 MiB"],
    8 => [:bad_write, "cannot write the output"],
    9 => [:file_too_big, "the file is bigger than 4 GiB"],
    10 => [:user_abort, "aborted"]
  }.freeze

  # ...and VP8StatusCode values when decoding
  DECODER_REASONS = {
    1 => [:out_of_memory, "out of memory"],
    2 => [:invalid_param, "invalid parameter, for example a crop outside the picture"],
    3 => [:bitstream_error, "the data is not a valid WebP bitstream"],
    4 => [:unsupported_feature, "unsupported feature, for example an animated image"],
    5 => [:suspended, "decoding was suspended"],
    6 => [:user_abort, "aborted"],
    7 => [:not_enough_data, "not enough data, the image may be truncated"]
  }.freeze

  # Base class for every error this gem raises.
  class Error < StandardError
    # the gem's error code from C, or nil
    attr_reader :code
    # libwebp's reason as a Symbol (for example :bad_dimension), or nil
    attr_reader :reason

    def initialize(message = nil, code: nil, reason: nil)
      super(message)
      @code = code
      @reason = reason
    end
  end

  class InvalidImageFormatError < Error; end
  class EncoderError < Error; end
  class DecoderError < Error; end
end
