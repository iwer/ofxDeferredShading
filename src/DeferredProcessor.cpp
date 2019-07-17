#include "DeferredProcessor.hpp"

using namespace ofxDeferred;

const std::string RenderPassRegistry::Bg("BgPass");
const std::string RenderPassRegistry::Bloom("BloomPass");
const std::string RenderPassRegistry::Blur("BlurPass");
const std::string RenderPassRegistry::Dof("DofPass");
const std::string RenderPassRegistry::Edge("EdgePass");
const std::string RenderPassRegistry::PointLight("PointLightPass");
const std::string RenderPassRegistry::ShadowLight("ShadowLightPass");
const std::string RenderPassRegistry::Ssao("SsaoPass");

RenderPass::RenderPass(const glm::vec2& sz, const std::string& n) : size(sz), name(n) {
	group.setName(name);
	group.add(enabled.set("active", true));
}

Processor::Processor() : isProcessing(true) {
	params.setName("ofxDeferredShading");
}

void Processor::init(unsigned w, unsigned h) {
	width = w;
	height = h;

	ofFbo::Settings s;
	s.width = width;
	s.height = height;
	s.internalformat = GL_RGBA;
	s.numSamples = 4;
	s.useDepth = true;
	s.useStencil = true;
	s.depthStencilAsTexture = true;

	for (int i = 0; i < 2; i++) {
		pingPong[i].allocate(s);
	}

	currentReadFbo = 0;

	gbuffer.setup(width, height);

	params.setName("deferred");
}

void Processor::begin(ofCamera& cam, bool bUseOwnShader) {

	for (auto pass : passes) {
		if (pass->getEnabled()) {
			pass->update(cam);
		}
	}

	gbuffer.begin(cam, bUseOwnShader);

}

void Processor::end(bool autoDraw) {

	gbuffer.end();

	if (isProcessing) {
		ofPushStyle();
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		ofSetColor(255);
		process();
		if (autoDraw) draw();
		glPopAttrib();
		ofPopStyle();
	}
	
}

void Processor::draw(float x, float y) const {
	draw(x, y, width, height);
}

void Processor::draw(float x, float y, float w, float h) const {
	pingPong[currentReadFbo].draw(0, 0, w, h);
}

void Processor::debugDraw() {
	gbuffer.debugDraw();
}

void Processor::process() {

	int numProcessedPasses = 0;
	for (auto pass : passes) {
		if (pass->getEnabled()) {
			if (numProcessedPasses == 0) pass->render(gbuffer.getFbo().getTexture(), pingPong[1 - currentReadFbo], gbuffer);
			else pass->render(pingPong[currentReadFbo].getTexture(), pingPong[1 - currentReadFbo], gbuffer);
			currentReadFbo = 1 - currentReadFbo;
			numProcessedPasses++;
		}
	}

	if (numProcessedPasses == 0) {
		pingPong[1 - currentReadFbo].begin();
		ofClear(0.);
		gbuffer.getTexture(GBuffer::TYPE_ALBEDO).draw(0, 0);
		pingPong[1 - currentReadFbo].end();
		currentReadFbo = 1 - currentReadFbo;
	}

}
