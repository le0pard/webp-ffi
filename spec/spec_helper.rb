require 'rspec'
require 'tmpdir'

require File.dirname(__FILE__) + "/../lib/webp_ffi"

module SpecHelpers
  def factory_path(name)
    File.expand_path(File.join(File.dirname(__FILE__), "factories", name))
  end

  # copy of a factory image cut after 'length' bytes
  def truncated_copy(name, length, dir)
    path = File.join(dir, "truncated_#{name}")
    File.binwrite(path, File.binread(factory_path(name), length))
    path
  end
end

RSpec.configure do |config|
  config.expect_with :rspec do |c|
    c.syntax = :expect
  end
  config.include SpecHelpers
end
