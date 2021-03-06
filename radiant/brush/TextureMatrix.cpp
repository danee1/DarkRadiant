#include "TextureMatrix.h"

#include "texturelib.h"
#include "math/Vector2.h"

// Constructor with empty arguments
TextureMatrix::TextureMatrix() {
	coords[0][0] = 2.0f;
	coords[0][1] = 0.f;
	coords[0][2] = 0.f;
	coords[1][0] = 0.f;
	coords[1][1] = 2.0f;
	coords[1][2] = 0.f;
}

// Construct the BP Definition out of the transformation matrix
// Basically copies over the values from the according components
TextureMatrix::TextureMatrix(const Matrix4& transform) {
	coords[0][0] = transform.xx();
	coords[0][1] = transform.yx();
	coords[0][2] = transform.tx();
	coords[1][0] = transform.xy();
	coords[1][1] = transform.yy();
	coords[1][2] = transform.ty();
}

// Construct a TextureMatrix out of "fake" shift scale rot definitions
TextureMatrix::TextureMatrix(const TexDef& texdef) {
	float r = degrees_to_radians(-texdef._rotate);
	float c = cos(r);
	float s = sin(r);
	float x = 1.0f / texdef._scale[0];
	float y = 1.0f / texdef._scale[1];
	coords[0][0] = x * c;
	coords[1][0] = x * s;
	coords[0][1] = y * -s;
	coords[1][1] = y * c;
	coords[0][2] = -texdef._shift[0];
	coords[1][2] = texdef._shift[1];
}

// shift a texture (texture adjustments) along it's current texture axes
void TextureMatrix::shift(float s, float t)
{
	// x and y are geometric values, which we must compute as ST increments
	// this depends on the texture size and the pixel/texel ratio
	// as a ratio against texture size
	// the scale of the texture is not relevant here (we work directly on a transformation from the base vectors)
	coords[0][2] -= s;
	coords[1][2] += t;
}

// apply same scale as the spinner button of the surface inspector
void TextureMatrix::scale(float s, float t, std::size_t shaderWidth, std::size_t shaderHeight)
{
    // We need to have the shader dimensions applied before calling getFakeTexCoords()
    applyShaderDimensions(shaderWidth, shaderHeight);

	// compute fake shift scale rot
	TexDef texdef = getFakeTexCoords();

	float newXScale = texdef._scale[0] + s;
	float newYScale = texdef._scale[1] + t;

	// Don't allow zero (or almost zero) scale values
	if (float_equal_epsilon(newXScale, 0, 1e-5f) ||
		float_equal_epsilon(newYScale, 0, 1e-5f))
	{
		return;
	}

	// Don't allow sign changes
	if ((newXScale*texdef._scale[0]) < 0.0f ||
		(newYScale*texdef._scale[1]) < 0.0f)
	{
		return;
	}

	// update
	texdef._scale[0] = newXScale;
	texdef._scale[1] = newYScale;

	// compute new normalized texture matrix
	*this = TextureMatrix(texdef);

    // Undo the previous step of adding the texture scale
    addScale(shaderWidth, shaderHeight);
}

// apply same rotation as the spinner button of the surface inspector
void TextureMatrix::rotate(float angle, std::size_t shaderWidth, std::size_t shaderHeight)
{
    // We need to have the shader dimensions applied before calling getFakeTexCoords()
    applyShaderDimensions(shaderWidth, shaderHeight);

	// compute fake shift scale rot
	TexDef texdef = getFakeTexCoords();

	// update
	texdef._rotate += angle;

	// compute new normalized texture matrix
	*this = TextureMatrix(texdef);

    // Undo the previous step of adding the texture scale
    addScale(shaderWidth, shaderHeight);
}

/* greebo: This removes the texture scaling from the
 * coordinates. The resulting coordinates are absolute
 * values within the shader image.
 *
 * An 128x256 texture with scaled coordinates 0.5,0.5
 * would be translated into the coordinates 64,128,
 * pointing to a defined pixel within the texture image.
 */
void TextureMatrix::applyShaderDimensions(std::size_t width, std::size_t height) {
	coords[0][0] *= width;
	coords[0][1] *= width;
	coords[0][2] *= width;
	coords[1][0] *= height;
	coords[1][1] *= height;
	coords[1][2] *= height;
}

/* greebo: this converts absolute coordinates into
 * relative ones, where everything is measured
 * in multiples of the texture x/y dimensions. */
void TextureMatrix::addScale(std::size_t width, std::size_t height) {
	coords[0][0] /= width;
	coords[0][1] /= width;
	coords[0][2] /= width;
	coords[1][0] /= height;
	coords[1][1] /= height;
	coords[1][2] /= height;
}

// compute a fake shift scale rot representation from the texture matrix
// these shift scale rot values are to be understood in the local axis base
// Note: this code looks similar to Texdef_fromTransform, but the algorithm is slightly different.
TexDef TextureMatrix::getFakeTexCoords() const
{
	TexDef texdef;

	texdef._scale[0] = 1.0 / Vector2(coords[0][0], coords[1][0]).getLength();
	texdef._scale[1] = 1.0 / Vector2(coords[0][1], coords[1][1]).getLength();

	texdef._rotate = -radians_to_degrees(arctangent_yx(coords[1][0], coords[0][0]));

	texdef._shift[0] = -coords[0][2];
	texdef._shift[1] = coords[1][2];

	// determine whether or not an axis is flipped using a 2d cross-product
	float cross = Vector2(coords[0][0], coords[0][1]).crossProduct(Vector2(coords[1][0], coords[1][1]));

	if (cross < 0)
	{
		// This is a bit of a compromise when using BPs--since we don't know *which* axis was flipped,
		// we pick one (rather arbitrarily) using the following convention: If the X-axis is between
		// 0 and 180, we assume it's the Y-axis that flipped, otherwise we assume it's the X-axis and
		// subtract out 180 degrees to compensate.
		if (texdef._rotate >= 180.0f)
		{
		      texdef._rotate -= 180.0f;
		      texdef._scale[0] = -texdef._scale[0];
		}
		else {
		      texdef._scale[1] = -texdef._scale[1];
		}
	}

	return texdef;
}

// All texture-projection translation (shift) values are congruent modulo the dimensions of the texture.
// This function normalises shift values to the smallest positive congruent values.
void TextureMatrix::normalise(float width, float height) {
	coords[0][2] = float_mod(coords[0][2], width);
	coords[1][2] = float_mod(coords[1][2], height);
}

/* greebo: This returns the transformation matrix.
 * As the member variables already ARE the matrix
 * components, they are just copied into the right places.
 */
Matrix4 TextureMatrix::getTransform() const {
	// Initialise the return value with the identity matrix
	Matrix4 transform = Matrix4::getIdentity();

	// Just copy the member variables to the according matrix components
	transform.xx() = coords[0][0];
	transform.yx() = coords[0][1];
	transform.tx() = coords[0][2];
	transform.xy() = coords[1][0];
	transform.yy() = coords[1][1];
	transform.ty() = coords[1][2];

	return transform;
}
