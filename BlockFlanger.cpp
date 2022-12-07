#include "daisysp.h"
#include "BlockFlanger.h"
#include <math.h>

using namespace daisysp;

void BlockFlanger::Init(float sample_rate){
    sample_rate_ = sample_rate;

    SetFeedback(.2f);

    del_.Init();
    lfo_amp_ = 0.f;
    SetDelay(.75);

    lfo_phase_ = 0.f;
    SetLfoFreq(.3);
    SetLfoDepth(.9);
}

void BlockFlanger::Process( float *buf, size_t size ){
    for( size_t i = 0; i < size; i++ ){
        float lfo_sig = ProcessLfo();
        del_.SetDelay( 1.f + lfo_sig + delay_ );
        float out = del_.Read();
        del_.Write( buf[ i ] + out * feedback_ );
        buf[ i ] = ( buf[ i ] + out ) * 0.5f;
    }
}

void BlockFlanger::SetFeedback(float feedback){
    feedback_ = fclamp(feedback, 0.f, 1.f);
    feedback_ *= .97f;
}

void BlockFlanger::SetLfoDepth(float depth){
    depth    = fclamp(depth, 0.f, .93f);
    lfo_amp_ = depth * delay_;
}

void BlockFlanger::SetLfoFreq(float freq){
    freq = 4.f * freq / sample_rate_;
    freq *= lfo_freq_ < 0.f ? -1.f : 1.f;  //if we're headed down, keep going
    lfo_freq_ = fclamp(freq, -.25f, .25f); //clip at +/- .125 * sr
}

void BlockFlanger::SetDelay(float delay){
    delay = (.1f + delay * 6.9); //.1 to 7 ms
    SetDelayMs(delay);
}

void BlockFlanger::SetDelayMs(float ms){
    ms     = fmax(.1, ms);
    delay_ = ms * .001f * sample_rate_; //ms to samples

    lfo_amp_ = fmin(lfo_amp_, delay_); //clip this if needed
}

float BlockFlanger::ProcessLfo(){
    lfo_phase_ += lfo_freq_;

    //wrap around and flip direction
    if(lfo_phase_ > 1.f)
    {
        lfo_phase_ = 1.f - (lfo_phase_ - 1.f);
        lfo_freq_ *= -1.f;
    }
    else if(lfo_phase_ < -1.f)
    {
        lfo_phase_ = -1.f - (lfo_phase_ + 1.f);
        lfo_freq_ *= -1.f;
    }

    return lfo_phase_ * lfo_amp_;
}