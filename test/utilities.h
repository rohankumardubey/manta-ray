#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <pch.h>

#include <scalar_map_2d.h>
#include <complex_map_2d.h>
#include <image_plane.h>
#include <complex_map_2d.h>
#include <vector_map_2d.h>

#include <piranha.h>
#include <string>

using namespace manta;

#define CHECK_VEC(v, ex, ey, ez, ew) {\
		math::real x = math::getX((v)); \
		math::real y = math::getY((v)); \
		math::real z = math::getZ((v)); \
		math::real w = math::getW((v)); \
		EXPECT_NEAR(x, (ex), 1E-7); \
		EXPECT_NEAR(y, (ey), 1E-7); \
		EXPECT_NEAR(z, (ez), 1E-7); \
		EXPECT_NEAR(w, (ew), 1E-7); \
	}

#define CHECK_VEC_EQ(a, b, eps) {\
		math::real xa = math::getX((a)); \
		math::real ya = math::getY((a)); \
		math::real za = math::getZ((a)); \
		math::real wa = math::getW((a)); \
		math::real xb = math::getX((b)); \
		math::real yb = math::getY((b)); \
		math::real zb = math::getZ((b)); \
		math::real wb = math::getW((b)); \
		EXPECT_NEAR(xa, (xb), eps); \
		EXPECT_NEAR(ya, (yb), eps); \
		EXPECT_NEAR(za, (zb), eps); \
		EXPECT_NEAR(wa, (wb), eps); \
	}

#define CHECK_VEC3_EQ(a, b, eps) {\
		math::real xa = math::getX((a)); \
		math::real ya = math::getY((a)); \
		math::real za = math::getZ((a)); \
		math::real xb = math::getX((b)); \
		math::real yb = math::getY((b)); \
		math::real zb = math::getZ((b)); \
		EXPECT_NEAR(xa, (xb), eps); \
		EXPECT_NEAR(ya, (yb), eps); \
		EXPECT_NEAR(za, (zb), eps); \
	}

#define CHECK_QUAT(v, ew, ex, ey, ez) {\
		math::real x = math::getQuatX((v)); \
		math::real y = math::getQuatY((v)); \
		math::real z = math::getQuatZ((v)); \
		math::real w = math::getQuatW((v)); \
		EXPECT_NEAR(x, (ex), 1E-7); \
		EXPECT_NEAR(y, (ey), 1E-7); \
		EXPECT_NEAR(z, (ez), 1E-7); \
		EXPECT_NEAR(w, (ew), 1E-7); \
	}

#define CHECK_QUAT_EQ(v, ev) {\
		math::real x = math::getQuatX((v)); \
		math::real y = math::getQuatY((v)); \
		math::real z = math::getQuatZ((v)); \
		math::real w = math::getQuatW((v)); \
		math::real ex = math::getQuatX((ev)); \
		math::real ey = math::getQuatY((ev)); \
		math::real ez = math::getQuatZ((ev)); \
		math::real ew = math::getQuatW((ev)); \
		EXPECT_NEAR(x, (ex), 1E-7); \
		EXPECT_NEAR(y, (ey), 1E-7); \
		EXPECT_NEAR(z, (ez), 1E-7); \
		EXPECT_NEAR(w, (ew), 1E-7); \
	}

#define CMF_PATH "../../../demos/cmfs/"
#define WORKSPACE_PATH "../../../workspace/"
#define TMP_PATH (WORKSPACE_PATH "tmp/")
#define SDL_TEST_FILES "../../../test/sdl/"

void writeToJpeg(const RealMap2D *scalarMap, const std::string &fname);
void writeToJpeg(const VectorMap2D *vectorMap, const std::string &fname);
void writeToJpeg(const ImagePlane *plane, const std::string &fname);
void writeToJpeg(const ComplexMap2D *plane, const std::string &fname, Margins *margins = nullptr);

#define CHECK_SDL_POS(parserStructure, _colStart, _colEnd, _lineStart, _lineEnd)	\
	EXPECT_EQ((parserStructure)->getSummaryToken()->colStart,	(_colStart));		\
	EXPECT_EQ((parserStructure)->getSummaryToken()->colEnd,		(_colEnd));			\
	EXPECT_EQ((parserStructure)->getSummaryToken()->lineStart,	(_lineStart));		\
	EXPECT_EQ((parserStructure)->getSummaryToken()->lineEnd,	(_lineEnd));

#define EXPECT_ERROR_CODE(error, code_)						\
	EXPECT_EQ((error)->getErrorCode().stage, code_.stage);	\
	EXPECT_EQ((error)->getErrorCode().code, code_.code);

#define EXPECT_ERROR_CODE_LINE(error, code_, line)			\
	EXPECT_EQ((error)->getErrorCode().stage, code_.stage);	\
	EXPECT_EQ((error)->getErrorCode().code, code_.code);	\
	EXPECT_EQ((error)->getErrorLocation()->lineStart, line);

piranha::IrCompilationUnit *compileFile(const std::string &filename, const piranha::ErrorList **errList);

#endif /* TEST_UTILITIES_H */
