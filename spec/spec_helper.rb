require 'rspec'
require 'tmpdir'

# WEBP_FFI_INSTALLED_GEM=<platform> tests the installed gem instead of lib/,
# for example a prebuilt platform gem on CI. rspec puts lib/ on the load path,
# so remove it to be sure the installed copy is the one that loads.
if ENV['WEBP_FFI_INSTALLED_GEM']
  $LOAD_PATH.delete(File.expand_path("../lib", __dir__))
  gem 'webp-ffi'
  require 'webp_ffi'
else
  require File.dirname(__FILE__) + "/../lib/webp_ffi"
end

Dir[File.join(__dir__, "support", "*.rb")].sort.each { |file| require file }

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
  config.include ImageBuilder
  # specs for the build and packaging code need the source tree
  config.filter_run_excluding :source_tree if ENV['WEBP_FFI_INSTALLED_GEM']
  # RBS runtime type checks (RBS_TEST_TARGET) reject calls with wrong types
  # before the gem can, and wrap the WebP methods
  config.filter_run_excluding :outside_type_check if ENV['RBS_TEST_TARGET']
end
