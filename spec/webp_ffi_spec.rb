require 'spec_helper'

describe WebpFfi do

  it "should calculate plus 100 by test" do
    WebpFfi.test(100).should == 200
    WebpFfi.test(150).should == 250
  end

end