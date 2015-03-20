//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File alsa.hpp
// Date 02.04.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************
//***************************************************************************
// The alsa stuff bases on code from Emmanuel Béranger
//***************************************************************************

#ifndef _ALSA_H_
#define _ALSA_H_

#include <alsa/asoundlib.h>

#include <QSound>
#include <QThread>

//***************************************************************************
// QAlsaSound
//***************************************************************************

class QAlsaSound : public QThread
{
   public:

      struct Device
      {
         QString name;
         QString desc;
      };

      struct WaveFormat
      {	
         u_int32_t dwSize;
         u_int16_t wFormatTag;
         u_int16_t wChannels;
         u_int32_t dwSamplesPerSec;
         u_int32_t dwAvgBytesPerSec;
         u_int16_t wBlockAlign;
         u_int16_t wBitsPerSample;
      };
      
      QAlsaSound(const QString& filename, QObject* aParent = 0);
      virtual ~QAlsaSound();
      
      // getter

      bool isInitialized()   { return initialized; }
      bool isFinished()      { return finished; }
      QString deviceName()   { return devicename; }
      QString fileName()     { return filename; }
      int loops()            { return playLoops; }
      int loopsRemaining()   { return remainingLoops; }

      // setter

      void setLoops(int num) { playLoops = num; }

      // interface

      void play();
      void stop();

      // static interface

      static const Device* getDevice(int index);
      static bool isAvailable();
      static void play(const QString& aFilename);
      static void cleanup();         // cleanup finished static instances
      static QString getDeviceName() { return devicename; }
      static void setDeviceName(QString device) 
      { devicename = device; available = -1; }

   private:

      // functions

      bool init(const QString& aFilename);
      void playSound();
      void run();
      char* findchunk(char* pstart, const char* fourcc, size_t n);

      // data

      snd_pcm_t* handle;
      snd_pcm_sframes_t frames;
      char* device;
      snd_pcm_uframes_t chunk_size, buffer_size;
      size_t bits_per_sample, bits_per_frame;
      
      int fd;
      u_long datastart;
      QObject* parent;
      QString filename;
      bool finished;
      bool initialized;
      bool running;
      int playLoops;
      int remainingLoops;

      // static stuff

      static QString devicename;
      static int fillDeviceList();
      static QList<Device*> devices;
      static int available;
      static QList<QAlsaSound*> sounds;  // hold active static instances
};

//***************************************************************************
#endif
