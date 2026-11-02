require 'spec_helper'

describe "webp-ffi.gemspec", :source_tree do
  let(:root) { File.expand_path("..", __dir__) }
  let(:gemspec) { Gem::Specification.load(File.join(root, "webp-ffi.gemspec")) }

  it "ships every C source and the build files" do
    sources = Dir.chdir(root) { Dir["ext/webp_ffi/*.{c,h}"] }
    expect(sources).not_to be_empty
    expect(gemspec.files).to include(*sources, "ext/webp_ffi/Rakefile", "ext/webp_ffi/build_config.rb")
  end

  it "ships the Ruby library" do
    ruby_files = Dir.chdir(root) { Dir["lib/**/*.rb"] }
    expect(gemspec.files).to include(*ruby_files)
  end

  it "leaves out specs, test images and build output" do
    expect(gemspec.files.grep(%r{\Aspec/|\.(o|so|bundle|dylib)\z})).to be_empty
  end

  it "needs Ruby 3.2 or newer, the oldest version CI tests" do
    expect(gemspec.required_ruby_version).to eq(Gem::Requirement.new(">= 3.2"))
  end

  it "builds the extension with ext/webp_ffi/Rakefile" do
    expect(gemspec.extensions).to eq(["ext/webp_ffi/Rakefile"])
  end
end
