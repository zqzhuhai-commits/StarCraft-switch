#include <memory>
#include "../openbw-switch/openbw/ui/native_sound.h"
#include "../openbw-switch/openbw/ui/common.h"
#include <SDL.h>
#include <cassert>
#include <vector>
#include <stdexcept>
namespace bwgame { namespace ui {
void log_str(a_string) {}
void fatal_error_str(a_string s) { throw std::runtime_error(std::string(s.data(),s.size())); }
}}
int main() {
    // A short valid 8-bit mono PCM WAV, converted into the native stereo mixer.
    std::vector<Uint8> wav(44+800,128);
    auto put=[&](int at, Uint32 value,int n) { for(int i=0;i<n;++i) wav[at+i]=Uint8(value>>(8*i)); };
    SDL_memcpy(wav.data(),"RIFF",4); put(4,wav.size()-8,4); SDL_memcpy(wav.data()+8,"WAVEfmt ",8);
    put(16,16,4); put(20,1,2); put(22,1,2); put(24,8000,4); put(28,8000,4); put(32,1,2); put(34,8,2);
    SDL_memcpy(wav.data()+36,"data",4); put(40,800,4);
    native_sound::frequency=44100;
    auto clip=native_sound::load_wav(wav.data(),wav.size()); assert(clip);
    native_sound::play(0,clip.get(),100,0); assert(native_sound::is_playing(0));
    native_sound::stop(0); assert(!native_sound::is_playing(0));
    native_sound::play(1,clip.get(),80,-64); assert(native_sound::is_playing(1));
    clip.reset(); assert(!native_sound::is_playing(1));
    assert(!native_sound::load_wav("invalid",7));
    SDL_Quit();
}
