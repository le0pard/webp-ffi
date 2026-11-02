require 'spec_helper'

describe "WebP raw RGBA pixels" do
  # 4x3 pixels with varied colors and alpha
  let(:pixels) { (0...12).flat_map { |i| [i * 20, 255 - i * 20, (i * 70) % 256, 255 - i * 10] }.pack("C*") }

  describe ".encode_rgba" do
    it "round-trips every pixel when lossless and exact" do
      image = WebP.decode_rgba(WebP.encode_rgba(pixels, 4, 3, lossless: true, exact: true))
      expect([image.width, image.height, image.rgba]).to eq([4, 3, pixels])
    end

    it "encodes lossy by default" do
      info = WebP.info(WebP.encode_rgba(pixels, 4, 3))
      expect([info.width, info.height, info.format, info.alpha?]).to eq([4, 3, :lossy, true])
    end

    it "applies crop and resize" do
      expect(WebP.webp_size(WebP.encode_rgba(pixels, 4, 3, crop_w: 2, crop_h: 2))).to eq([2, 2])
      expect(WebP.webp_size(WebP.encode_rgba(pixels, 4, 3, resize_w: 8, resize_h: 6))).to eq([8, 6])
    end

    it "rejects pixel data of the wrong size" do
      expect { WebP.encode_rgba(pixels, 5, 3) }
        .to raise_error(ArgumentError, /expected 5 x 3 x 4 bytes of RGBA pixels, got 48/)
      expect { WebP.encode_rgba("", 0, 0) }.to raise_error(ArgumentError)
    end

    it "rejects options for files or raw YUV" do
      expect { WebP.encode_rgba(pixels, 4, 3, metadata: :all) }.to raise_error(ArgumentError, /unknown encode option :metadata/)
      expect { WebP.encode_rgba(pixels, 4, 3, width: 4) }.to raise_error(ArgumentError, /unknown encode option :width/)
    end

    it "reports images larger than WebP allows" do
      expect { WebP.encode_rgba("\0".b * 20_000 * 4, 20_000, 1) }.to raise_error(
        an_instance_of(WebP::EncoderError).and(having_attributes(reason: :bad_dimension, code: 7)))
    end
  end

  describe ".decode_rgba" do
    let(:webp) { File.binread(factory_path("1.webp")) }

    it "returns binary RGBA pixels with their size" do
      image = WebP.decode_rgba(webp)
      expect([image.width, image.height, image.rgba.bytesize, image.rgba.encoding])
        .to eq([400, 301, 400 * 301 * 4, Encoding::BINARY])
    end

    it "matches the PAM output of decode_data" do
      pam = WebP.decode_data(webp, output_format: :pam)
      expect(pam.byteslice(-400 * 301 * 4..)).to eq(WebP.decode_rgba(webp).rgba)
    end

    it "applies crop and resize" do
      image = WebP.decode_rgba(webp, crop_w: 100, crop_h: 50)
      expect([image.width, image.height]).to eq([100, 50])
      image = WebP.decode_rgba(webp, resize_w: 40, resize_h: 30)
      expect([image.width, image.height, image.rgba.bytesize]).to eq([40, 30, 40 * 30 * 4])
    end

    it "rejects an output format" do
      expect { WebP.decode_rgba(webp, output_format: :png) }.to raise_error(ArgumentError, /unknown decode option :output_format/)
    end

    it "raises for data that is not WebP" do
      expect { WebP.decode_rgba("not webp") }.to raise_error(WebP::DecoderError, /Invalid webp image/)
    end
  end
end
