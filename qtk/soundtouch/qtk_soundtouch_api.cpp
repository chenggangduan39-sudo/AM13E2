#include "qtk_soundtouch_api.h"

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "SoundTouch.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk_soundtouch_api.h"
#include "qtk_soundtouch_cfg.h"

using namespace soundtouch;
using namespace std;

typedef struct
{
    SoundTouch *pst;
}qtk_soundtouch_api_t;

static void qtk_soundtouch_api_init(qtk_soundtouch_api_t* api, qtk_soundtouch_cfg_t* cfg)
{
	api->pst->setSampleRate(cfg->sampleRate);
	api->pst->setChannels(cfg->channels);

	api->pst->setTempo(cfg->tempo);
	api->pst->setPitch(cfg->pitch);
	api->pst->setRate(cfg->rate);

	api->pst->setSetting(SETTING_USE_QUICKSEEK, cfg->quick);
	api->pst->setSetting(SETTING_USE_AA_FILTER, !(cfg->noAntiAlias));

    if (cfg->speech)
    {
        // use settings for speech processing
    	api->pst->setSetting(SETTING_SEQUENCE_MS, 40);
    	api->pst->setSetting(SETTING_SEEKWINDOW_MS, 15);
    	api->pst->setSetting(SETTING_OVERLAP_MS, 8);
//        fprintf(stderr, "Tune processing parameters for speech processing.\n");
    }
}
extern "C" void* qtk_soundtouch_api_new(void* c)
{
	qtk_soundtouch_api_t* api;
	qtk_soundtouch_cfg_t cfg;

	api = (qtk_soundtouch_api_t*)wtk_malloc(sizeof(*api));
	api->pst = new SoundTouch();
	if (api->pst == NULL)
	{
		qtk_soundtouch_api_delete(api);
		api=NULL;
		return api;
	}
	if (c)
	{
		qtk_soundtouch_api_init(api, (qtk_soundtouch_cfg_t*)c);
	}else
	{
		qtk_soundtouch_cfg_init(&cfg);
		qtk_soundtouch_api_init(api, &cfg);
	}

	return api;
}
extern "C" void qtk_soundtouch_api_feed(void* api, short* data, int len)
{
	((qtk_soundtouch_api_t*)api)->pst->putSamples(data, len);
}
extern "C" int qtk_soundtouch_api_recv(void* api, short* recvbuf, int size)
{
	((qtk_soundtouch_api_t*)api)->pst->receiveSamples(recvbuf, size);
}

extern "C" void qtk_soundtouch_api_flush(void*api)
{
	((qtk_soundtouch_api_t*)api)->pst->flush();
}

extern "C" void qtk_soundtouch_api_delete(void* api)
{
	if (((qtk_soundtouch_api_t*)api)->pst)
	{
		delete ((qtk_soundtouch_api_t*)api)->pst;
	}
	wtk_free((qtk_soundtouch_api_t*)api);
}

extern "C" void qtk_soundtouch_api_settemp(void* api, float tempoDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setTempo(tempoDelta);
}

extern "C" void qtk_soundtouch_api_setpitch(void* api, float pitchDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setPitch(pitchDelta);
}

extern "C" void qtk_soundtouch_api_setrate(void* api, float rateDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setRate(rateDelta);
}

extern "C" void qtk_soundtouch_api_setTempoChange(void* api, float tempoDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setTempoChange(tempoDelta);
}

extern "C" void qtk_soundtouch_api_setPitchSemiTones(void* api, float pitchDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setPitchSemiTones(pitchDelta);
}

extern "C" void qtk_soundtouch_api_setRateChange(void* api, float rateDelta)
{
	((qtk_soundtouch_api_t*)api)->pst->setRateChange(rateDelta);
}

////////////////raw test/////////////
#ifdef DEBUG
#include "RunParameters.h"
#include "WavFile.h"
#include "BPMDetect.h"
#if _WIN32
    #include <io.h>
    #include <fcntl.h>

    // Macro for Win32 standard input/output stream support: Sets a file stream into binary mode
    #define SET_STREAM_TO_BIN_MODE(f) (_setmode(_fileno(f), _O_BINARY))
#else
    // Not needed for GNU environment...
    #define SET_STREAM_TO_BIN_MODE(f) {}
#endif
static const char _helloText[] =
    "\n"
    "   SoundStretch v%s -  Copyright (c) Olli Parviainen 2001 - 2017\n"
    "==================================================================\n"
    "author e-mail: <oparviai"
    "@"
    "iki.fi> - WWW: http://www.surina.net/soundtouch\n"
    "\n"
    "This program is subject to (L)GPL license. Run \"soundstretch -license\" for\n"
    "more information.\n"
    "\n";
static void openFiles(WavInFile **inFile, WavOutFile **outFile, const RunParameters *params)
{
    int bits, samplerate, channels;

    if (strcmp(params->inFileName, "stdin") == 0)
    {
        // used 'stdin' as input file
        SET_STREAM_TO_BIN_MODE(stdin);
        *inFile = new WavInFile(stdin);
    }
    else
    {
        // open input file...
        *inFile = new WavInFile(params->inFileName);
    }

    // ... open output file with same sound parameters
    bits = (int)(*inFile)->getNumBits();
    samplerate = (int)(*inFile)->getSampleRate();
    channels = (int)(*inFile)->getNumChannels();

    if (params->outFileName)
    {
        if (strcmp(params->outFileName, "stdout") == 0)
        {
            SET_STREAM_TO_BIN_MODE(stdout);
            *outFile = new WavOutFile(stdout, samplerate, bits, channels);
        }
        else
        {
            *outFile = new WavOutFile(params->outFileName, samplerate, bits, channels);
        }
    }
    else
    {
        *outFile = NULL;
    }
}

int test_soundtouch(const int nParams, const char * const paramStr[])
{
    WavInFile *inFile;
    WavOutFile *outFile;
    RunParameters *params;
    SoundTouch soundTouch, *pSoundTouch;
    SAMPLETYPE sampleBuffer[BUFF_SIZE];
    int nSamples, nChannels;

    fprintf(stderr, _helloText, SoundTouch::getVersionString());

    pSoundTouch = &soundTouch;
    // Parse command line parameters
    params = new RunParameters(nParams, paramStr);

    // Open input & output files
    openFiles(&inFile, &outFile, params);

    // Setup the 'SoundTouch' object for processing the sound
    pSoundTouch->setSampleRate(16000);
    pSoundTouch->setChannels(1);

    pSoundTouch->setTempoChange(params->tempoDelta);
    pSoundTouch->setPitchSemiTones(params->pitchDelta);
    pSoundTouch->setRateChange(params->rateDelta);

    pSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);
    pSoundTouch->setSetting(SETTING_USE_AA_FILTER, !0);

    if (params->speech)
    {
        // use settings for speech processing
    	pSoundTouch->setSetting(SETTING_SEQUENCE_MS, 40);
    	pSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
    	pSoundTouch->setSetting(SETTING_OVERLAP_MS, 8);
        fprintf(stderr, "Tune processing parameters for speech processing.\n");
    }

    while (inFile->eof() == 0)
    {
        int num;

        // Read a chunk of samples from the input file
        num = inFile->read(sampleBuffer, BUFF_SIZE);
        nSamples = num / (int)inFile->getNumChannels();

        // Feed the samples into SoundTouch processor
        pSoundTouch->putSamples(sampleBuffer, nSamples);
        do
        {
            nSamples = pSoundTouch->receiveSamples(sampleBuffer, BUFF_SIZE);
            outFile->write(sampleBuffer, nSamples * nChannels);
        } while (nSamples != 0);
    }
//
    pSoundTouch->flush();
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, BUFF_SIZE);
        outFile->write(sampleBuffer, nSamples * nChannels);
    } while (nSamples != 0);

    // Close WAV file handles & dispose of the objects
    delete inFile;
    delete outFile;
    delete params;

    return 0;
}
#endif
