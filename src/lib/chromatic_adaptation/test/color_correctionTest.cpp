/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "adaptation_transform.h"
#include "color_correction.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using std::string;

TEST_CASE( "color_correctionTest/testTransform", "[unit]" )
{
	cv::Matx<double, 3, 3> mat = color_correction::get_adaptation_matrix<adaptation_transform::von_kries>({192, 255, 255}, {255, 255, 255});

	{
		std::stringstream ss;
		ss << mat;
		assertEquals( "[1.065577644398845, 0.2109225385610203, -0.01323982375544511;\n"
		              " 0.02316834486251534, 0.9872336916455102, -0.00467809248316883;\n"
		              " 0, 0, 1]", ss.str() );
	}

	std::tuple<double, double, double> c = color_correction(std::move(mat)).transform(180, 98, 255);
	assertAlmostEquals( 209.09822971, std::get<0>(c) );
	assertAlmostEquals( 99.72629027, std::get<1>(c) );
	assertAlmostEquals( 255, std::get<2>(c) );
}

