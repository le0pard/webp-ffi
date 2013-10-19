require 'rspec'

require File.dirname(__FILE__) + "/../lib/webp_ffi"

RSpec.configure do |config|
  config.expect_with :rspec do |c|
    c.syntax = :expect
  end
end