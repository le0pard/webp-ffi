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
  
  context "webp_info" do
    factories[:webp].each do |image|
      it "#{image} image info" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}"))
        data = File.open(filename, "rb").read
        info = WebpFfi.webp_info(data)
        [info[:width], info[:height]].should == factories[:info][image][:size]
        info[:has_alpha].should == factories[:info][image][:has_alpha]
      end
    end
    it "raise InvalidImageFormatError for non-webp image" do
      filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/1.png"))
      data = File.open(filename, "rb").read
      expect { WebpFfi.webp_size(data) }.to raise_error WebpFfi::InvalidImageFormatError
    end
  end

end