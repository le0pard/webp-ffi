require 'spec_helper'

describe WebpFfi do
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

  it "calculate plus 100 by test" do
    WebpFfi::C.test(100).should == 200
    WebpFfi::C.test(150).should == 250
  end
  
  it "decoder version" do
    WebpFfi.decoder_version.should_not be_nil
    WebpFfi.decoder_version.should =~ /^([0-9]+)\.([0-9]+)\.([0-9]+)$/
  end
  
  it "encoder version" do
    WebpFfi.encoder_version.should_not be_nil
    WebpFfi.decoder_version.should =~ /^([0-9]+)\.([0-9]+)\.([0-9]+)$/
  end
  
  context "webp_size" do
    factories[:webp].each do |image|
      it "#{image} image size == #{factories[:info][image][:size]}" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        data = File.open(filename, "rb").read
        info = WebpFfi.webp_size(data)
        info.class.should == Array
        info.size.should == 2
        info.should == factories[:info][image][:size]
      end
    end
    it "raise InvalidImageFormatError for non-webp image" do
      filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/1.png"))
      data = File.open(filename, "rb").read
      expect { WebpFfi.webp_size(data) }.to raise_error WebpFfi::InvalidImageFormatError
    end
  end
  
  context "decode" do
    factories[:webp].each do |image|
      it "#{image}.webp image" do
        out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
        Dir.mkdir(out_dir) unless File.exists?(out_dir)
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.webp"))
        out_filename = File.expand_path(File.join(out_dir, "#{image}.webp.png"))
        WebpFfi.decode(in_filename, out_filename)
      end
    end
  end
  
  context "encode" do
    before :all do
      @out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
      Dir.mkdir(@out_dir) unless File.exists?(@out_dir)
    end
    factories[:png].each do |image|
      it "#{image}.png image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.png.webp"))
        WebpFfi.encode(in_filename, out_filename)
      end
    end
    factories[:jpg].each do |image|
      it "#{image}.jpg image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.jpg"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.jpg.webp"))
        WebpFfi.encode(in_filename, out_filename)
      end
    end
    factories[:tiff].each do |image|
      it "#{image}.tif image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.tif"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.tif.webp"))
        WebpFfi.encode(in_filename, out_filename)
      end
    end
  end
  
  context "encode with options" do
    before :all do
      @out_dir = File.expand_path(File.join(File.dirname(__FILE__), "../tmp/"))
      Dir.mkdir(@out_dir) unless File.exists?(@out_dir)
    end
    factories[:png].each do |image|
      it "#{image}.png image" do
        in_filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}.png"))
        out_filename = File.expand_path(File.join(@out_dir, "#{image}.50png.webp"))
        WebpFfi.encode(in_filename, out_filename, quality: 50)
      end
    end
  end

end