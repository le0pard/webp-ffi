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

  it "encodes the same image from several threads at once" do
    outputs = Array.new(4) { |i| File.join(@dir, "#{i}.webp") }
    outputs.map { |output| Thread.new { WebP.encode(factory_path("1.png"), output) } }.each(&:join)
    expect(outputs.map { |output| File.binread(output) }.uniq.size).to eq(1)
    expect(WebP.webp_size(File.binread(outputs.first))).to eq([400, 301])
  end
end
