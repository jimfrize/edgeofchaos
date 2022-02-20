#include "plugin.hpp"

struct Syn : Module
{
	enum ParamId
	{
		RUN_PARAM,
		BPM_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		RUN_CV_INPUT,
		BPM_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		OUTPUTS_LEN
	};
	enum LightId
	{
		ENUMS(RUN_LIGHT, 3),
		LIGHTS_LEN
	};

	bool tog = 0.f;
	std::shared_ptr<Font> font;
	float tempo =120.0f;

	Syn()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BPM_PARAM, 0, 999, 120, "BPM");
		configSwitch(RUN_PARAM, 0.f, 1.f, 1.f, "run");
		configInput(RUN_CV_INPUT, "run CV");
		configInput(BPM_CV_INPUT, "BPM CV");
	}

	void process(const ProcessArgs& args) override
	{
		_SYNC = true;


		if(inputs[BPM_CV_INPUT].isConnected())
		{
			float cv = clamp(inputs[BPM_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			params[BPM_PARAM].setValue( std::round( cv * 999.f ) );
		}

		_BPM = std::round(params[BPM_PARAM].getValue());
		tempo = _BPM;

		/////////////////////////////////////////////
		// get toggle switch state and set run LED //
		/////////////////////////////////////////////

		if( inputs[RUN_CV_INPUT].isConnected() )
		{
			tog = inputs[RUN_CV_INPUT].getVoltage() > 0.f;
		}

		else
		{
			tog = params[RUN_PARAM].getValue() > 0.f;
		}

		lights[RUN_LIGHT + 0].setBrightness(tog);
		lights[RUN_LIGHT + 1].setBrightness(tog);
		_RUN = tog;
	}

	void onRemove() override
	{
        _SYNC = false;
    }
};

struct BpmDisplayWidget : TransparentWidget
{
	float *value = NULL;
	std::shared_ptr<Font> font;
	std::string fontPath = asset::plugin(pluginInstance, "res/SyneMono-Regular.ttf");

	void drawLayer(const DrawArgs& args, int layer) override
	{
		if (layer != 1){ return; }
		if (!value){ return; }

		font = APP->window->loadFont(fontPath);

		if (font)
		{
			nvgFontSize(args.vg, 18);
			nvgFontFaceId(args.vg, font->handle);
			std::string bpmString = std::to_string((int) *value);
			NVGcolor textColor = nvgRGB(0xff, 0xff, 0x00);
			nvgFillColor(args.vg, textColor);

			float stringOffset = 3.f - ( (bpmString.length() - 1) * 4.f );
			nvgText(args.vg, stringOffset, 120.f, bpmString.data(), NULL);
		}
	}
};

struct BigPot : app::SvgKnob
{
    widget::SvgWidget* bg;
 
    BigPot()
	{
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;
 
        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);
 
        setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundBigBlackKnob.svg")));
        bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundBigBlackKnob_bg.svg")));
		snap = true;
		shadow->opacity = 0.0;
    }
};

struct JACKPort : app::SvgPort
{
    JACKPort()
	{
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/jack.svg")));
		shadow->opacity = 0.0;
    }
};

struct SynWidget : ModuleWidget
{
	SynWidget(Syn* module)
	{
		float vertical_spacing = 8.5f;
		float vertical_offset = 10.f;
		float horizontal_spacing = 5.08f;

		//BPM DISPLAY 
		BpmDisplayWidget *display = new BpmDisplayWidget();
		display->box.pos = Vec(23,45);
		display->box.size = Vec(45, 20);

		if (module)
		{
      		display->value = &module->tempo;
		}

		addChild(display);

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/syn.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<PB61303>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 1.5f + vertical_offset)), module, Syn::RUN_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedGreenBlueLight>>>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 1.5f + vertical_offset)), module, Syn::RUN_PARAM, Syn::RUN_LIGHT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 3.f + vertical_offset)), module, Syn::RUN_CV_INPUT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 8.75f + vertical_offset)), module, Syn::BPM_CV_INPUT));
		addParam(createParamCentered<BigPot>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 10.5f + vertical_offset)), module, Syn::BPM_PARAM));
	}
};

Model* modelSyn = createModel<Syn, SynWidget>("syn");