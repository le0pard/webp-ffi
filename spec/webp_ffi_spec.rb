require 'spec_helper'

describe WebP do
  factories = {
    webp: ["1", "2", "3", "4", "5", "6", "7"],
    png: ["1", "2", "3", "4"],
    jpg: ["5", "6"],
    tiff: ["7"],
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
        size: [300, 300],
        has_alpha: true
      },
      "4" => {
        size: [2000, 2353],
        has_alpha: true
      },
      "5" => {
        size: [550, 368],
        has_alpha: false
      },
      "6" => {
        size: [1024, 772],
        has_alpha: false
      },
      "7" => {
        size: [1419, 1001],
        has_alpha: false
      }
    }
  }
  
  before :all do
    @out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
    Dir.mkdir(@out_dir) unless File.exists?(@out_dir)
  end

  it "calculate plus 100 by test_c (verify C)" do
    WebP::C.test_c(100).should == 200
    WebP::C.test_c(150).should == 250
  end
  
  it "decoder version" do
    WebP.decoder_version.should_not be_nil
    WebP.decoder_version.should =~ /^([0-9]+)\.([0-9]+)\.([0-9]+)$/
  end
  
  it "encoder version" do
    WebP.encoder_version.should_not be_nil
    WebP.decoder_version.should =~ /^([0-9]+)\.([0-9]+)\.([0-9]+)$/
  end
  
  context "webp_size" do
    factories[:webp].each do |image|
      it "#{image} image size == #{factories[:info][image][:size]}" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        data = File.open(filename, "rb").read
        info = WebP.webp_size(data)
        info.class.should == Array
        info.size.should == 2
        info.should == factories[:info][image][:size]
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
        WebP.encode(in_filename, out_filename).should be_true
      end
    end
    factories[:jpg].each do |image|
      it "#{image}.jpg image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.jpg"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.jpg.webp"))
        WebP.encode(in_filename, out_filename).should be_true
      end
    end
    factories[:tiff].each do |image|
      it "#{image}.tif image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.tif"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.tif.webp"))
        WebP.encode(in_filename, out_filename).should be_true
      end
    end
    factories[:webp].each do |image|
      it "raise EncoderError on #{image}.webp image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}invalid.webp.png"))
        expect { WebP.encode(in_filename, out_filename) }.to raise_error WebP::EncoderError
      end
    end
  end
  
  context "encode with options" do
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
        expect { WebP.encode(in_filename, out_filename, crop_w: 30000) }.to raise_error WebP::EncoderError
      end
    end
  end
  
  context "decode" do
    factories[:webp].each do |image|
      it "#{image}.webp image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.webp.png"))
        WebP.decode(in_filename, out_filename).should be_true
      end
    end
  end
  
  context "decode with output_format" do
    [:pam, :ppm].each do |output_format|
      factories[:webp].each do |image|
        it "#{image}.webp image to #{output_format}" do
          in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
          out_filename = File.expand_path(File.join(@out_dir, "#{image}.#{output_format}.png"))
          WebP.decode(in_filename, out_filename, output_format: output_format).should be_true
        end
      end
    end
  end

end