require 'spec_helper'

# Each option has to reach libwebp. When an option changes the encoded bytes,
# that proves it did.
describe "WebP encode options" do
  let(:jpg) { File.binread(factory_path("3.jpg")) }
  let(:png) { File.binread(factory_path("1.png")) }

  def encode(data, options = {})
    WebP.encode_data(data, options)
  end

  {
    { quality: 50 } => {}, { method: 0 } => {}, { segments: 1 } => {}, { sns_strength: 0 } => {},
    { filter_strength: 0 } => {}, { filter_sharpness: 7 } => {}, { filter_type: 0 } => {},
    { pass: 10 } => {}, { partition_limit: 100 } => {}, { low_memory: true } => {},
    { preset: :drawing } => {}, { target_size: 20_000 } => {}, { target_PSNR: 30 } => {},
    { use_sharp_yuv: true } => {},
    { autofilter: true } => { quality: 50 }, { preprocessing: 1 } => { quality: 50 },
    { emulate_jpeg_size: true } => { quality: 50 },
    # libwebp only splits token partitions in low memory mode
    { partitions: 3 } => { low_memory: true }
  }.each do |option, base|
    it "applies #{option.inspect}#{" with #{base.inspect}" unless base.empty?} to JPEG input" do
      expect(encode(jpg, base.merge(option))).not_to eq(encode(jpg, base))
    end
  end

  {
    { alpha_compression: 0 } => {}, { alpha_quality: 50 } => {}, { exact: true } => {},
    { exact: true, lossless: true } => { lossless: true }, { level: 0 } => { lossless: true },
    { near_lossless: 60 } => { lossless: true }
  }.each do |option, base|
    it "applies #{option.inspect} to PNG input with alpha" do
      expect(encode(png, base.merge(option))).not_to eq(encode(png, base))
    end
  end

  # These change nothing visible for the test images, but must be accepted.
  [{ image_hint: :photo }, { image_hint: "graph" }, { alpha_filtering: 2 },
   { show_compressed: true }, { thread_level: true }].each do |option|
    it "accepts #{option.inspect}" do
      expect(WebP.webp_size(encode(png, option))).to eq([400, 301])
    end
  end

  it "produces the same file with and without threads" do
    expect(encode(jpg, thread_level: true)).to eq(encode(jpg))
  end

  describe "target_size and target_PSNR" do
    # libwebp only searches with more than one pass; like cwebp, the gem uses 6
    it "gets close to target_size" do
      expect(encode(jpg, target_size: 20_000).bytesize).to be_within(4_000).of(20_000)
    end

    it "uses an explicit pass" do
      expect(encode(jpg, target_size: 20_000, pass: 2)).not_to eq(encode(jpg, target_size: 20_000))
    end

    it "gets smaller for a lower target_PSNR" do
      expect(encode(jpg, target_PSNR: 30).bytesize).to be < encode(jpg, target_PSNR: 40).bytesize
    end
  end

  describe "presets" do
    it "accepts every libwebp preset by name" do
      sizes = %i[default picture photo drawing icon text].map { |preset| encode(jpg, preset: preset, quality: 75).bytesize }
      expect(sizes).to all(be_positive)
    end

    it "keeps an explicit option over the preset" do
      expect(encode(jpg, preset: :photo, segments: 1)).not_to eq(encode(jpg, preset: :photo))
    end
  end

  describe "level" do
    it "turns on lossless encoding" do
      expect(WebP.info(encode(jpg, level: 1)).format).to eq(:lossless)
    end

    it "picks its own settings, and keeps an explicit quality" do
      expect(encode(png, level: 0)).not_to eq(encode(png, level: 9))
      expect(encode(png, level: 9, quality: 10)).not_to eq(encode(png, level: 9))
    end
  end

  describe "lossless input from JPEG and TIFF" do
    # The pixels must stay RGB until encoding, or lossless output is not exact.
    it "keeps every pixel of a TIFF" do
      pixels = (0...64).map { |i| [i * 4, 255 - i * 4, (i * 37) % 256] }.flatten.pack("C*")
      webp = encode(tiff(8, 8, pixels: pixels), lossless: true)
      rgba = WebP.decode_rgba(webp).rgba.unpack("C*").each_slice(4).map { |pixel| pixel.first(3) }
      expect(rgba.flatten.pack("C*")).to eq(pixels)
    end

    it "uses sharp RGB to YUV conversion for JPEG input" do
      expect(encode(jpg, use_sharp_yuv: true)).not_to eq(encode(jpg))
    end
  end
end
