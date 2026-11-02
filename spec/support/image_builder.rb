require 'zlib'

# Builds small, valid test images in memory, so the specs need no extra
# fixture files.
module ImageBuilder
  module_function

  def png_chunk(type, data)
    [data.bytesize].pack("N") + type.b + data.b + [Zlib.crc32(type + data)].pack("N")
  end

  # 'rows' are the scanlines without filter bytes, in the order they are
  # stored (for interlaced images, pass after pass). 'chunks' go before IDAT.
  def png(width, height, rows:, bit_depth: 8, color_type: 2, interlace: 0, chunks: [])
    header = [width, height, bit_depth, color_type, 0, 0, interlace].pack("NNC5")
    raw = rows.map { |row| "\0".b + row.b }.join
    "\x89PNG\r\n\x1A\n".b + png_chunk("IHDR", header) + chunks.join +
      png_chunk("IDAT", Zlib::Deflate.deflate(raw)) + png_chunk("IEND", "")
  end

  def png_iccp(profile)
    png_chunk("iCCP", "icc\0\0".b + Zlib::Deflate.deflate(profile))
  end

  def png_exif(exif)
    png_chunk("eXIf", exif)
  end

  def png_xmp(xmp)
    # iTXt: keyword, compression flag and method, language, translated keyword
    png_chunk("iTXt", "XML:com.adobe.xmp\0\0\0\0\0".b + xmp.b)
  end

  # tEXt chunk, or zTXt when 'compressed'
  def png_text(key, text, compressed: false)
    if compressed
      png_chunk("zTXt", key.b + "\0\0".b + Zlib::Deflate.deflate(text.b))
    else
      png_chunk("tEXt", key.b + "\0".b + text.b)
    end
  end

  # ImageMagick's "raw profile" text: "\n<name>\n<length>\n<hex lines>\n"
  def raw_profile(name, data)
    hex = data.unpack1("H*").scan(/.{1,72}/).join("\n")
    "\n#{name}\n#{format('%8d', data.bytesize)}\n#{hex}\n"
  end

  # little-endian, uncompressed 8-bit RGB with optional ICC and XMP tags
  def tiff(width, height, pixels:, icc: nil, xmp: nil)
    entries = [[256, 4, 1, width], [257, 4, 1, height], [258, 3, 3, :bits], [259, 3, 1, 1],
               [262, 3, 1, 2], [273, 4, 1, :strip], [277, 3, 1, 3], [278, 4, 1, height],
               [279, 4, 1, pixels.bytesize], [284, 3, 1, 1]]
    entries << [700, 1, xmp.bytesize, :xmp] if xmp
    entries << [34675, 7, icc.bytesize, :icc] if icc
    blobs = { bits: [8, 8, 8].pack("v3"), strip: pixels.b, xmp: xmp&.b, icc: icc&.b }.compact
    data_start = 8 + 2 + entries.size * 12 + 4
    data = "".b
    offsets = blobs.to_h do |name, blob|
      data << "\0" if (data_start + data.bytesize).odd?  # word aligned
      offset = data_start + data.bytesize
      data << blob
      [name, offset]
    end
    ifd = [entries.size].pack("v") + entries.map do |tag, type, count, value|
      field = if value.is_a?(Symbol) then [offsets.fetch(value)].pack("V")
              elsif type == 3 then [value, 0].pack("vv")
              else [value].pack("V")
              end
      [tag, type, count].pack("vvV") + field
    end.join + [0].pack("V")
    "II*\0".b + [8].pack("V") + ifd + data
  end

  # 'jpeg' with extra marker segments ([marker, payload] pairs) after SOI
  def jpeg_with_segments(jpeg, segments)
    jpeg.byteslice(0, 2) + segments.map { |marker, payload|
      [0xFF, marker, payload.bytesize + 2].pack("CCn") + payload.b
    }.join + jpeg.byteslice(2..)
  end

  # APP2 segments holding an ICC profile split into 'parts', in file order
  def jpeg_icc_segments(profile, parts: 1, order: nil)
    size = (profile.bytesize / parts.to_f).ceil
    chunks = profile.b.chars.each_slice(size).map(&:join)
    segments = chunks.each_with_index.map do |chunk, i|
      [0xE2, "ICC_PROFILE\0".b + [i + 1, chunks.size].pack("CC") + chunk]
    end
    order ? order.map { |i| segments[i] } : segments
  end

  def jpeg_exif_segment(exif) = [0xE1, "Exif\0\0".b + exif.b]
  def jpeg_xmp_segment(xmp) = [0xE1, "http://ns.adobe.com/xap/1.0/\0".b + xmp.b]

  # A minimal ICC v2 display profile (header and an empty tag table) that
  # libpng accepts. 'tag' pads it so tests can tell profiles apart. The fixed
  # pseudo-random padding keeps it from compressing below the 92-byte iCCP
  # chunk that libpng 1.6.59 needs, like a real profile.
  def icc_profile(tag = "")
    profile = ("\0" * 132).b + tag.b + Random.new(42).bytes(256)
    profile << "\0" until (profile.bytesize % 4).zero?  # libpng wants whole words
    profile[0, 4] = [profile.bytesize].pack("N")
    profile[8, 4] = [0x02100000].pack("N")         # version 2.1
    profile[12, 4] = "mntr"                          # display device
    profile[16, 4] = "RGB "                          # color space
    profile[20, 4] = "XYZ "                          # connection space
    profile[36, 4] = "acsp"                          # signature
    profile[68, 12] = [0xF6D6, 0x10000, 0xD32D].pack("N3")  # D50 illuminant
    profile
  end

  # EXIF data: a TIFF header with an empty first directory
  def exif
    "II*\0".b + [8].pack("V") + [0].pack("v") + [0].pack("V")
  end

  def xmp(title = "webp-ffi")
    %(<x:xmpmeta xmlns:x="adobe:ns:meta/"><dc:title>#{title}</dc:title></x:xmpmeta>)
  end

  # An animated WebP with 'frames' copies of a still lossless WebP
  def animated_webp(still, frames: 2)
    width, height = WebP.webp_size(still)
    le24 = ->(value) { [value].pack("V").byteslice(0, 3) }
    image = still.byteslice(12..)  # the VP8L chunk
    frame = le24.(0) + le24.(0) + le24.(width - 1) + le24.(height - 1) + le24.(100) + "\0".b + image
    frame << "\0" if frame.bytesize.odd?
    vp8x = "VP8X".b + [10].pack("V") + [0x02 | 0x10].pack("C") + "\0\0\0".b + le24.(width - 1) + le24.(height - 1)
    anim = "ANIM".b + [6].pack("V") + [0xFFFFFFFF].pack("V") + [0].pack("v")
    anmf = ("ANMF".b + [frame.bytesize].pack("V") + frame) * frames
    body = "WEBP".b + vp8x + anim + anmf
    "RIFF".b + [body.bytesize].pack("V") + body
  end

  # WebP chunks after the RIFF header: [[tag, payload], ...]
  def webp_chunks(webp)
    chunks = []
    offset = 12
    while offset + 8 <= webp.bytesize
      tag = webp.byteslice(offset, 4)
      size = webp.byteslice(offset + 4, 4).unpack1("V")
      chunks << [tag, webp.byteslice(offset + 8, size)]
      offset += 8 + size + (size & 1)
    end
    chunks
  end
end
