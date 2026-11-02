require 'spec_helper'

describe "WebP in-memory encode and decode" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  def factory_data(name)
    File.binread(factory_path(name))
  end

  # width and height from the IHDR chunk of a PNG
  def png_size(data)
    data.byteslice(16, 8).unpack("NN")
  end

  describe ".encode_data" do
    { "1.png" => [400, 301], "3.jpg" => [550, 368], "5.tif" => [1419, 1001] }.each do |name, size|
      it "encodes #{File.extname(name)} data to the same WebP as the file API" do
        output = File.join(@dir, "out.webp")
        WebP.encode(factory_path(name), output)
        webp = WebP.encode_data(factory_data(name))
        expect(webp).to eq(File.binread(output))
        expect(WebP.webp_size(webp)).to eq(size)
      end
    end

    it "returns binary data" do
      expect(WebP.encode_data(factory_data("1.png")).encoding).to eq(Encoding::BINARY)
    end

    it "applies options" do
      expect(WebP.webp_size(WebP.encode_data(factory_data("1.png"), resize_w: 100, resize_h: 75))).to eq([100, 75])
      expect(WebP.encode_data(factory_data("1.png"), lossless: true).byteslice(12, 4)).to eq("VP8L")
    end

    it "encodes raw YUV 4:2:0 data when :width and :height are given" do
      # 4x2 pixels: 8 bytes of Y, then 2 bytes of U and 2 bytes of V
      yuv = ("\x80" * 12).b
      expect(WebP.webp_size(WebP.encode_data(yuv, width: 4, height: 2))).to eq([4, 2])
    end

    it "leaves the input string unchanged" do
      data = factory_data("1.png")
      copy = data.dup
      WebP.encode_data(data)
      expect(data).to eq(copy)
    end

    it "accepts objects with #to_str" do
      data = Struct.new(:to_str).new(factory_data("1.png"))
      expect(WebP.webp_size(WebP.encode_data(data))).to eq([400, 301])
    end

    it "raises for data that is not an image" do
      expect { WebP.encode_data("not an image") }
        .to raise_error(WebP::EncoderError, "Cannot read input picture data")
    end

    it "raises for empty data" do
      expect { WebP.encode_data("") }.to raise_error(WebP::EncoderError, "Cannot read input picture data")
    end

    it "raises for truncated data" do
      expect { WebP.encode_data(factory_data("1.png").byteslice(0, 60_000)) }
        .to raise_error(WebP::EncoderError, "Cannot read input picture data")
    end

    it "raises for a bad crop like the file API" do
      expect { WebP.encode_data(factory_data("1.png"), crop_w: 30_000) }
        .to raise_error(WebP::EncoderError, "Cannot crop picture")
    end

    it "raises TypeError for data that is not a string", :outside_type_check do
      expect { WebP.encode_data(nil) }.to raise_error(TypeError, /expected image data as a String or an IO, got NilClass/)
    end

    it "checks the options" do
      expect { WebP.encode_data(factory_data("1.png"), cropt_y: 1) }.to raise_error(ArgumentError, /unknown encode option/)
    end
  end

  describe ".decode_data" do
    [:png, :pam, :ppm, :pgm, :bmp, :tiff, :yuv, :alpha_plane_only].each do |format|
      it "decodes to the same #{format} output as the file API" do
        output = File.join(@dir, "out.#{format}")
        WebP.decode(factory_path("1.webp"), output, output_format: format)
        expect(WebP.decode_data(factory_data("1.webp"), output_format: format)).to eq(File.binread(output))
      end
    end

    it "returns a binary PNG by default" do
      png = WebP.decode_data(factory_data("1.webp"))
      expect(png.encoding).to eq(Encoding::BINARY)
      expect(png.byteslice(0, 8)).to eq("\x89PNG\r\n\x1A\n".b)
      expect(png_size(png)).to eq([400, 301])
    end

    it "applies crop and resize" do
      expect(png_size(WebP.decode_data(factory_data("1.webp"), crop_w: 200, crop_h: 100))).to eq([200, 100])
      expect(png_size(WebP.decode_data(factory_data("1.webp"), resize_w: 200, resize_h: 150))).to eq([200, 150])
    end

    it "round-trips with encode_data" do
      webp = WebP.encode_data(factory_data("3.jpg"))
      expect(png_size(WebP.decode_data(webp))).to eq([550, 368])
    end

    it "raises for data that is not WebP" do
      expect { WebP.decode_data(factory_data("1.png")) }.to raise_error(
        an_instance_of(WebP::DecoderError).and(having_attributes(
          message: "Invalid webp image (the data is not a valid WebP bitstream)", code: 2, reason: :bitstream_error)))
    end

    it "raises for empty data" do
      expect { WebP.decode_data("") }
        .to raise_error(WebP::DecoderError, "Invalid webp image (not enough data, the image may be truncated)")
    end

    it "raises for options the image cannot use" do
      expect { WebP.decode_data(factory_data("1.webp"), crop_w: 30_000) }.to raise_error(
        an_instance_of(WebP::DecoderError).and(having_attributes(
          message: "Decoding failed (invalid parameter, for example a crop outside the picture)", reason: :invalid_param)))
    end

    it "raises for an unknown output format" do
      expect { WebP.decode_data(factory_data("1.webp"), output_format: :jpg) }
        .to raise_error(ArgumentError, /unknown output_format :jpg/)
    end
  end
end

describe "WebP data from an IO" do
  require 'stringio'

  it "reads image data from files and StringIO" do
    webp = File.open(factory_path("1.png"), "rb") { |file| WebP.encode_data(file) }
    expect(WebP.webp_size(StringIO.new(webp))).to eq([400, 301])
    expect(WebP.decode_data(StringIO.new(webp)).byteslice(1, 3)).to eq("PNG")
    expect(WebP.decode_rgba(StringIO.new(webp)).width).to eq(400)
  end

  it "reads from the current position" do
    io = StringIO.new("junk" + File.binread(factory_path("1.webp")))
    io.read(4)
    expect(WebP.webp_size(io)).to eq([400, 301])
  end
end
