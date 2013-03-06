# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'webp/ffi/version'

Gem::Specification.new do |spec|
  spec.name          = "webp-ffi"
  spec.version       = Webp::Ffi::VERSION
  spec.authors       = ["Alexey Vasyliev"]
  spec.email         = ["leopard.not.a@gmail.com"]
  spec.description   = %q{Ruby wrapper for libwebp}
  spec.summary       = %q{Ruby wrapper for libwebp}
  spec.homepage      = "http://leopard.in.ua/webp-ffi"
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.3"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rake-compiler", "~> 0.8.3"
end
