#include "plugin.hpp"
#include <random>

struct Datawave : Module
{
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger reseedTrigger;
	dsp::SchmittTrigger seedTrigger;

	std::default_random_engine generator; // we need to use this pRNG as rand() has a global seed value
	std::uniform_real_distribution<float> distribution; // we could use other built-in distributions, but we are the cool kids, so we will roll our own! ;)

	bool seed_flag = false;
	bool reseed_flag = false;
	bool clock_flag = false;
	bool mode_flag = false;
	bool tenx = false;

	float seed_cv = 0.f;
	float scale_cv = 1.f;
	float offset_cv = 0.f;
	float slew_cv = 1.f;
	float tenx_cv = 0.f;

	float target = 0.f;
	float current = 0.f;
	float gen_seed = 0.f;
	float reseed = 0.f;
	float clock = 0.f;
	float slew = 0.f;
	float offset = 0.f;
	float scale = 1.f;

	unsigned int mode = 0;

	enum ParamId
	{
		SEED_PARAM,
		SCALE_PARAM,
		OFFSET_PARAM,
		SLEW_PARAM,
		TENX_PARAM,
		MODE_SWITCH,
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		SEED_CV_INPUT,
		SCALE_CV_INPUT,
		MODE_CV_INPUT,
		OFFSET_CV_INPUT,
		SLEW_CV_INPUT,
		TENX_CV_INPUT,
		GEN_SEED_INPUT,
		RESEED_INPUT,
		CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		RAND_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		ENUMS(MODE_LIGHT, 3),
		ENUMS(TENX_LIGHT, 3),
		LIGHTS_LEN
	};

	Datawave()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SEED_PARAM, 0, RAND_MAX, 0, "seed");
		configSwitch(MODE_SWITCH, 0.f, 1.f, 0.f, "mode");
		configParam(MODE_PARAM, 0, 4, 0, "mode");
		configParam(OFFSET_PARAM, 0.f, 10.f, 0.f, "offset");
		configParam(SCALE_PARAM, 0.f, 10.f, 10.f, "scale");
		configSwitch(TENX_PARAM, 0.f, 1.f, 0.f, "10X");
		configParam(SLEW_PARAM, 0.f, 10000.f, 0.f, "slew");

		configInput(SEED_CV_INPUT, "seed");
		configInput(SCALE_CV_INPUT, "scale");
		configInput(MODE_CV_INPUT, "mode");

		configInput(OFFSET_CV_INPUT, "offset");
		configInput(SLEW_CV_INPUT, "slew");
		configInput(TENX_CV_INPUT, "10X");

		configInput(RESEED_INPUT, "reseed");
		configInput(GEN_SEED_INPUT, "gen seed");
		configInput(CLOCK_INPUT, "clock");
		configOutput(RAND_OUTPUT, "CV");
	}

	void process(const ProcessArgs& args) override
	{
		/////////////////////////////
		// set current output mode //
		/////////////////////////////

		if(inputs[MODE_CV_INPUT].isConnected()) // check mode CV input
		{
			params[MODE_PARAM].setValue(inputs[MODE_CV_INPUT].getVoltage() / 2.f);
		}

		else
		{
			if(params[MODE_SWITCH].getValue() > 0.f && mode_flag == false) // if mode button pressed
			{
				params[MODE_PARAM].setValue(params[MODE_PARAM].getValue() + 1); // step to next mode
				params[MODE_PARAM].setValue( (int)params[MODE_PARAM].getValue() % 5 ); // when last mode reached, wrap around
				mode_flag = true; // use flag so value is set only once per button press
			}

			if(params[MODE_SWITCH].getValue() == 0.f) // if mode button is relaeased
			{
				mode_flag = false; // reset flag
			}
		}

		//////////////////
		// set mode LED //
		//////////////////

		mode = params[MODE_PARAM].getValue(); // set mode variable

		if(mode == 0) // cyan
		{
			lights[MODE_LIGHT + 0].setBrightness(0.f);
			lights[MODE_LIGHT + 1].setBrightness(1.f);
			lights[MODE_LIGHT + 2].setBrightness(1.f);
		}

		else if(mode == 1) // magenta
		{
			lights[MODE_LIGHT + 0].setBrightness(1.f);
			lights[MODE_LIGHT + 1].setBrightness(0.f);
			lights[MODE_LIGHT + 2].setBrightness(1.f);
		}

		else if(mode == 2) // yellow
		{
			lights[MODE_LIGHT + 0].setBrightness(1.f);
			lights[MODE_LIGHT + 1].setBrightness(1.f);
			lights[MODE_LIGHT + 2].setBrightness(0.f);				
		}

		else if(mode == 3) // green
		{
			lights[MODE_LIGHT + 0].setBrightness(0.f);
			lights[MODE_LIGHT + 1].setBrightness(1.f);
			lights[MODE_LIGHT + 2].setBrightness(0.f);				
		}

		else // white
		{
			lights[MODE_LIGHT + 0].setBrightness(1.f);
			lights[MODE_LIGHT + 1].setBrightness(1.f);
			lights[MODE_LIGHT + 2].setBrightness(1.f);				
		}

		//////////////////////////////////
		// set slew rate multiplier LED //
		//////////////////////////////////

		if(inputs[TENX_CV_INPUT].isConnected())
		{
			tenx = inputs[TENX_CV_INPUT].getVoltage() > 0.f;
			lights[TENX_LIGHT + 1].setBrightness(tenx);
			lights[TENX_LIGHT + 2].setBrightness(tenx);
		}

		else
		{
			tenx = params[TENX_PARAM].getValue() > 0.f;
			lights[TENX_LIGHT + 1].setBrightness(tenx);
			lights[TENX_LIGHT + 2].setBrightness(tenx);
		}

		////////////////////////////////////////////////
		// only run when the clock input is connected //
		////////////////////////////////////////////////

		if(inputs[CLOCK_INPUT].isConnected())
		{
			//////////////////////////////
			// check and read CV inputs //
			//////////////////////////////

			if(inputs[SEED_CV_INPUT].isConnected()) { seed_cv = inputs[SEED_CV_INPUT].getVoltage() / 10.f; }
			else { seed_cv = 1.f; }

			if(inputs[SCALE_CV_INPUT].isConnected()) { scale_cv = inputs[SCALE_CV_INPUT].getVoltage() / 10.f; }
			else { scale_cv = 1.f; }

			if(inputs[OFFSET_CV_INPUT].isConnected()) { offset_cv = inputs[OFFSET_CV_INPUT].getVoltage() / 10.f; }
			else { offset_cv = 1.f; }

			if(inputs[SLEW_CV_INPUT].isConnected()) { slew_cv = inputs[SLEW_CV_INPUT].getVoltage() / 10.f; }
			else { slew_cv = 1.f; }		

			if(inputs[GEN_SEED_INPUT].isConnected())
			{
				gen_seed = seedTrigger.process(inputs[GEN_SEED_INPUT].getVoltage(), 0.1f, 2.f);
				
				if(gen_seed && seed_flag == false)
				{
					params[SEED_PARAM].setValue( rand() ); // cheap means of choosing a random seed for the "proper" pRNG
					seed_flag = true;
				}

				if(!gen_seed)
				{
					seed_flag = false;
				}
			}

			if(inputs[RESEED_INPUT].isConnected())
			{
				reseed = reseedTrigger.process(inputs[RESEED_INPUT].getVoltage(), 0.1f, 2.f);
				
				if(reseed && reseed_flag == false)
				{
					float seed = params[SEED_PARAM].getValue() * seed_cv;
					generator.seed(seed);
					reseed_flag = true;
				}

				if(!reseed)
				{
					reseed_flag = false;
				}
			}

			///////////////////////////
			// check for clock pulse //
			///////////////////////////

			clock = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f);

			/////////////////////////
			// if clock edge rises //
			/////////////////////////

			if(clock && clock_flag == false)
			{
				offset = params[OFFSET_PARAM].getValue() * offset_cv;
				scale = params[SCALE_PARAM].getValue() * scale_cv;

				float A = distribution(generator);
				float B = distribution(generator);

				if(mode == 0) // uniform
				{
					target = (float) A * scale + offset;
				}

				else if(mode == 1) // inverse linear
				{
					if(A < B) { target = (float) A * scale + offset; }
					else { target = (float) B * scale + offset; }
				}
				
				else if(mode == 2) // linear
				{
					if(A > B) { target = (float) A * scale + offset; }
					else { target = (float) B * scale + offset; }
				}

				else if(mode == 3) // triangle
				{
					float average = (float) ( A + B ) / 2.f;
					target = (float) average * scale + offset;
				}

				else // inverse triangle
				{
					float average = (float) ( A + B ) / 2.f;

					if(average > 0.5f)
					{
						average = (float) average - 0.5f;
					}

					else
					{
						average = (float) average + 0.5f;
					}

					target = (float) average * scale + offset;
				}
				
				clock_flag = true; // use flag so value is set only once per clock pulse
			}

			/////////////////////////
			// if clock edge falls //
			/////////////////////////

			if(!clock)
			{
				clock_flag = false; // reset clock flag
			}

			/////////////////////////////
			// set main output voltage //
			/////////////////////////////

			slew = params[SLEW_PARAM].getValue() * slew_cv; // get current slew rate

			if(slew < 10.f) // if the slew rate is tiny, don't interpolate
			{
				current = target;
			}

			else // interpolate!
			{
				if(tenx){ slew = slew * 10.f; } // check for slew rate multiplier

				if(current < target) // ramp up to target value
				{
					current += (1.f/slew);
				}

				if(current > target) // ramp down to target value
				{
					current -= (1.f/slew);
				}
			}

			current = clamp(current,0.f,10.f); // clamp values to keep them in range

			outputs[RAND_OUTPUT].setVoltage(current); // set output to current value
		}
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

struct DatawaveWidget : ModuleWidget
{
	
	DatawaveWidget(Datawave* module)
	{
		float vertical_spacing = 8.5f;
		float vertical_offset = 10.f;
		float horizontal_spacing = 5.08f;

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/datawave.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing + vertical_offset)), module, Datawave::SEED_CV_INPUT));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 2.f + vertical_offset)), module, Datawave::SEED_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 3.f + vertical_offset)), module, Datawave::RESEED_INPUT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 3.f + vertical_offset)), module, Datawave::GEN_SEED_INPUT));

		addParam(createParamCentered<TL1105>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 4.5f + vertical_offset)), module, Datawave::MODE_SWITCH));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 4.5f + vertical_offset)), module, Datawave::MODE_CV_INPUT));
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<RedGreenBlueLight>>>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 4.5f + vertical_offset)), module, Datawave::MODE_SWITCH, Datawave::MODE_LIGHT));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 6.f + vertical_offset)), module, Datawave::OFFSET_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 6.f + vertical_offset)), module, Datawave::OFFSET_CV_INPUT));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 7.f + vertical_offset)), module, Datawave::SCALE_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 7.f + vertical_offset)), module, Datawave::SCALE_CV_INPUT));

		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 8.5f + vertical_offset)), module, Datawave::SLEW_CV_INPUT));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 8.5f + vertical_offset)), module, Datawave::SLEW_PARAM));

		addParam(createParamCentered<TL1105>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 9.5f + vertical_offset)), module, Datawave::TENX_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedGreenBlueLight>>>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 9.5f + vertical_offset)), module, Datawave::TENX_PARAM, Datawave::TENX_LIGHT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 9.5f + vertical_offset)), module, Datawave::TENX_CV_INPUT));

		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 11.f + vertical_offset)), module, Datawave::CLOCK_INPUT));
		addOutput(createOutputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 12.5f + vertical_offset)), module, Datawave::RAND_OUTPUT));
	}
};

Model* modelDatawave = createModel<Datawave, DatawaveWidget>("datawave");