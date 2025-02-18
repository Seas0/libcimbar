/* This code is subject to the terms of the Mozilla Public License, v.2.0.
 * http://mozilla.org/MPL/2.0/. */
#include "cimbar_js.h"

#include "cimb_translator/Config.h"
#include "encoder/SimpleEncoder.h"
#include "gui/window_glfw.h"
#include "util/byte_istream.h"

#include <iostream>
#include <sstream>

namespace {
// shared global objects
std::shared_ptr<cimbar::window_glfw> _window;
std::shared_ptr<fountain_encoder_stream> _fountainEncoderStream;
std::optional<cv::Mat> _nextFrame;

// encoder state
int _frameCount = 0;
uint8_t _encodeId = 109;

// settings
unsigned _ecc = cimbar::Config::ecc_bytes();
unsigned _colorBits = cimbar::Config::color_bits();
int _compressionLevel = cimbar::Config::compression_level();
bool _legacyMode = false;
bool _shaking = true;
} // namespace

extern "C" {

int initialize_GL(int width, int height) {
  if (_window)
    return 1;

  // must be divisible by 4???
  if (width % 4 != 0)
    width += (4 - width % 4);
  if (height % 4 != 0)
    height += (4 - height % 4);
  std::cerr << "initializing " << width << " by " << height << " window";

  _window =
      std::make_shared<cimbar::window_glfw>(width, height, "Cimbar Encoder");
  if (!_window or !_window->is_good())
    return 0;

  return 1;
}

// render() and next_frame() could be put in the same function,
// but it seems cleaner to split them.
// in any case, we're concerned with frame pacing (some encodes take longer than
// others)
int render() {
  if (!_window or !_fountainEncoderStream or _window->should_close())
    return -1;

  if (_nextFrame) {
    _window->show(*_nextFrame, 0);
    if (_shaking)
      _window->shake();
    return 1;
  }
  return 0;
}

int next_frame() {
  if (!_window || !_fountainEncoderStream)
    return 0;

  // we generate 8x the amount of required symbol blocks.
  // this number is somewhat arbitrary, but needs to not be
  // *too* low (1-2), or we risk long runs of blocks the decoder
  // has already seen.
  unsigned required = _fountainEncoderStream->blocks_required() * 8;
  if (_fountainEncoderStream->block_count() > required) {
    _fountainEncoderStream->restart();
    _window->shake(0);
    _frameCount = 0;
  }

  SimpleEncoder enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
  if (_legacyMode)
    enc.set_legacy_mode();

  enc.set_encode_id(_encodeId);
  _nextFrame = enc.encode_next(*_fountainEncoderStream, _window->width());
  return ++_frameCount;
}

int encode(unsigned char *buffer, unsigned size, int encode_id) {
  _frameCount = 0;
  if (!FountainInit::init())
    std::cerr << "failed FountainInit :(" << std::endl;

  SimpleEncoder enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
  if (_legacyMode)
    enc.set_legacy_mode();

  if (encode_id < 0)
    enc.set_encode_id(
        ++_encodeId); // increment _encodeId every time we change files
  else
    enc.set_encode_id(static_cast<uint8_t>(encode_id));

  cimbar::byte_istream bis(reinterpret_cast<char *>(buffer), size);
  _fountainEncoderStream = enc.create_fountain_encoder(bis, _compressionLevel);

  if (!_fountainEncoderStream)
    return 0;

  _nextFrame.reset();
  return 1;
}

int configure(unsigned color_bits, unsigned ecc, unsigned compression,
              bool shaking, bool legacy_mode) {
  // default config values if insane provided
  if (color_bits > 3) // [0,3]
    color_bits = cimbar::Config::color_bits();
  if (ecc >= 150) // [0,149]
    ecc = cimbar::Config::ecc_bytes();
  if (compression > 22) // [0,22]
    compression = cimbar::Config::compression_level();

  // check if we need to reconfigure the stream
  if (bool reconf = (color_bits != _colorBits || ecc != _ecc ||
                     compression != _compressionLevel || shaking != !_shaking ||
                     legacy_mode != _legacyMode);
      reconf) {
    // update config
    _colorBits = color_bits;
    _ecc = ecc;
    _compressionLevel = compression;
    _legacyMode = legacy_mode;
    _shaking = shaking;

    // try to refresh the stream
    if (_window && _fountainEncoderStream) {
      unsigned buff_size_new = cimbar::Config::fountain_chunk_size(
          _ecc, cimbar::Config::symbol_bits() + _colorBits, _legacyMode);
      if (!_fountainEncoderStream->restart_and_resize_buffer(buff_size_new)) {
        // if the data is too small, we should throw out _fountainEncoderStream
        // as there's no need for large data chunking
        _fountainEncoderStream = nullptr;
        // -- and clear the canvas.
        _window->clear();
        _nextFrame.reset();
      }
      // reset the frame count and shaking state
      _frameCount = 0;
      _window->shake(0);
    }
  }
  return 0;
}

int set_encode_id(int encode_id) {
  _encodeId = static_cast<uint8_t>(encode_id);
  return _encodeId;
}

int get_encode_id() { return _encodeId; }

} // extern "C"
