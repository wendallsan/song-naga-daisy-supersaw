#include "daisy_seed.h"
#include "daisysp.h"
#define NUMBER_OF_SAWS 7
#define CURVE_POINTS 17
#define SAMPLE_RATE 48000
#define ROOT_FREQ 110.0
using namespace daisy;
using namespace daisysp;
DaisySeed hw;
Oscillator superSaws[ NUMBER_OF_SAWS ];
ATone hpf;
float detuneCurve[ CURVE_POINTS ] = {
	0.0,
	0.00967268,
	0.0220363,
	0.0339636,
	0.0467636,
	0.0591273,
	0.0714909,
	0.0838545,
	0.0967273,
	0.121527,
	0.147127,
	0.193455,
	0.243418,
	0.2933815,
	0.343345,
	0.3928, 
	1.0
}, ampCurve[ CURVE_POINTS ] = {
	0.03836,
	0.12,
	0.19,
	0.25,
	0.31,
	0.37,
	0.42,
	0.46,
	0.5,
	0.53,
	0.56,
	0.58,
	0.59,
	0.6,
	0.605,
	0.6,
	0.59
}, knobValue,
	detunes[ NUMBER_OF_SAWS ] = {
		-0.11002314,
		-0.0628844,
		-0.01952357,
		0.0,
		0.01991221,
		0.06216538,
		0.10745242
	},
	phases[ NUMBER_OF_SAWS ],
	mixAmounts[ NUMBER_OF_SAWS ] = { 0.4, 0.4, 0.4, 0.4, 0.5, 0.5, 0.45 };
int knobCurveIndex;
enum AdcChannel { waveformKnob, NUM_ADC_CHANNELS };
void AudioCallback( AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size ){
	for( size_t i = 0; i < size; i++ ){
		float superSawSignal = 0.0;
		for( int i = 0; i < NUMBER_OF_SAWS; i++ ) superSawSignal += ( superSaws[ i ].Process() );
		out[0][i] = out[1][i] = hpf.Process( superSawSignal );
	}
}
float lookupDetune( float knobAmount ){
	float minDetune = detuneCurve[ knobCurveIndex ];
	float maxDetune = detuneCurve[ knobCurveIndex + 1 ];
	float minCurveValue = detuneCurve[ knobCurveIndex ];
	float maxCurveValue = detuneCurve[ knobCurveIndex + 1 ];
	float output = minDetune + ( ( maxDetune - minDetune ) * ( ( knobAmount - minCurveValue ) / ( maxCurveValue / minCurveValue ) ) );
	return output;
}
float lookupAmp( float knobAmount ){
	float minAmp = ampCurve[ knobCurveIndex  ];
	float maxAmp = ampCurve[ knobCurveIndex + 1];
	float minCurveValue = ( float )knobCurveIndex / CURVE_POINTS;
	float maxCurveValue = ( ( float )knobCurveIndex + 1 ) / CURVE_POINTS;
	float output = minAmp + ( ( maxAmp - minAmp ) * ( ( knobAmount - minCurveValue ) / ( maxCurveValue / minCurveValue ) ) );
	return output;
}
int main(){
	hw.Init();
	hw.StartLog();
	for( int i = 0; i < NUMBER_OF_SAWS; i++ ){
		superSaws[ i ].Init( SAMPLE_RATE );
		superSaws[ i ].SetWaveform( superSaws[ i ].WAVE_POLYBLEP_SAW );
		float randy = rand() / RAND_MAX;
		phases[ i ] = randy;
		superSaws[ i ].PhaseAdd( randy );
	}
    hpf.Init( SAMPLE_RATE );
	float hpfFreq = ROOT_FREQ;
	hpf.SetFreq( hpfFreq );
	AdcChannelConfig adcConfig[ NUM_ADC_CHANNELS ];
    adcConfig[ waveformKnob ].InitSingle( daisy::seed::A0 );
	hw.adc.Init( adcConfig, NUM_ADC_CHANNELS );
    hw.adc.Start();
	hw.SetAudioBlockSize( 4 ); // number of samples handled per callback
	hw.SetAudioSampleRate( SaiHandle::Config::SampleRate::SAI_48KHZ );
	hw.StartAudio( AudioCallback );
	while( true ){
		knobValue = hw.adc.GetFloat( waveformKnob );
		knobCurveIndex = knobValue * CURVE_POINTS;
		for( int i = 0; i < NUMBER_OF_SAWS; i++ ){ // ADJUST FREQUENCIES
			superSaws[ i ].SetFreq( 
				ROOT_FREQ + 
				lookupDetune( knobValue ) * detunes[ i ] * ROOT_FREQ
			);
		}
		for( int i = 0; i < NUMBER_OF_SAWS; i++ ){ // ADJUST AMPLITUDES
			superSaws[ i ].SetAmp( 
				i == 3?
					1.0 - fmap( knobValue, 0.0, mixAmounts[ i ] ) :
					lookupAmp( knobValue ) * mixAmounts[ i ]
			);
		}
		System::Delay( 1 );
	}
}