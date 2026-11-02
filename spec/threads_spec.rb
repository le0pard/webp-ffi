require 'spec_helper'

describe "WebP with threads" do
  around do |example|
    Dir.mktmpdir("webp-ffi") do |dir|
      @dir = dir
      example.run
    end
  end

  # Counts how often a background thread wakes up while the block runs. While a
  # C call holds Ruby's Global VM Lock, the background thread cannot run at all.
  def heartbeats_during
    beats = 0
    heartbeat = Thread.new { loop { beats += 1; sleep 0.001 } }
    sleep 0.01
    before = beats
    yield
    beats - before
  ensure
    heartbeat&.kill
  end

  it "lets other threads run while encoding" do
    beats = heartbeats_during { WebP.encode(factory_path("4.jpg"), File.join(@dir, "4.webp"), method: 6) }
    expect(beats).to be >= 10
  end

  it "lets other threads run while decoding" do
    beats = heartbeats_during { WebP.decode(factory_path("5.webp"), File.join(@dir, "5.png")) }
    expect(beats).to be >= 10
  end

  it "lets other threads run while encoding data in memory" do
    data = File.binread(factory_path("4.jpg"))
    beats = heartbeats_during { WebP.encode_data(data, method: 6) }
    expect(beats).to be >= 10
  end

  it "lets other threads run while decoding data in memory" do
    data = File.binread(factory_path("5.webp"))
    beats = heartbeats_during { WebP.decode_data(data) }
    expect(beats).to be >= 10
  end

  it "encodes data from several threads at once" do
    data = File.binread(factory_path("1.png"))
    results = Array.new(4) { Thread.new { WebP.encode_data(data) } }.map(&:value)
    expect(results.uniq.size).to eq(1)
    expect(WebP.webp_size(results.first)).to eq([400, 301])
  end

  it "gives the same results when many threads mix every call" do
    png_data = File.binread(factory_path("1.png"))
    jpg = File.binread(factory_path("3.jpg"))
    webp = File.binread(factory_path("1.webp"))
    calls = {
      encode_png: -> { WebP.encode_data(png_data, method: 0) },
      encode_jpg: -> { WebP.encode_data(jpg, method: 0, quality: 50) },
      decode: -> { WebP.decode_data(webp, output_format: :tiff) },
      rgba: -> { WebP.decode_rgba(webp, resize_w: 40, resize_h: 30) },
      info: -> { WebP.info(webp) },
      broken: -> { WebP.decode_data("broken") rescue $!.class }
    }
    expected = calls.transform_values(&:call)
    results = Array.new(8) do |t|
      Thread.new do
        Array.new(18) { |i| name = calls.keys[(t + i) % calls.size]; [name, calls.fetch(name).call] }
      end
    end.flat_map(&:value)
    expect(results.size).to eq(8 * 18)
    results.each { |name, value| expect(value).to eq(expected.fetch(name)), "#{name} differs" }
  end

  it "encodes the same image from several threads at once" do
    outputs = Array.new(4) { |i| File.join(@dir, "#{i}.webp") }
    outputs.map { |output| Thread.new { WebP.encode(factory_path("1.png"), output) } }.each(&:join)
    expect(outputs.map { |output| File.binread(output) }.uniq.size).to eq(1)
    expect(WebP.webp_size(File.binread(outputs.first))).to eq([400, 301])
  end
end
