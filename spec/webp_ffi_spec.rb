require 'spec_helper'

describe WebP do
  factories = {
    webp: ["1", "2", "3", "4", "5"],
    png: ["1", "2"],
    jpg: ["3", "4"],
    tiff: ["5"],
    info: {
      "1" => {
        size: [400, 301],
        has_alpha: true
      },
      "2" => {
        size: [386, 395],
        has_alpha: true
      },
      "3" => {
        size: [550, 368],
        has_alpha: false
      },
      "4" => {
        size: [1024, 772],
        has_alpha: false
      },
      "5" => {
        size: [1419, 1001],
        has_alpha: false
      }
    }
  }

  before :all do
    @out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
    Dir.mkdir(@out_dir) unless File.exists?(@out_dir)
  end
  after :all do
    # @out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
    # Dir["#{@out_dir}/*{.png,.webp}"].each do |file|
    #   File.delete(file) rescue nil
    # end
  end

  it "calculate plus 100 by test_c (verify C)" do
    expect(WebP::C.test_c(100)).to eq(200)
    expect(WebP::C.test_c(150)).to eq(250)
  end

  it "decoder version" do
    expect(WebP.decoder_version).not_to be_nil
    expect(WebP.decoder_version).to match(/^([0-9]+)\.([0-9]+)\.([0-9]+)$/)
  end

  it "encoder version" do
    expect(WebP.encoder_version).not_to be_nil
    expect(WebP.decoder_version).to match(/^([0-9]+)\.([0-9]+)\.([0-9]+)$/)
  end

  context "webp_size" do
    factories[:webp].each do |image|
      it "#{image} image size == #{factories[:info][image][:size]}" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        data = File.open(filename, "rb").read
        info = WebP.webp_size(data)
        expect(info.class).to eq(Array)
        expect(info.size).to eq(2)
        expect(info).to eq(factories[:info][image][:size])
      end
    end
    it "raise InvalidImageFormatError for non-webp image" do
      filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/1.png"))
      data = File.open(filename, "rb").read
      expect { WebP.webp_size(data) }.to raise_error WebP::InvalidImageFormatError
    end
  end

  context "encode" do
    factories[:png].each do |image|
      it "#{image}.png image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.png.webp"))
        expect(WebP.encode(in_filename, out_filename)).to be_truthy
      end

      it "#{image}.png image with near_lossless" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}_near_lossless.png.webp"))
        expect(WebP.encode(in_filename, out_filename, near_lossless: 60)).to be_truthy
      end
    end
    factories[:jpg].each do |image|
      it "#{image}.jpg image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.jpg"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.jpg.webp"))
        expect(WebP.encode(in_filename, out_filename)).to be_truthy
      end

      it "#{image}.jpg image with near_lossless" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.jpg"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}_near_lossless.jpg.webp"))
        expect(WebP.encode(in_filename, out_filename, near_lossless: 60)).to be_truthy
      end
    end
    factories[:tiff].each do |image|
      it "#{image}.tif image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.tif"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.tif.webp"))
        expect(WebP.encode(in_filename, out_filename)).to be_truthy
      end
    end
    factories[:webp].each do |image|
      it "raise EncoderError on #{image}.webp image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}invalid.webp.png"))
        expect { WebP.encode(in_filename, out_filename) }.to raise_error WebP::EncoderError
      end
    end
    context "with options" do
      factories[:png].each do |image|
        it "#{image}.png image" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}.50png.webp"))
          WebP.encode(in_filename, out_filename, quality: 50, method: 0, alpha_quality: 10, alpha_compression: 1)
        end
      end
    end
    context "raise EncoderError on invalid crop options" do
      factories[:png].each do |image|
        it "#{image}.png image" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}.invpng.webp"))
          expect { WebP.encode(in_filename, out_filename, crop_w: 30000) }.to raise_error WebP::EncoderError, /crop/i
        end
      end
    end
  end

  context "decode" do
    factories[:webp].each do |image|
      it "#{image}.webp image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.webp.png"))
        expect(WebP.decode(in_filename, out_filename)).to be_truthy
      end
    end
    context "with output_format" do
      [:png, :pam, :ppm, :pgm, :bmp, :tiff, :yuv, :alpha_plane_only].each do |output_format|
        factories[:webp].take(2).each do |image|
          it "#{image}.webp image to #{output_format}" do
            in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
            out_filename = File.expand_path(File.join(@out_dir, "#{image}.#{output_format}.png"))
            expect(WebP.decode(in_filename, out_filename, output_format: output_format)).to be_truthy
          end
        end
      end
    end
    context "with options" do
      factories[:webp].take(2).each do |image|
        it "#{image}.webp image to png and crop" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}_crop.png"))
          expect(WebP.decode(in_filename, out_filename, crop_w: 200, crop_h: 200)).to be_truthy
        end
        it "#{image}.webp image to png and scale" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}_resize.png"))
          expect(WebP.decode(in_filename, out_filename, resize_w: 200, resize_h: 200)).to be_truthy
        end
      end
    end
    context "raise DecoderError on invalid webp image" do
      factories[:png].each do |image|
        it "#{image}.png image" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}.invpng.webp"))
          expect { WebP.decode(in_filename, out_filename) }.to raise_error WebP::DecoderError, /invalid webp/i
        end
      end
    end
    context "raise DecoderError on invalid options" do
      factories[:png].each do |image|
        it "#{image}.png image" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}.png"))
          expect { WebP.decode(in_filename, out_filename, crop_w: 30000) }.to raise_error WebP::DecoderError, /decoding failed/i
        end
      end
    end
  end

end
