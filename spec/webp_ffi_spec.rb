require 'spec_helper'

describe WebpFfi do
  factories = {
    webp: ["1.webp", "2.webp", "3.webp", "4.webp", "5.webp", "6.webp"],
    info: {
      "1.webp" => {
        size: [400, 301],
        has_alpha: true
      },
      "2.webp" => {
        size: [386, 395],
        has_alpha: true
      },
      "3.webp" => {
        size: [300, 300],
        has_alpha: true
      },
      "4.webp" => {
        size: [2000, 2353],
        has_alpha: true
      },
      "5.webp" => {
        size: [550, 368],
        has_alpha: false
      },
      "6.webp" => {
        size: [1024, 772],
        has_alpha: false
      }
    }
  }

  it "calculate plus 100 by test" do
    WebpFfi::C.test(100).should == 200
    WebpFfi::C.test(150).should == 250
  end
  
  it "show decoder version" do
    puts "decoder version: #{WebpFfi.decoder_version}"
    WebpFfi.decoder_version.should_not be_nil
  end
  
  it "show encoder version" do
    puts "encoder version: #{WebpFfi.encoder_version}"
    WebpFfi.encoder_version.should_not be_nil
  end
  
  context "webp_size" do
    factories[:webp].each do |image|
      it "#{image} image size == #{factories[:info][image][:size]}" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}"))
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
  
  context "webp_decode_rgba" do
    factories[:webp].each do |image|
      it "#{image} webp_decode_rgba" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}"))
        data = File.open(filename, "rb").read
        output_data = WebpFfi.webp_decode_rgba(data)
        # TODO: finish
      end
    end
  end

end