require 'spec_helper'
require 'rbs'

# sig/webp.rbs must be valid and cover exactly the public WebP methods.
describe "sig/webp.rbs", :source_tree do
  let(:environment) do
    loader = RBS::EnvironmentLoader.new
    loader.add(path: Pathname(File.expand_path("../sig", __dir__)))
    RBS::Environment.from_loader(loader).resolve_type_names
  end
  let(:builder) { RBS::DefinitionBuilder.new(env: environment) }

  def type_name(name)
    RBS::TypeName.parse("::#{name}")
  end

  def own_methods(definition, name)
    definition.methods.select { |_, method| method.defined_in.to_s == "::#{name}" }.keys
  end

  it "builds every declared type" do
    %w[WebP WebP::Error WebP::EncoderError WebP::DecoderError WebP::InvalidImageFormatError
       WebP::Info WebP::RGBAImage].each do |name|
      expect { builder.build_instance(type_name(name)) }.not_to raise_error
      expect { builder.build_singleton(type_name(name)) }.not_to raise_error
    end
  end

  it "declares every public WebP method", :outside_type_check do
    declared = own_methods(builder.build_singleton(type_name("WebP")), "WebP")
    expect(declared).to match_array(WebP.singleton_methods(false))
  end

  it "declares every method of the result types" do
    { "WebP::Info" => WebP::Info, "WebP::RGBAImage" => WebP::RGBAImage }.each do |name, klass|
      declared = own_methods(builder.build_instance(type_name(name)), name)
      expect(declared).to match_array(klass.instance_methods(false))
    end
  end
end
