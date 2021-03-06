#include "FlockingVisualization.h"

#include "cinder/app/App.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"

using namespace ci;

const int FLOCK_WIDTH = 50;
const int BUFFER_WIDTH = 128;
const int NUM_PARTICLES = BUFFER_WIDTH * BUFFER_WIDTH;

FlockingVisualization::FlockingVisualization()
{

	mLoudness = 1.0;
	mAccumulatedLoudness = 0.0f;
	mBeatConstant = 1.4;

	mSeparateOnly = false;

	mStep = true;
	mIteratonIndex = 0;

	std::vector<std::string> feedbackVaryings({
		"tf_position",
		"tf_velocity",
		"tf_color"
	});
	gl::GlslProg::Format updateFormat;
	updateFormat.vertex(app::loadAsset("flocking_update.vert"))
		.feedbackFormat(GL_SEPARATE_ATTRIBS)
		.feedbackVaryings(feedbackVaryings);

	mUpdateShader = gl::GlslProg::create(updateFormat);

	gl::GlslProg::Format renderFormat;
	renderFormat.vertex(app::loadAsset("flocking.vert"))
		.fragment(app::loadAsset("point_circle.frag"));

	mRenderShader = gl::GlslProg::create(renderFormat);

	std::array<vec3, NUM_PARTICLES> positions;
	std::array<vec3, NUM_PARTICLES> velocities;
	std::array<vec3, NUM_PARTICLES> colors;


	for (int i = 0; i < NUM_PARTICLES; ++i) {
		positions[i] = vec3(FLOCK_WIDTH * (Rand::randFloat() - 0.5f),
			FLOCK_WIDTH * (Rand::randFloat() - 0.5f),
			FLOCK_WIDTH * (Rand::randFloat() - 0.5f));

		velocities[i] = Rand::randVec3();
		colors[i] = vec3(0, 0, 0);
	}

	for (int i = 0; i < 2; ++i) {
		mVaos[i] = gl::Vao::create();
		gl::ScopedVao scopeVao(mVaos[i]);
		{
			mPositions[i] = gl::Vbo::create(GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW);
			{
				gl::ScopedBuffer scopeBuffer(mPositions[i]);
				gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(0);
			}

			mVelocities[i] = gl::Vbo::create(GL_ARRAY_BUFFER, velocities.size() * sizeof(vec3), velocities.data(), GL_STATIC_DRAW);
			{
				gl::ScopedBuffer scopeBuffer(mVelocities[i]);
				gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(1);
			}

			mColors[i] = gl::Vbo::create(GL_ARRAY_BUFFER, colors.size() * sizeof(vec3), colors.data(), GL_STATIC_DRAW);
			{
				gl::ScopedBuffer scopeBuffer(mColors[i]);
				gl::vertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(2);
			}
		}
	}

	mPositionBufTex[0] = gl::BufferTexture::create(mPositions[0], GL_RGB32F);
	mPositionBufTex[1] = gl::BufferTexture::create(mPositions[1], GL_RGB32F);
	mVelocityBufTex[0] = gl::BufferTexture::create(mVelocities[0], GL_RGB32F);
	mVelocityBufTex[1] = gl::BufferTexture::create(mVelocities[1], GL_RGB32F);
}

void FlockingVisualization::switchParams(OscVisController &controller) {
	controller.subscribeSliderListener("Loudness", 0, 2, [&](float val) { mLoudness = val; });
	controller.subscribeSliderListener("Beat Constant", 1.1, 2, [&](float val) { mBeatConstant = val; });
	controller.subscribeSliderGlslListener("Speed", 0.5, 4, 2, mUpdateShader, "i_speed");
	controller.subscribeSliderGlslListener("Roaming Distance", 20, 120, 40, mUpdateShader, "i_roamingDistance");
	controller.subscribeSliderGlslListener("Separation Distance", 0, 30, 12, mUpdateShader, "i_separationDistance");
	controller.subscribeSliderGlslListener("Cohesion Distance", 0, 30, 8, mUpdateShader, "i_cohesionDistance");
	controller.subscribeSliderGlslListener("Alignment Distance", 0, 30, 6, mUpdateShader, "i_alignmentDistance");
}


void FlockingVisualization::update(const World& world)
{
	world.audioSource->update();
	float loudness = audio::linearToDecibel(world.audioSource->getVolume()) * 0.01f * mLoudness;;
	mAccumulatedLoudness += loudness;

	mUpdateShader->uniform("i_delta", world.deltaSource->delta());
	mUpdateShader->uniform("i_loudness", loudness);
	mUpdateShader->uniform("i_accumulatedLoudness", mAccumulatedLoudness);
	mUpdateShader->uniform("i_beat", world.beatDetector->getBeat());
	mUpdateShader->uniform("i_eqs", &(world.audioSource->getEqs(3, mLoudness))[0], 3);


	gl::ScopedVao scopedVao(mVaos[mIteratonIndex & 1]);
	gl::ScopedTextureBind scopeTexPos(mPositionBufTex[mIteratonIndex & 1]->getTarget(), mPositionBufTex[mIteratonIndex & 1]->getId(), 0);
	gl::ScopedTextureBind scopeTexVel(mVelocityBufTex[mIteratonIndex & 1]->getTarget(), mVelocityBufTex[mIteratonIndex & 1]->getId(), 1);

	mUpdateShader->uniform("tex_position", 0);
	mUpdateShader->uniform("tex_velocity", 1);

	gl::ScopedGlslProg glsl(mUpdateShader);
	gl::ScopedState scopeState(GL_RASTERIZER_DISCARD, true);

	mIteratonIndex++;

	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mPositions[mIteratonIndex & 1]);
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, mVelocities[mIteratonIndex & 1]);
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, mColors[mIteratonIndex & 1]);

	gl::beginTransformFeedback(GL_POINTS);
	gl::drawArrays(GL_POINTS, 0, NUM_PARTICLES);

	gl::endTransformFeedback();
}

void FlockingVisualization::draw(const World& world)
{
	world.camera->lookAt(vec3(0.0, 0.0, 100.0), vec3(0.0));

	gl::pushMatrices();
	gl::setMatrices(*world.camera);
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::ScopedGlslProg glsl(mRenderShader);
	gl::ScopedVao scopedVao(mVaos[mIteratonIndex & 1]);
	gl::context()->setDefaultShaderVars();

	gl::ScopedState pointSize(GL_PROGRAM_POINT_SIZE, true);
	gl::drawArrays(GL_POINTS, 0, NUM_PARTICLES);

	gl::popMatrices();
}