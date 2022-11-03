/************************************************************************************************\
*                                                                                                *
*                     The Lisa Emulator Project  V1.2.7      RC5 2022.07.04                      *
*                                    http://lisaem.sunder.net                                    *
*                                                                                                *
*                          ::TODO:: incomple code, need to finish this.                          *
*                                                                                                *
*                     based on the following example code and a few others :                     *
*                         https://github.com/ohwada/MAC_cpp_Samples and                          *
* https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound *
*                                                                                                *
\************************************************************************************************/


#ifdef __linux__
#include <AL/al.h>
#include <AL/alc.h>
#endif

#ifdef __WXOSX__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include <string> 
#include <cstring>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <fstream>
#include <type_traits>
#include <vector>
#include <bit>



class LisaEmSoundOpenAl {
// globals
ALCdevice          *device = NULL;

ALCcontext* my_ALContext = NULL;
//ALCopenALContext   *my_ALContext = NULL;
ALuint              openALsource;        // Lisa sound playback from via6522
ALuint              openALWavsource[MAX_WAV_SOUNDS];  // individual WAV file sound effects (snd_* enum list below)
ALuint              lisabuffer;
ALuint              wavbuffer[MAX_WAV_SOUNDS];

public:
           int  LisaEmOAL_IsSoundPlaying(int soundid);
           int  LisaEmOAL_IsSoundLoaded(int soundid);
           void LisaEmOAL_StopSound(int soundid);
           void LisaEmOAL_PlaySound(int soundid);
           void LisaEmOAL_InitSounds(wxString skindir, LisaSkin skin);
           void LisaEmOAL_sound_play(uint16 t2, uint8 SR, uint8 floppy_iorom); // called from via6522.c
           void LisaEmOAL_sound_off(void);                                     // called from via6522.c
           char* rawSoundData[MAX_WAV_SOUNDS]; 

struct RIFF_Header {
    uint8  chunkID[4];
    uint32 chunkSize;  //uint32 // size not including chunkSize or chunkID
    uint8  format[4];
};


// Struct to hold fmt subchunk data for WAVE files.
struct WAVE_Format {
    uint8  subChunkID[4];  
    uint32 subChunkSize;  
    uint16 audioFormat;   
    uint16 numChannels;   
    uint32 sampleRate;    
    uint32 byteRate;      
    uint16 blockAlign;    
    uint16 bitsPerSample; 
};


// Struct to hold the data of the wave file
struct WAVE_Data {
    char subChunkID[4]; //should contain the word data
    long subChunk2Size; //Stores the size of the data block
};




/*
        template<typename alFunction, typename... Params>
        auto alCallImpl(const char* filename,
            const std::uint_fast32_t line,
            alFunction function,
            Params... params)
            ->typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, decltype(function(params...))>
        {
            auto ret = function(std::forward<Params>(params)...);
            check_al_errors(filename, line);
            return ret;
        }
        
        template<typename alFunction, typename... Params>
        auto alCallImpl(const char* filename,
            const std::uint_fast32_t line,
            alFunction function,
            Params... params)
            ->typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
        {
            function(std::forward<Params>(params)...);
            return check_al_errors(filename, line);
        }
        
        template<typename alcFunction, typename... Params>
        auto alcCallImpl(const char* filename,
            const std::uint_fast32_t line,
            alcFunction function,
            ALCdevice* device,
            Params... params)
            ->typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
        {
            function(std::forward<Params>(params)...);
            return check_alc_errors(filename, line, device);
        }
        
        template<typename alcFunction, typename ReturnType, typename... Params>
        auto alcCallImpl(const char* filename,
            const std::uint_fast32_t line,
            alcFunction function,
            ReturnType& returnValue,
            ALCdevice* device,
            Params... params)
            ->typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, bool>
        {
            returnValue = function(std::forward<Params>(params)...);
            return check_alc_errors(filename, line, device);
        }
*/


};
