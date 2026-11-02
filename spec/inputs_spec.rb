require 'spec_helper'

describe "WebP inputs and outputs" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  # lossless, exact encoding and decoding back to RGBA: [[r, g, b, a], ...]
  def round_trip(data)
    WebP.decode_rgba(WebP.encode_data(data, lossless: true, exact: true)).rgba.unpack("C*").each_slice(4).to_a
  end

  describe "PNG color types" do
    it "reads 8-bit grayscale" do
      data = png(4, 1, color_type: 0, rows: [[0, 80, 160, 255].pack("C*")])
      expect(round_trip(data)).to eq([0, 80, 160, 255].map { |v| [v, v, v, 255] })
    end

    it "reads 1-bit grayscale" do
      data = png(8, 1, bit_depth: 1, color_type: 0, rows: [[0b10100000].pack("C")])
      expect(round_trip(data).map(&:first)).to eq([255, 0, 255, 0, 0, 0, 0, 0])
    end

    it "reads grayscale with alpha" do
      data = png(2, 1, color_type: 4, rows: [[50, 0, 200, 255].pack("C*")])
      expect(round_trip(data)).to eq([[50, 50, 50, 0], [200, 200, 200, 255]])
    end

    it "reads palette images with transparency" do
      palette = png_chunk("PLTE", [255, 0, 0, 0, 0, 255].pack("C*"))
      transparency = png_chunk("tRNS", [0, 255].pack("C*"))
      data = png(2, 1, color_type: 3, rows: [[0, 1].pack("C*")], chunks: [palette, transparency])
      expect(round_trip(data)).to eq([[255, 0, 0, 0], [0, 0, 255, 255]])
    end

    it "reads 16-bit RGB by keeping the high byte" do
      data = png(1, 1, bit_depth: 16, color_type: 2, rows: [[0x12FF, 0x3400, 0xABCD].pack("n*")])
      expect(round_trip(data)).to eq([[0x12, 0x34, 0xAB, 255]])
    end

    it "reads interlaced images" do
      # Adam7 stores a 2x2 image as pass 1: (0,0), pass 6: (1,0), pass 7: (0,1) (1,1)
      rows = [[10, 20, 30], [40, 50, 60], [70, 80, 90, 100, 110, 120]].map { |row| row.pack("C*") }
      data = png(2, 2, interlace: 1, rows: rows)
      expect(round_trip(data).map { |pixel| pixel.first(3) })
        .to eq([[10, 20, 30], [40, 50, 60], [70, 80, 90], [100, 110, 120]])
    end
  end

  it "reads raw YUV 4:2:0 data when :width and :height are given" do
    yuv = ([128] * 8 + [128] * 2 + [128] * 2).pack("C*")
    expect(WebP.webp_size(WebP.encode_data(yuv, width: 4, height: 2))).to eq([4, 2])
  end

  it "rejects raw YUV data that is too short" do
    expect { WebP.encode_data("abc", width: 4, height: 2) }
      .to raise_error(WebP::EncoderError, "Cannot read input picture data")
  end

  it "rejects data too short to tell the format" do
    expect { WebP.encode_data("ab") }.to raise_error(WebP::EncoderError, "Cannot read input picture data")
  end

  it "writes BMP rows padded to 4 bytes for images without alpha" do
    output = File.join(@dir, "3.bmp")
    WebP.decode(factory_path("3.webp"), output, output_format: :bmp)
    bmp = File.binread(output)
    width, height, bits = bmp.byteslice(18, 12).unpack("Vl<xxv")
    row = (width * 3 + 3) & ~3
    expect([width, -height, bits]).to eq([550, 368, 24])
    expect(bmp.bytesize).to eq(54 + row * 368)
    expect(bmp.byteslice(2, 4).unpack1("V")).to eq(bmp.bytesize)
  end

  it "rejects a folder as the input file" do
    expect { WebP.encode(@dir, File.join(@dir, "out.webp")) }
      .to raise_error(WebP::EncoderError, "Cannot read input picture file: #{@dir}")
    expect { WebP.decode(@dir, File.join(@dir, "out.png")) }
      .to raise_error(WebP::DecoderError, "Cannot read input file: #{@dir}")
  end

  it "rejects an option combination libwebp does not accept" do
    expect { WebP.encode_data(File.binread(factory_path("1.png")), pass: 0) }
      .to raise_error(WebP::EncoderError, "Invalid configuration")
  end

  # /dev/full accepts the file but fails every write, like a full disk
  context "when the disk is full", if: File.exist?("/dev/full") do
    it "fails to encode" do
      expect { WebP.encode(factory_path("1.png"), "/dev/full") }
        .to raise_error(an_instance_of(WebP::EncoderError).and(having_attributes(reason: :bad_write)))
    end

    it "fails to decode" do
      expect { WebP.decode(factory_path("1.webp"), "/dev/full") }
        .to raise_error(WebP::DecoderError, "Cannot write output file: /dev/full")
    end
  end
end
