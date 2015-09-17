#include "DotsVisualization.h"

#include <vector>

using namespace std;

const float DAMPING = 0.1;

void DotsVisualization::setup(AudioSource* audioSource, BeatDetector* beatDetector)
{
	ShaderVisualization::setup("dots.frag");

	mLoudness = 1.0;
	mHue = 0.0;
	mAccumulatedLoudness = 0.0f;
	mAudioSource = audioSource;
	mBeatDetector = beatDetector;
}

void DotsVisualization::renderUniforms()
{
	ShaderVisualization::renderUniforms();

	mAudioSource->update();
	mBeatDetector->update(1.5);
	mAccumulatedLoudness += mBeatDetector->getBeat() + mAudioSource->getVolume() * mLoudness;

	vector<float> eqs = mAudioSource->getEqs(BIN_COUNT);

	for (int i = 0; i < BIN_COUNT; ++i) {
		if (eqs[i] > mEqs[i]) {
			mEqs[i] = eqs[i] * mLoudness;
		}
		else {
			mEqs[i] -= (mEqs[i] - eqs[i]) * DAMPING;
		}
	}


	mShader->uniform("i_eqs", &mEqs[0], BIN_COUNT);
	mShader->uniform("i_accumulatedLoudness", mAccumulatedLoudness);
	mShader->uniform("i_hue", mHue);
}

void DotsVisualization::switchParams(params::InterfaceGlRef params, const string &group) {
	addParamName(group + "/Loudness");
	params->addParam(group + "/Loudness", &mLoudness)
		.min(0.0)
		.max(2.0)
		.step(0.001)
		.group(group);

	addParamName(group + "/Hue");
	params->addParam(group + "/Hue", &mHue)
		.min(0.0)
		.max(1.0)
		.step(0.01)
		.group(group);
}
