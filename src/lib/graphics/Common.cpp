/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Common.h"

#include "base91/base.hpp"
#include "serialize/format.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <opencv2/opencv.hpp>

#include <map>
#include <string>
#include "bitmaps.h"

using cimbar::RGB;
using std::array;
using std::string;
using std::vector;

namespace {
	RGB getColor4(unsigned index)
	{
		// opencv uses BGR, but we don't have to conform to this tyranny
		static constexpr array<RGB, 4> colors = {
		    RGB(0, 0xFF, 0xFF),
		    RGB(0xFF, 0xFF, 0),
		    RGB(0xFF, 0, 0xFF),
		    RGB(0, 0xFF, 0)
		};
		return colors[index];
	}

	RGB getColor8(unsigned index)
	{
		static constexpr array<RGB, 8> colors = {
		    RGB(0, 0xFF, 0xFF), // cyan
		    RGB(0x7F, 0x7F, 0xFF),  // mid-blue
		    RGB(0xFF, 0, 0xFF), // magenta
		    RGB(0xFF, 65, 65), // red
		    RGB(0xFF, 0x9F, 0),  // orange
		    RGB(0xFF, 0xFF, 0), // yellow
		    RGB(0xFF, 0xFF, 0xFF),
		    RGB(0, 0xFF, 0),
		};
		return colors[index];
	}

	string load_file(string path)
	{
		auto it = cimbar::bitmaps.find(path);
		if (it == cimbar::bitmaps.end())
			return "";

		return base91::decode(it->second);
	}
}

namespace cimbar {

cv::Mat load_img(string path)
{
	string bytes = load_file(path);
	if (bytes.empty())
		return cv::Mat();

	int width, height, channels;
	std::unique_ptr<uint8_t[]> imgdata(stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels, STBI_rgb_alpha));
	size_t len = width * height * channels;
	cv::Mat mat(height, width, CV_MAKETYPE(CV_8U, channels));
	std::copy(imgdata.get(), imgdata.get()+len, mat.data);
	cv::cvtColor(mat, mat, cv::COLOR_RGBA2RGB);
	return mat;
}

RGB getColor(unsigned index, unsigned num_colors)
{
	if (num_colors <= 4)
		return getColor4(index);
	else
		return getColor8(index);
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color)
{
	static cv::Vec3b background({0, 0, 0});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath);

	uchar r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);
	cv::MatIterator_<cv::Vec3b> end = tile.end<cv::Vec3b>();
	for (cv::MatIterator_<cv::Vec3b> it = tile.begin<cv::Vec3b>(); it != end; ++it)
	{
		cv::Vec3b& c = *it;
		if (c == background)
		{
			if (dark)
				c = {0, 0, 0};
			else
				c = {255, 255, 255};
			continue;
		}
		c = {r, g, b};
	}
	return tile;
}

}
