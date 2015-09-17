#include "AudioShaderVisualization.h"

#include "cinder\gl\gl.h"
#include "cinder\app\App.h"

void AudioShaderVisualization::setup(AudioSource* audioSource, const fs::path &fragmentShader)
{
	ShaderVisualization::setup(fragmentShader);
	mAudioSource = audioSource;
	mHue = 0;
	mCycleHueSpeed = 0.0;
	mVolume = 1.0;
}

void AudioShaderVisualization::switchParams(params::InterfaceGlRef params, const string &group)
{
	addParamName(group + "/Hue");
	params->addParam(group + "/Hue", &mHue)
		.min(0.0)
		.max(1.0)
		.step(0.01)
		.group(group);

	addParamName(group + "/Volume");
	params->addParam(group + "/Volume", &mVolume)
		.min(0.0)
		.max(3.0)
		.step(0.01)
		.group(group);

	addParamName(group + "/Cycle Hue Speed");
	params->addParam(group + "/Cycle Hue Speed", &mCycleHueSpeed)
		.min(0.0)
		.max(0.01667)
		.step(0.0001)
		.group(group);
}

void AudioShaderVisualization::update()
{
	mAudioSource->update();
	mTexture = mAudioSource->getMagSpectrumTexture();

	mHue = glm::fract(mHue + mCycleHueSpeed);
}

void AudioShaderVisualization::renderUniforms()
{
	mShader->uniform("tex_audio", 0);
	mShader->uniform("i_hue", mHue);
	mShader->uniform("i_volume", mVolume);
	ShaderVisualization::renderUniforms();
}

void AudioShaderVisualization::draw()
{
	gl::ScopedGlslProg glScp(mShader);
	gl::ScopedTextureBind texscp(mTexture);

	renderUniforms();

	gl::drawSolidRect(app::getWindowIndex(0)->getBounds());
}