require 'spec_helper'

# Broken input must raise a WebP::Error (or still work, when the damage happens
# to be harmless), and never crash the process. A crash ends the test run.
describe "WebP with broken input" do
  # outcome of one call: :ok, or the error class it raised
  def outcome
    yield
    :ok
  rescue WebP::Error => e
    e.class
  end

  # positions to cut the data at: the first bytes, where headers live, and
  # spread over the rest
  def cut_points(data, count = 24)
    ([1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 100] + (1..count).map { |i| data.bytesize * i / (count + 1) }).uniq.sort
  end

  # copies of 'data' with three bytes flipped each, past the first 'skip' bytes:
  # damage to image sizes in headers is covered by the cuts, and could make a
  # reader allocate gigabytes
  def flipped(data, count: 24, skip: 128, seed: 2026)
    random = Random.new(seed)
    Array.new(count) do
      copy = data.b.dup
      3.times do
        offset = skip + random.rand(copy.bytesize - skip)
        copy.setbyte(offset, copy.getbyte(offset) ^ 0xFF)
      end
      copy
    end
  end

  def encode_options = { method: 0, quality: 50, metadata: :all }

  {
    "PNG" => -> { File.binread(factory_path("1.png")) },
    "JPEG" => -> { File.binread(factory_path("3.jpg")) },
    "TIFF" => -> { File.binread(factory_path("multipage.tif")) },
    "PNG with metadata" => -> {
      png(4, 4, rows: ["\x10\x20\x30".b * 4] * 4,
          chunks: [png_iccp(icc_profile), png_exif(exif), png_xmp(xmp), png_text("Raw profile type xmp", raw_profile("xmp", xmp))])
    },
    "JPEG with metadata" => -> {
      jpeg_with_segments(File.binread(factory_path("3.jpg")),
                         [jpeg_exif_segment(exif), jpeg_xmp_segment(xmp)] + jpeg_icc_segments(icc_profile, parts: 3))
    }
  }.each do |name, input|
    describe "#{name} data" do
      let(:data) { instance_exec(&input) }

      it "survives being cut short" do
        outcomes = cut_points(data).map { |length| outcome { WebP.encode_data(data.byteslice(0, length), encode_options) } }
        expect(outcomes - [:ok, WebP::EncoderError]).to be_empty
        expect(outcomes).to include(WebP::EncoderError)
      end

      it "survives flipped bytes" do
        outcomes = flipped(data, skip: [128, data.bytesize / 4].min).map { |broken| outcome { WebP.encode_data(broken, encode_options) } }
        expect(outcomes - [:ok, WebP::EncoderError]).to be_empty
      end
    end
  end

  describe "WebP data" do
    let(:webp) { File.binread(factory_path("1.webp")) }
    let(:lossy) { WebP.encode_data(File.binread(factory_path("1.png")), quality: 50) }

    it "survives being cut short" do
      [webp, lossy].each do |data|
        outcomes = cut_points(data).flat_map do |length|
          part = data.byteslice(0, length)
          [outcome { WebP.decode_data(part) }, outcome { WebP.decode_rgba(part) },
           outcome { WebP.info(part) }, outcome { WebP.webp_size(part) }]
        end
        expect(outcomes - [:ok, WebP::DecoderError, WebP::InvalidImageFormatError]).to be_empty
        expect(outcomes).to include(WebP::DecoderError)
      end
    end

    it "survives flipped bytes" do
      [webp, lossy].each do |data|
        outcomes = flipped(data, skip: 40).flat_map do |broken|
          [outcome { WebP.decode_data(broken) }, outcome { WebP.decode_rgba(broken) }, outcome { WebP.info(broken) }]
        end
        expect(outcomes - [:ok, WebP::DecoderError, WebP::InvalidImageFormatError]).to be_empty
      end
    end
  end
end
