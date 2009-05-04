#include "ARBBumpProgram.h"
#include "render/backend/GLProgramFactory.h"

#include "igame.h"
#include "string/string.h"

namespace render
{

namespace
{
    /* CONSTANT FRAGMENT PROGRAM PARAMETERS
     * These should match what is used by interaction_fp.cg
     */
    const int C2_LIGHT_ORIGIN = 2;
    const int C3_LIGHT_COLOR = 3;
    const int C4_VIEW_ORIGIN = 4;
    const int C6_LIGHT_SCALE = 6;
    const int C7_AMBIENT_FACTOR = 7;

    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

}

// Main construction
void ARBBumpProgram::create()
{
	// Initialise the lightScale value
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
	if (!scaleList.empty()) 
    {
		_lightScale = strToDouble(scaleList[0].getContent());
	}
	else {
		_lightScale = 1.0;
	}

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    {
      glGenProgramsARB(1, &m_vertex_program);
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);


        // Create the vertex program
        GLProgramFactory::createARBProgram(
            GLProgramFactory::getGLProgramPath(BUMP_VP_FILENAME),
            GL_VERTEX_PROGRAM_ARB
        );

      glGenProgramsARB(1, &m_fragment_program);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

        // Create the fragment program
        GLProgramFactory::createARBProgram(
            GLProgramFactory::getGLProgramPath(BUMP_FP_FILENAME),
            GL_FRAGMENT_PROGRAM_ARB
        );    
    }

    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::destroy()
{
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);
    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::enable()
{
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::disable()
{
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::applyRenderParams(const Vector3& viewer, 
                                       const Matrix4& objectToWorld, 
                                       const Vector3& origin, 
                                       const Vector3& colour, 
                                       const Matrix4& world2light,
                                       float ambientFactor)
{
    Matrix4 worldToObject(objectToWorld);
    matrix4_affine_invert(worldToObject);

    // Calculate the light origin in object space
    Vector3 localLight(origin);
    matrix4_transform_point(worldToObject, localLight);

    Matrix4 local2light(world2light);
    local2light.multiplyBy(objectToWorld); // local->world->light

    // view origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C4_VIEW_ORIGIN, viewer.x(), viewer.y(), viewer.z(), 0
    );

    // light origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C2_LIGHT_ORIGIN, localLight.x(), localLight.y(), localLight.z(), 1
    );

    // light colour
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C3_LIGHT_COLOR, colour.x(), colour.y(), colour.z(), 0
    );

	// light scale
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C6_LIGHT_SCALE, _lightScale, _lightScale, _lightScale, 0
    );

	// ambient factor
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C7_AMBIENT_FACTOR, ambientFactor, 0, 0, 0
    );

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL_debugAssertNoErrors();
}

}




