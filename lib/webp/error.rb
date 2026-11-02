module WebP

  ENCODER_ERRORS = [
    "Version mismatch",
    "Invalid configuration",
    "Cannot read input picture file",
    "Cannot open output file",
    "Cannot crop picture",
    "Cannot resize picture",
    "Cannot encode picture as WebP"]

  DECODER_ERRORS = [
    "Version mismatch",
    "Invalid webp image",
    "Invalid output format",
    "Decoding failed",
    "Cannot read input file",
    "Cannot write output file"]

  # error codes that point at the input or the output file
  ENCODER_FILE_ERRORS = { 3 => :input, 4 => :output }.freeze
  DECODER_FILE_ERRORS = { 2 => :input, 4 => :input, 5 => :input, 6 => :output }.freeze

  class InvalidImageFormatError < StandardError; end
  class EncoderError < StandardError; end
  class DecoderError < StandardError; end
end
