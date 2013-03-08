require 'spec_helper'

describe WebpFfi do
  factories = {
    webp: ["1.webp", "2.webp", "3.webp", "4.webp"],
    info: {
      "1.webp" => {
        size: [400, 301]
      },
      "2.webp" => {
        size: [386, 395]
      },
      "3.webp" => {
        size: [300, 300]
      },
      "4.webp" => {
        size: [2000, 2353]
      }
    }
  }

  it "calculate plus 100 by test" do
    WebpFfi::C.test(100).should == 200
    WebpFfi::C.test(150).should == 250
  end
  
  context "#WebPGetInfo" do
    factories[:webp].each do |image|
      it "show webp #{image} size" do
        filename = File.expand_path(File.join(File.dirname(__FILE__), "factories/#{image}"))
        data = File.open(filename, "rb").read
        info = WebpFfi.webp_size(data)
        info.class.should == Array
        info.size.should == 2
        info.should == factories[:info][image][:size]
      end
    end
  end

end