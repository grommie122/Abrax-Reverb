#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

namespace
{
struct Control
{
    const char* id;
    const char* name;
    float minimum, maximum, initial;
};

constexpr std::array<Control, 5> controls {{
    {"size", "Size (%)", 0, 100, 82},
    {"damping", "Damping (%)", 0, 100, 42},
    {"width", "Width (%)", 0, 100, 100},
    {"mix", "Mix (%)", 0, 100, 25},
    {"freeze", "Freeze", 0, 1, 0}
}};

using State = juce::AudioProcessorValueTreeState;

State::ParameterLayout makeParameters()
{
    State::ParameterLayout layout;
    for (const auto& control : controls)
    {
        const juce::ParameterID id {control.id, 1};
        if (control.maximum == 1)
            layout.add(std::make_unique<juce::AudioParameterBool>(id, control.name, control.initial != 0));
        else
            layout.add(std::make_unique<juce::AudioParameterFloat>(id, control.name,
                juce::NormalisableRange<float>{control.minimum, control.maximum, 0.1f}, control.initial));
    }
    return layout;
}

class AbraxEditor final : public juce::AudioProcessorEditor
{
public:
    AbraxEditor(juce::AudioProcessor& processor, State& state) : AudioProcessorEditor(processor)
    {
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const auto& control = controls[knobIndices[i]];
            auto& knob = knobs[i];
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 23);
            knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffc5a9ff));
            knob.setName(control.name);
            addAndMakeVisible(knob);
            knobLinks[i] = std::make_unique<State::SliderAttachment>(state, control.id, knob);
        }
        for (size_t i = 0; i < switches.size(); ++i)
        {
            const auto& control = controls[4];
            switches[i].setButtonText(control.name);
            addAndMakeVisible(switches[i]);
            switchLinks[i] = std::make_unique<State::ButtonAttachment>(state, control.id, switches[i]);
        }
        setSize(580, 264);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff151225));
        g.setColour(juce::Colours::white);
        g.setFont(26.0f);
        g.drawText("Abrax Reverb", 20, 12, 580, 38, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff2a2142));
        g.fillRoundedRectangle(20, 60, 540, 184, 12);
        g.setColour(juce::Colours::white);
        g.setFont(17.0f);
        g.drawText("Hall", 34, 67, 120, 25, juce::Justification::centredLeft);
        g.setFont(13.0f);
        for (size_t i = 0; i < knobs.size(); ++i)
            g.drawText(controls[knobIndices[i]].name, knobX[i], 208, 106, 23, juce::Justification::centred);
    }

    void resized() override
    {
        for (size_t i = 0; i < knobs.size(); ++i)
            knobs[i].setBounds(knobX[i], 102, 106, 103);
        switches[0].setBounds(455, 67, 90, 25);
    }

private:
    static constexpr std::array<size_t, 4> knobIndices {0, 1, 2, 3};
    static constexpr std::array<int, 4> knobX {36, 169, 302, 435};
    std::array<juce::Slider, 4> knobs;
    std::array<juce::ToggleButton, 1> switches;
    std::array<std::unique_ptr<State::SliderAttachment>, 4> knobLinks;
    std::array<std::unique_ptr<State::ButtonAttachment>, 1> switchLinks;
};

class AbraxReverb final : public juce::AudioProcessor
{
public:
    AbraxReverb() : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        state(*this, nullptr, "AbraxReverb", makeParameters())
    {
        for (size_t i = 0; i < controls.size(); ++i)
            values[i] = state.getRawParameterValue(controls[i].id);
    }

    const juce::String getName() const override { return "Abrax Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return new AbraxEditor(*this, state); }
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto output = layout.getMainOutputChannelSet();
        return layout.getMainInputChannelSet() == output
            && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
    }

    void prepareToPlay(double sampleRate, int) override
    {
        reverb.setSampleRate(sampleRate);
        reverb.reset();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        const auto mix = values[3]->load() * 0.01f;
        juce::Reverb::Parameters parameters;
        parameters.roomSize = 0.65f + 0.35f * values[0]->load() * 0.01f;
        parameters.damping = values[1]->load() * 0.01f;
        parameters.width = values[2]->load() * 0.01f;
        parameters.wetLevel = mix / 3.0f;
        parameters.dryLevel = (1.0f - mix) / 2.0f;
        parameters.freezeMode = values[4]->load();
        reverb.setParameters(parameters);

        if (buffer.getNumChannels() == 2)
            reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
        else if (buffer.getNumChannels() == 1)
            reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
    }
    void getStateInformation(juce::MemoryBlock& destination) override
    {
        if (auto xml = state.copyState().createXml())
            copyXmlToBinary(*xml, destination);
    }

    void setStateInformation(const void* data, int bytes) override
    {
        if (auto xml = getXmlFromBinary(data, bytes))
            if (xml->hasTagName(state.state.getType()))
                state.replaceState(juce::ValueTree::fromXml(*xml));
    }

private:
    State state;
    std::array<std::atomic<float>*, 5> values {};
    juce::Reverb reverb;
};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AbraxReverb();
}



