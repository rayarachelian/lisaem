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

// need to make all these globals.
// device is top level contains listener, my_ALContext, sources.
// listener is the person listening
// openALContext #1 contains multiple sources
// Each source can have one or more buffers objects attached to it.
// Buffer objects are not part of a specific openALContext – they are shared among all openALContexts on one
// device.
// For every openALContext, there is automatically one Listener object


// get vars.h for ALERT_LOG - maybe split out ALERT_LOG?
#define __IN_LISAEM_WX__  1
extern "C"
{
 #include <vars.h>
 #include <stdio.h>
 #include <stdlib.h>
}
#undef __IN_LISAEM_WX__



#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/choicdlg.h>
#include <wx/cmdline.h>
#include <wx/utils.h> 
#include <wx/dnd.h>
#include <LisaConfig.h>
#include <LisaConfigFrame.h>
#include <LisaSkin.h>

#include <LisaEmSoundOpenAL.h>


#define MAX_WAV_SOUNDS 8


#define alCall(function, ...) alCallImpl(__FILE__, __LINE__, function, __VA_ARGS__)
#define alcCall(function, device, ...) alcCallImpl(__FILE__, __LINE__, function, device, __VA_ARGS__)




class SoundSys
{
public:
  SoundSys() { Init(); }
  ~SoundSys() { Close(); }

  void Init();
  void Close();

  void Play(const Wave* const wave );
  typedef std::string SoundExcept;
private:
  ALCdevice*  m_device;
  ALCcontext* m_ctx; 
};

void SoundSys::Init()
{
  m_ctx = 0;

  m_device = alcOpenDevice(NULL);
  if ( !m_device )
    throw SoundExcept("Couldn't open default OpenAL device.");

  m_ctx = alcCreateContext(m_device, NULL);
  if ( !m_ctx )
  {
    alcCloseDevice(m_device);
    throw SoundExcept("Could not create context.");
  }

}

void SoundSys::Play( const Wave* const wave )
{
  if ( !m_ctx || !m_device )
    throw SoundExcept("SoundSys not initialized");

  if ( !wave )
    throw SoundExcept("Wave not available for play");
    ...

*/ ////////////////////////////////////////////////////////////////////////////////////////////////////////

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

LisaEmSoundOpenAl *my_lisaem_openal = NULL;

extern float normalthrottle;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// from https://stackoverflow.com/questions/36949957/loading-a-wav-file-for-openal


/*
bool LisaEmSoundOpenAl::LisaEmOAL_load_wav_file_header(std::ifstream& file,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)

*/
bool LisaEmSoundOpenAl::loadWavFile(int soundid, char *filename, ALuint* buffer,
                                    ALsizei* size, ALsizei* frequency,
                                    ALenum* format) {
    //Local Declarations
    FILE*          soundFile = NULL;
    WAVE_Format    wave_format;
    RIFF_Header    riff_header;
    WAVE_Data      wave_data;
    uint8         *data;

    *size = wave_data.subChunk2Size;
    *frequency = wave_format.sampleRate;
    if (wave_format.numChannels             ==  1) {
        if (wave_format.bitsPerSample       ==  8)       *format = AL_FORMAT_MONO8;
        else if (wave_format.bitsPerSample  == 16)       *format = AL_FORMAT_MONO16;
    } else if (wave_format.numChannels      ==  2) {
        if (wave_format.bitsPerSample       ==  8)       *format = AL_FORMAT_STEREO8;
        else if (wave_format.bitsPerSample  == 16)       *format = AL_FORMAT_STEREO16;
    }

    try {
        soundFile = fopen(filename, "rb");
        if (!soundFile)
            throw (filename);

        // Read in the first chunk into the struct
        fread(&riff_header, sizeof(RIFF_Header), 1, soundFile);

        //check for RIFF and WAVE tag in memory
        if   ((riff_header.chunkID[0] != 'R'  ||
               riff_header.chunkID[1] != 'I'  ||
               riff_header.chunkID[2] != 'F'  ||
               riff_header.chunkID[3] != 'F'     ) ||
              (riff_header.format[0]  != 'W'  ||
               riff_header.format[1]  != 'A'  ||
               riff_header.format[2]  != 'V'  ||
               riff_header.format[3]  != 'E'    ))
            throw ("Invalid RIFF or WAVE Header");

        //Read in the 2nd chunk for the wave info
        fread(&wave_format, sizeof(WAVE_Format), 1, soundFile);
        //check for fmt tag in memory
        if (wave_format.subChunkID[0] != 'f' ||
            wave_format.subChunkID[1] != 'm' ||
            wave_format.subChunkID[2] != 't' ||
            wave_format.subChunkID[3] != ' ')
            throw ("Invalid Wave Format");

        //check for extra parameters;
        if (wave_format.subChunkSize > 16)
            fseek(soundFile, sizeof(short), SEEK_CUR);

        //Read in the the last byte of data before the sound file
        fread(&wave_data, sizeof(WAVE_Data), 1, soundFile);

        //check for data tag in memory
        if (wave_data.subChunkID[0] != 'd' ||
            wave_data.subChunkID[1] != 'a' ||
            wave_data.subChunkID[2] != 't' ||
            wave_data.subChunkID[3] != 'a'   )
            throw ("Invalid data header");

        //Allocate memory for data
        data = new unsigned char[wave_data.subChunk2Size];

        // Read in the sound data into the soundData variable
        if (!fread(data, wave_data.subChunk2Size, 1, soundFile))
            throw ("error loading WAVE data into struct!");

        //Now we set the variables that we passed in with the
        //data from the structs
        *size = wave_data.subChunk2Size;
        *frequency = wave_format.sampleRate;
        //The format is worked out by looking at the number of
        //channels and the bits per sample.
        if (wave_format.numChannels            ==  1) {
            if (wave_format.bitsPerSample      ==  8)      *format = AL_FORMAT_MONO8;
            else if (wave_format.bitsPerSample == 16)      *format = AL_FORMAT_MONO16;
        } else if (wave_format.numChannels     ==  2) {
            if (wave_format.bitsPerSample      ==  8)      *format = AL_FORMAT_STEREO8;
            else if (wave_format.bitsPerSample == 16)      *format = AL_FORMAT_STEREO16;
        }
        //create our openAL buffer and check for success
        alGenBuffers(2, buffer);
        if(alGetError() != AL_NO_ERROR) { std::cerr << alGetError() << std::endl; }
        //now we put our data into the openAL buffer and
        //check for success
        alBufferData(*buffer, *format, (void*)data, *size, *frequency);
        if(alGetError() != AL_NO_ERROR) { std::cerr << alGetError() << std::endl; }
        //clean up and return true if successful
        fclose(soundFile);
        delete data;
        return true;
    } catch(const char* error) {
        //our catch statement for if we throw a string
        std::cerr << error << " : trying to load "
        << filename << std::endl;
        //clean up memory if wave loading fails
        if (soundFile != NULL)
            fclose(soundFile);
        //return false to indicate the failure to load wave
        delete data;
        return false;
    }
}

int LisaEmSoundOpenAl::main() {
    ALuint buffer, source;
    ALint state;
    ALsizei size;
    ALsizei frequency;
    ALenum format;

    ALCcontext *context;
    ALCdevice *device;

    device = alcOpenDevice(nullptr);
    if (device == NULL)
    {
        cerr << "Error finding default Audio Output Device" << endl;
    }

    context = alcCreateContext(device,NULL);

    alcMakeContextCurrent(context);

    alGetError();

    loadWavFile("test.wav", &buffer, &size, &frequency, &format);

    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);

    // Play
    alSourcePlay(source);

    // Wait for the song to complete
    do {
        alGetSourcei(source, AL_SOURCE_STATE, &state);
    } while (state == AL_PLAYING);

    // Clean up sources and buffers
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




























// these are loaded the first time, unlike with wxSounds, so return true for now, but will report failures via ALERT_LOG
int LisaEmSoundOpenAl::LisaEmOAL_IsSoundLoaded(int soundid) {
    if (!device) return 0;
    if (!my_ALContext) return 0;
    if  (!rawSoundData[soundid]) return 0;
    return 1;
}

int LisaEmSoundOpenAl::LisaEmOAL_IsSoundPlaying(int soundid) {
    if (!device) return 0;
    if (!my_ALContext) return 0;
    if  (!rawSoundData[soundid]) return 0;

    ALint state = AL_PLAYING;
    alCall(alGetSourcei, openALWavsource[soundid], AL_SOURCE_STATE, &state);
    return (state == AL_PLAYING); 
}

void LisaEmSoundOpenAl::LisaEmOAL_StopSound(int soundid) {
    if (!device) return;
    if (!my_ALContext) return;
    if  (!rawSoundData[soundid]) return;
    alSourcePause(openALWavsource[soundid]);
}


// Inside LisaEmApp::OnInit:2970    
void  LisaEmSoundOpenAl::LisaEmOAL_InitSounds(wxString skindir, LisaSkin *skin) {
    //------ initialize globals -----------------------------------------------------------------------------------------------------------
    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device)
        return 0;

    //ALCopenALContext* my_ALContext;
    if  (!alcCall(alcCreateopenALContext, my_ALContext, device, device, nullptr) || !my_ALContext) 
        { ALERT_LOG(0,"ERROR: Could not create audio my_ALContext"); return;}

    ALCboolean openALContextMadeCurrent = false;
    if  (!alcCall(alcMakeopenALContextCurrent, openALContextMadeCurrent, device, my_ALContext)
        || openALContextMadeCurrent != ALC_TRUE) { ALERT_LOG(0,"ERROR: Could not make audio my_ALContext current"); return;}

    wxString sndfile[MAX_WAV_SOUNDS];
    char *sndfilec;
    sndfile[0]=skindir + skin->floppy_eject;            // 0
    sndfile[1]=skindir + skin->floppy_insert;           // 1
    sndfile[2]=skindir + skin->floppy_insert_nopower;   // 2
    sndfile[3]=skindir + skin->floppy_motor1;           // 3
    sndfile[4]=skindir + skin->floppy_motor2;           // 4
    sndfile[5]=skindir + skin->lisa_power_switch01;     // 5
    sndfile[6]=skindir + skin->lisa_power_switch02;     // 6
    sndfile[7]=skindir + skin->poweroffclk;             // 7

    for  (  int soundid=0; soundid<8; soundid++) {
            //------ load wav code ----------------------------------------------------------------------------------------------------------------
            std::uint8_t channels;
            std::int32_t sampleRate;
            std::uint8_t bitsPerSample;

            ALsizei dataSize;
            
            char filename[4096];
            sndfilec=CSTR(sndfile[soundid]);
            strncpy(filename,sndfilec,4096);

            if     (!rawSoundData[soundid])
                    {rawSoundData[soundid] = load_wav(filename, channels, sampleRate, bitsPerSample, dataSize);}
            if     ( rawSoundData[soundid] == NULL || dataSize == 0) {ALERT_LOG(0,"ERROR: Could not load wav"); return;}
            
            std::vector<char> soundData(rawSoundData[soundid], rawSoundData[soundid] + dataSize);
            
            alCall(alGenBuffers, 1, &wavbuffer[soundid]);
            
            ALenum format;
            if      (channels == 1 && bitsPerSample ==  8)  format = AL_FORMAT_MONO8;
            else if (channels == 1 && bitsPerSample == 16)  format = AL_FORMAT_MONO16;
            else if (channels == 2 && bitsPerSample ==  8)  format = AL_FORMAT_STEREO8;
            else if (channels == 2 && bitsPerSample == 16)  format = AL_FORMAT_STEREO16;
            else   { ALERT_LOG(0, "ERROR: unrecognised wave format: %d channels, %d bps", channels, bitsPerSample); return; }
            
            alCall(alBufferData, wavbuffer[soundid], format, soundData.data(), soundData.size(), sampleRate);
            soundData.clear(); // erase the sound in RAM
            //-----------------------------------------------------------------------------------------------------------------------------------------------------
            }
}


void LisaEmSoundOpenAl::LisaEmOAL_DestroySounds(void) {
    for  (  int soundid=0; soundid<8; soundid++) {
            alCall(alDeleteSources, 1, &openALWavsource[soundid]);
            alCall(alDeleteBuffers, 1, &wavbuffer[soundid]);
            }

    alcCall(alcMakeopenALContextCurrent, openALContextMadeCurrent, openALDevice, nullptr);
    alcCall(alcDestroyopenALContext, device, my_ALContext);
    ALCboolean closed;
    alcCall(alcCloseDevice, closed, device, device);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * OpenAL Sample - this should be good.
 * 2020-03-01 K.OHWADA
 */


// play tone
// reference : https://achapi2718.blogspot.com/2013/02/c-openal.html

using namespace std;

// global
const int TONE_SAMPLINGRATE = 44100;


/**
 * createTone
  * @param frequency : Hz
  * @param duration : sec
 * @return 16 bit 44.1 KHz sampling, tone_size
 */
int16_t* LisaEmSoundOpenAl::LisaEmOAL_createTone(int t2, int duration, int *tone_size, uint8_t SR)
{

    int bufsize = TONE_SAMPLINGRATE  * duration;
	int16_t* buff = new int16_t[bufsize];


// tones 0x60 (high pitch ROM), 0x20 (low pitch ROM), 0xa0 (click ROM)

    unsigned int tone=0x80;
    unsigned int volume=0x0f;
    unsigned int iorom=0;
    unsigned int vlow=32768, vhigh=32768;
    unsigned int cycles, eighthcycles, rate;
    long samples=0, i=0, r=0; 
    
    long data_size=bufsize *sizeof(int16_t);

    // 128 is our 0V reference (i.e. center), vhigh is the +V, vlow=-V, thus creating a square wave at the selected volume/amplitude
    vlow =32768-(volume<<10); // <<3
    vhigh=32768+(volume<<10); // <<3

    // *** this works from RAM now, but the pitch  is way too high, *4 is to compensate for that.
    cycles=t2/4;
    // if (!(my_lisaconfig->iorom & 0x20)) cycles=cycles*8/10;       // cycles *=0.8
    if (!(iorom & 0x20)) cycles=cycles*8/10;                      // cycles *=0.8
    cycles=(cycles>>1)+(cycles>>2);                               // cycles *=0.75; (1/2 + 1/4th)
    eighthcycles=cycles>>3;                                       // since we use SR's my_ALContext eightcycles /=8;

    if (eighthcycles<1) eighthcycles=1;                           // high frequency can set this to zero, need at least 1

    if (t2>0x90) samples=cycles;                                  // prevent click from turning into beep

//  fprintf(stderr,"low:%d high:%d\n",vlow,vhigh);

#ifdef DEBUGFREQ
//  int last=0, flip=0, period=0;
#endif
    rate=0; // rate=bit count rate, rate=max
    while (i<bufsize)
    {
        int val=(SR & (1<<(rate&7))  ) ? vhigh:vlow;
#ifdef DEBUGFREQ
      if (last!=val) {
           period=(i-flip)*2;
           flip=i; last=val;
        }
#endif
        buff[i++]=val;
        if ( (r++)>cycles) {rate++; r=0;}
    }

#ifdef DEBUGFREQ
    fprintf(stderr, "\n\nPeriod is: %d ; freq=%f \n\n",period,44100.0/period);
#endif

 *tone_size = bufsize;
	return buff;
}


void LisaEmSoundOpenAl::LisaEmOAL_sound_play(uint16 t2, uint8 SR, uint8 floppy_iorom) { // called from via6522.c, plays a Lisa sound
    // temporarily slow down CPU durring beeps so that they're fully played.
    normalthrottle=my_lisaframe->throttle; update_menu_checkmarks(); //updateThrottleMenus(5.0);

    // open default device
	if (!device) device=alcOpenDevice(NULL);
	if (!device) {cerr << "alcOpenDevice Faild" << endl;  return;} // EXIT_FAILURE;

	// version
	// ALCint major_version;
	// ALCint minor_version;
	// alcGetIntegerv(device, ALC_MAJOR_VERSION, 256, &major_version);
	// alcGetIntegerv(device, ALC_MINOR_VERSION, 256, &minor_version);
	// ALERT_LOG(0,"OpenAL Version: %d.%d \n", major_version, minor_version);

	if (!my_ALContext) my_ALContext = alcCreateopenALContext(device, NULL);
	if (!my_ALContext) {ALERT_LOG(0,"ALContext create Failed");} return; // EXIT_FAILURE;
	alcMakeopenALContextCurrent(my_ALContext);

    // generate Buffer and Source
	alGenBuffers(1, &lisabuffer);
	alGenSources(1, &openALsource);

    // generate tone
    int tone_size;
	int16_t* data = createTone(T2, duration, &tone_size,SR);

    // fills a buffer with audio data
	int data_size =  tone_size * sizeof(int16_t);
	alBufferData(lisabuffer, AL_FORMAT_MONO16, &data[0], data_size, TONE_SAMPLINGRATE);

    // play sound data in buffer
	alSourcei(openALsource, AL_BUFFER, lisabuffer);
	alSourcei(openALsource, AL_LOOPING, AL_TRUE); //enable looping
	ALERT_LOG(0,"Calling play");
	alSourcePlay(openALsource);
	ALERT_LOG(0,"returned from play");


//	alSourcePause(openALsource);
// Cleanup
//
//	alcDestroyopenALContext(my_ALContext);
//	alcCloseDevice(device);
//	delete[] data;
}


LisaEmSoundOpenAl::LisaEmOAL_sound_off(void)
{
    // restore throttle
    if (normalthrottle!=0)
    {my_lisaframe->throttle=normalthrottle; update_menu_checkmarks(); }// updateThrottleMenus(my_lisaframe->throttle);}

    if (cpu68k_clocks-my_lisaframe->lastclk<50000) return;  // prevent sound from shutting down immediately
    if (my_lisa_sound.IsPlaying() ) ALERT_LOG(0,"Stopping sound due to change of VIA1 ACR1");
    alSourcePause(openALsource);
	//alDeleteSources(1, &openALsource);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// WAV playback w/o libalure - from https://indiegamedev.net/2020/01/17/c-openal-function-call-wrapping/ and https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/ etc
///                                  https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/


bool LisaEmSoundOpenAl::LisaEmOAL_check_al_errors(const std::string& filename, const std::uint_fast32_t line)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        //std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n";
        switch (error) {
            case AL_INVALID_NAME:      ALERT_LOG(0,"%s:%d AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function",      filename,line); break;
            case AL_INVALID_ENUM:      ALERT_LOG(0,"%s:%d AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function",filename,line); break;
            case AL_INVALID_VALUE:     ALERT_LOG(0,"%s:%d AL_INVALID_VALUE: an invalid value was passed to an OpenAL function",    filename,line); break;
            case AL_INVALID_OPERATION: ALERT_LOG(0,"%s:%d AL_INVALID_OPERATION: the requested operation is not valid",             filename,line); break;
            case AL_OUT_OF_MEMORY:     ALERT_LOG(0,"%s:%d AL_OUT_OF_MEMORY: OpenAL ranout of memory",                              filename,line); break;
            default:                   ALERT_LOG(0,"%s:%d UNKNOWN AL ERROR: %s",filename, line, error); break;
        }
        return false;
    }
    return true;
}

bool LisaEmSoundOpenAl::LisaEmOAL_check_alc_errors(const std::string& filename, const std::uint_fast32_t line, ALCdevice* device)
{
    ALCenum error = alcGetError(device);
    if (error != ALC_NO_ERROR)
    {
        //std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n";
        switch (error)  {
            case ALC_INVALID_VALUE:         ALERT_LOG(0,"%s:%d ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function",           filename,line); break;
            case ALC_INVALID_DEVICE:        ALERT_LOG(0,"%s:%d ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function",              filename,line); break;
            case ALC_INVALID_openALContext: ALERT_LOG(0,"%s:%d ALC_INVALID_openALContext: a bad my_ALContext was passed to an OpenAL function",filename,line); break;
            case ALC_INVALID_ENUM:          ALERT_LOG(0,"%s:%d ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function",       filename,line); break;
            case ALC_OUT_OF_MEMORY:         ALERT_LOG(0,"%s:%d ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function",      filename,line); break;
            default:                        ALERT_LOG(0,"%s:%d UNKNOWN ALC ERROR: %s",filename,line,error); break;
        }
        //std::cerr << std::endl;
        return false;
    }
    return true;
}

std::int32_t convert_to_int(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    if constexpr (std::endian::native == std::endian::little)
    //if (std::endian::native == std::endian::little)
        std::memcpy(&a, buffer, len);
    else
        for (std::size_t i = 0; i < len; ++i)
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}
// loadWavFile("test.wav", &buffer, &size, &frequency, &format);


bool LisaEmSoundOpenAl::LisaEmOAL_load_wav_file_header(std::ifstream& file,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    char buffer[4];
    if (!file.is_open())
        return false;

    // the RIFF
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read RIFF" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "RIFF", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
        return false;
    }

    // the size of the file
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read size of file" << std::endl;
        return false;
    }

    // the WAVE
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read WAVE" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "WAVE", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
        return false;
    }

    // "fmt/0"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read fmt/0" << std::endl;
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read the 16" << std::endl;
        return false;
    }

    // PCM should be 1?
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read PCM" << std::endl;
        return false;
    }

    // the number of channels
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read number of channels" << std::endl;
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read sample rate" << std::endl;
        return false;
    }
    sampleRate = convert_to_int(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
        return false;
    }

    // ?? dafaq
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read dafaq" << std::endl;
        return false;
    }

    // bitsPerSample
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read bits per sample" << std::endl;
        return false;
    }
    bitsPerSample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data chunk header" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "data", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
        return false;
    }

    // size of data
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data size" << std::endl;
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if (file.eof())
    {
        std::cerr << "ERROR: reached EOF on the file" << std::endl;
        return false;
    }
    if (file.fail())
    {
        std::cerr << "ERROR: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

char* LisaEmSoundOpenAl::LisaEmOAL_load_wav(const std::string& filename,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
        return nullptr;
    }
    if (!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
    {
        std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
        return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}

void LisaEmSoundOpenAl::LisaEmOAL_PlaySound(int soundid, int async)    // This plays a .wav file, if we've already loaded it, it won't load it again, but just replays it
{
//------ initialize globals -----------------------------------------------------------------------------------------------------------
    /*ALCdevice* */
    device = alcOpenDevice(nullptr);
    if (!device) return 0;

    //ALCopenALContext* my_ALContext;
    if  (!alcCall(alcCreateopenALContext, my_ALContext, device, device, nullptr) || !my_ALContext) 
        { ALERT_LOG(0,"ERROR: Could not create audio my_ALContext"); return;}

    ALCboolean openALContextMadeCurrent = false;
    if  (!alcCall(alcMakeopenALContextCurrent, openALContextMadeCurrent, device, my_ALContext)
        || openALContextMadeCurrent != ALC_TRUE) { ALERT_LOG(0,"ERROR: Could not make audio my_ALContext current"); return;}

//------ load wav code ----------------------------------------------------------------------------------------------------------------
    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    ALsizei dataSize;

    if  (!rawSoundData[soundid])
         {rawSoundData[soundid] = load_wav("iamtheprotectorofthissystem.wav", channels, sampleRate, bitsPerSample, dataSize);}
    if  ( rawSoundData[soundid] == NULL || dataSize == 0) {ALERT_LOG(0,"ERROR: Could not load wav"); return;}

    std::vector<char> soundData(rawSoundData[soundid], rawSoundData[soundid] + dataSize);

    alCall(alGenBuffers, 1, &wavbuffer[soundid]);

    ALenum format;
    if      (channels == 1 && bitsPerSample ==  8)  format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16)  format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample ==  8)  format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16)  format = AL_FORMAT_STEREO16;
    else   { ALERT_LOG(0, "ERROR: unrecognised wave format: %d channels, %d bps", channels, bitsPerSample); return; }

    alCall(alBufferData, wavbuffer[soundid], format, soundData.data(), soundData.size(), sampleRate);
    soundData.clear(); // erase the sound in RAM
//-----------------------------------------------------------------------------------------------------------------------------------------------------

    //ALuint source;
    alCall(alGenSources, 1, &openALWavsource[soundid]);
    alCall(alSourcef,        openALWavsource[soundid], AL_PITCH, 1);
    alCall(alSourcef,        openALWavsource[soundid], AL_GAIN, 1.0f);
    alCall(alSource3f,       openALWavsource[soundid], AL_POSITION, 0, 0, 0);
    alCall(alSource3f,       openALWavsource[soundid], AL_VELOCITY, 0, 0, 0);
    alCall(alSourcei,        openALWavsource[soundid], AL_LOOPING, (async & 2) AL_TRUE:AL_FALSE);
    alCall(alSourcei,        openALWavsource[soundid], AL_BUFFER, wavbuffer[soundid]);
    alCall(alSourcePlay,     openALWavsource[soundid]);

    ALint state = AL_PLAYING;
    if (!(async & 1)) {while (state == AL_PLAYING) {  wxMilliSleep(20); alCall(alGetSourcei, openALWavsource[soundid], AL_SOURCE_STATE, &state);} }


//-------------- stop sound----------------------------------------------------------------------------------------------------------------------------
	//alSourcePause(openALWavsource[soundid]);

//-------------- delete sound and context and device --------------------------------------------------------------------------------------------------
    //alCall(alDeleteSources, 1, &openALWavsource[soundid]);
    //alCall(alDeleteBuffers, 1, &wavbuffer[soundid]);

    //alcCall(alcMakeopenALContextCurrent, openALContextMadeCurrent, openALDevice, nullptr);
    //alcCall(alcDestroyopenALContext, device, my_ALContext);

    //ALCboolean closed;
    //alcCall(alcCloseDevice, closed, device, device);
}
