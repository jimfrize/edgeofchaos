#include "plugin.hpp"
#include <dsp/digital.hpp>

///////////////////////////////////////////////////////////////
// global variables for syncing with external -=Syn=- module // 
///////////////////////////////////////////////////////////////

bool _SYNC = false;
int _BPM = 120;
bool _RUN = true;

struct Chronos : Module
{
	enum ParamId
	{
		RUN_PARAM,
		BPM_PARAM,
		RATE1_PARAM,
		OFF1_PARAM,
		RATE2_PARAM,
		OFF2_PARAM,
		RATE3_PARAM,
		OFF3_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		RUN_CV_INPUT,
		BPM_CV_INPUT,
		RATE1_CV_INPUT,
		OFF1_CV_INPUT,
		RATE2_CV_INPUT,
		OFF2_CV_INPUT,
		RATE3_CV_INPUT,
		OFF3_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		SUB1_OUTPUT,
		SUB2_OUTPUT,
		SUB3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		ENUMS(RUN_LIGHT, 3),
		LIGHTS_LEN
	};

	bool run = 0.f;
	bool tog = 0.f;
	bool reset_flag = true;

	rack::dsp::TTimer<float> TIMER; // main timer

	float freq = 0.f;
	const float mults[15] = { 1.f / 128.f, 1.f / 64.f, 1.f / 32.f, 1.f / 16.f, 1.f / 8.f, 1.f / 4.f, 1.f / 2.f, 1.f, 2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f };

	Chronos()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RUN_PARAM, 0.f, 1.f, 1.f, "run");
		configParam(BPM_PARAM, 0, 999, 120, "BPM");
		configParam(RATE1_PARAM, 0, 14, 7, "rate 1");
		configParam(RATE2_PARAM, 0, 14, 7, "rate 2");
		configParam(RATE3_PARAM, 0, 14, 7, "rate 3");
		configParam(OFF1_PARAM, 0.f, 1.f, 0.f, "offset 1");
		configParam(OFF2_PARAM, 0.f, 1.f, 0.f, "offset 2");
		configParam(OFF3_PARAM, 0.f, 1.f, 0.f, "offset 3");

		configInput(RUN_CV_INPUT, "run CV");
		configInput(BPM_CV_INPUT, "BPM CV");
		configInput(RATE1_CV_INPUT, "rate 1 CV");
		configInput(OFF1_CV_INPUT, "offset 1 CV");
		configInput(RATE2_CV_INPUT, "rate 2 CV");
		configInput(OFF2_CV_INPUT, "offset 2 CV");
		configInput(RATE3_CV_INPUT, "rate 3 CV");
		configInput(OFF3_CV_INPUT, "offset 3 CV");

		configOutput(SUB1_OUTPUT, "sub clock 1");
		configOutput(SUB2_OUTPUT, "sub clock 2");
		configOutput(SUB3_OUTPUT, "sub clock 3");
	}

	void process(const ProcessArgs& args) override
	{
			///////////////////////////
			// get and set run state //
			///////////////////////////

			if(inputs[RUN_CV_INPUT].isConnected()) // when using CV to control run state
			{
				bool run_cv = inputs[RUN_CV_INPUT].getVoltage() > 0.f; // get CV

				if(_SYNC) // if global sync mode is on
				{
					run = _RUN; // run clock internally according to run state of external -=Syn=- module
					tog = run_cv; // toggle output according to run CV
				}

				else // if not in global sync mode
				{
					// set run mode and toggle output according to run CV
					run = run_cv;
					tog = run_cv;
				}
			}

			else // when not using CV to control run state
			{
				if(_SYNC) // if global sync mode is on
				{
					run = _RUN; // run clock internally according to run state of external -=Syn=- module
					tog = params[RUN_PARAM].getValue() > 0.f; // toggle output according to state of run switch
				}

				else // if not in global sync mode
				{
					tog = params[RUN_PARAM].getValue() > 0.f; // toggle output according to state of run switch
					run = tog; // run clock internally according to state of run switch
				}
			}

			// toggle run switch LED
			lights[RUN_LIGHT + 0].setBrightness(tog);
			lights[RUN_LIGHT + 2].setBrightness(tog);

			////////////////////////////////////////////
			// get connected CV inputs and set params //
			////////////////////////////////////////////

			if(inputs[RATE1_CV_INPUT].isConnected())
			{
				int rate1 = std::round(inputs[RATE1_CV_INPUT].getVoltage() / 10.f * 14.f);
				params[RATE1_PARAM].setValue( clamp(rate1, 0, 14) );
			}

			if(inputs[RATE2_CV_INPUT].isConnected())
			{
				int rate2 = std::round(inputs[RATE2_CV_INPUT].getVoltage() / 10.f * 14.f);
				params[RATE2_PARAM].setValue( clamp(rate2, 0, 14) );
			}


			if(inputs[RATE3_CV_INPUT].isConnected())
			{
				int rate3 = std::round(inputs[RATE3_CV_INPUT].getVoltage() / 10.f * 14.f);
				params[RATE3_PARAM].setValue( clamp(rate3, 0, 14) );
			}

			if(inputs[OFF1_CV_INPUT].isConnected())
			{
				float off1 = inputs[OFF1_CV_INPUT].getVoltage() / 10.f;
				off1 = clamp(off1, 0.f, 1.f);
				params[OFF1_PARAM].setValue( off1 );
			}

			if(inputs[OFF2_CV_INPUT].isConnected())
			{
				float off2 = inputs[OFF2_CV_INPUT].getVoltage() / 10.f;
				off2 = clamp(off2, 0.f, 1.f);
				params[OFF2_PARAM].setValue( off2 );
			}

			if(inputs[OFF3_CV_INPUT].isConnected())
			{
				float off3 = inputs[OFF3_CV_INPUT].getVoltage() / 10.f;
				off3 = clamp(off3, 0.f, 1.f);
				params[OFF3_PARAM].setValue( off3 );
			}

			//////////////////////////
			// if run state is true //
			//////////////////////////

			if(run)
			{
				if(_SYNC) // if global sync mode is on
				{
					if(reset_flag) // do once when sync mode is first turned on
					{
						TIMER.reset();
						if(outputs[SUB1_OUTPUT].isConnected()) { outputs[SUB1_OUTPUT].setVoltage(0.f); }
						if(outputs[SUB2_OUTPUT].isConnected()) { outputs[SUB2_OUTPUT].setVoltage(0.f); }
						if(outputs[SUB2_OUTPUT].isConnected()) { outputs[SUB2_OUTPUT].setVoltage(0.f); }

						reset_flag = false;
					}

					freq = _BPM / 60.f; // set main clock freq according to BPM from external -=Syn=- module
					params[BPM_PARAM].setValue(_BPM); // set BPM knob
				}

				else // when global sync mode is off
				{
					reset_flag = true;

					int local_bpm = 120.f;

					if(inputs[BPM_CV_INPUT].isConnected()) // if using BPM CV
					{
						float bpm_cv = clamp((inputs[BPM_CV_INPUT].getVoltage() / 10.f), 0.f, 1.f); // get and clamp CV
						local_bpm = std::round( bpm_cv * 999.f ); // scale and round CV to BPM range
						params[BPM_PARAM].setValue(local_bpm); // set BPM knob
					}

					else // if not using BPM CV
					{
						local_bpm = std::round(params[BPM_PARAM].getValue()); // set BPM according to the BPM knob
					}

					freq = local_bpm / 60.f; // set main clock freq according to local BPM
				}

				///////////////////////
				// set clock outputs //
				///////////////////////
				
				float _time = TIMER.getTime();
				if(_time >= 128.f) { TIMER.reset(); } // reset timer to prevent wrap around / overflow, do this when the timer is >= the maximum time div

				if(tog) // if outputs are toggled on, set thier phase and voltage
				{
					if(outputs[SUB1_OUTPUT].isConnected())
					{
						float phase = std::fmod( (_time + params[OFF1_PARAM].getValue() ) * mults[ (int)params[RATE1_PARAM].getValue() ], 1.f);
						outputs[SUB1_OUTPUT].setVoltage(phase < 0.5f ? 10.f : 0.f);
					}

					if(outputs[SUB2_OUTPUT].isConnected())
					{
						float phase = std::fmod( (_time + params[OFF2_PARAM].getValue() ) * mults[ (int)params[RATE2_PARAM].getValue() ], 1.f);
						outputs[SUB2_OUTPUT].setVoltage(phase < 0.5f ? 10.f : 0.f);
					}

					if(outputs[SUB3_OUTPUT].isConnected())
					{
						float phase = std::fmod( (_time + params[OFF3_PARAM].getValue() ) * mults[ (int)params[RATE3_PARAM].getValue() ], 1.f);
						outputs[SUB3_OUTPUT].setVoltage(phase < 0.5f ? 10.f : 0.f);
					}
				}

				TIMER.process(freq * args.sampleTime); // accumulate!!!
				}

			///////////////////////////
			// if run state is false //
			///////////////////////////

			else
			{
				TIMER.reset();
				if(outputs[SUB1_OUTPUT].isConnected()) { outputs[SUB1_OUTPUT].setVoltage(0.f); }
				if(outputs[SUB2_OUTPUT].isConnected()) { outputs[SUB2_OUTPUT].setVoltage(0.f); }
				if(outputs[SUB2_OUTPUT].isConnected()) { outputs[SUB2_OUTPUT].setVoltage(0.f); }
			}
	}
};

struct Pot : app::SvgKnob
{
    widget::SvgWidget* bg;
 
    Pot()
	{
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;
 
        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);
 
        setSvg(Svg::load(asset::system("res/ComponentLibrary/Trimpot.svg")));
        bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Trimpot_bg.svg")));
		snap = true;
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

struct ChronosWidget : ModuleWidget
{
	ChronosWidget(Chronos* module)
	{
		float vertical_spacing = 8.5f;
		float vertical_offset = 10.f;
		float horizontal_spacing = 5.08f;

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/chronos.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
				
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 1.f + vertical_offset)), module, Chronos::RUN_CV_INPUT));
		addParam(createParamCentered<TL1105>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 1.f + vertical_offset)), module, Chronos::RUN_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedGreenBlueLight>>>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 1.f + vertical_offset)), module, Chronos::RUN_PARAM, Chronos::RUN_LIGHT));
		
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 2.f + vertical_offset)), module, Chronos::BPM_CV_INPUT));
		addParam(createParamCentered<Pot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 2.f + vertical_offset)), module, Chronos::BPM_PARAM));

		addParam(createParamCentered<Pot>(mm2px(Vec(horizontal_spacing, vertical_spacing * 3.5f + vertical_offset)), module, Chronos::RATE1_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 3.5f + vertical_offset)), module, Chronos::OFF1_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 4.5f + vertical_offset)), module, Chronos::RATE1_CV_INPUT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 4.5f + vertical_offset)), module, Chronos::OFF1_CV_INPUT));
		addOutput(createOutputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 5.5f + vertical_offset)), module, Chronos::SUB1_OUTPUT));

		addParam(createParamCentered<Pot>(mm2px(Vec(horizontal_spacing, vertical_spacing * 7.f + vertical_offset)), module, Chronos::RATE2_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 7.f + vertical_offset)), module, Chronos::OFF2_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 8.f + vertical_offset)), module, Chronos::RATE2_CV_INPUT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 8.f + vertical_offset)), module, Chronos::OFF2_CV_INPUT));
		addOutput(createOutputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 9.f + vertical_offset)), module, Chronos::SUB2_OUTPUT));

		addParam(createParamCentered<Pot>(mm2px(Vec(horizontal_spacing, vertical_spacing * 10.5f + vertical_offset)), module, Chronos::RATE3_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 10.5f + vertical_offset)), module, Chronos::OFF3_PARAM));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing, vertical_spacing * 11.5f + vertical_offset)), module, Chronos::RATE3_CV_INPUT));
		addInput(createInputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 3.f, vertical_spacing * 11.5f + vertical_offset)), module, Chronos::OFF3_CV_INPUT));
		addOutput(createOutputCentered<JACKPort>(mm2px(Vec(horizontal_spacing * 2.f, vertical_spacing * 12.5f + vertical_offset)), module, Chronos::SUB3_OUTPUT));
	}
};

Model* modelChronos = createModel<Chronos, ChronosWidget>("chronos");