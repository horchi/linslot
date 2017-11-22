//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File alsa.cc
// Date 02.04.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************
//***************************************************************************
// The alsa stuff bases on code from Emmanuel Béranger
//***************************************************************************

// #define ALSA_TEST

#include <alsa.hpp>
#include <QFile>

//***************************************************************************
// Statics
//***************************************************************************

QList<QAlsaSound::Device*> QAlsaSound::devices;
QList<QAlsaSound*> QAlsaSound::sounds;
int QAlsaSound::available = -1;
QString QAlsaSound::devicename = "default";

//***************************************************************************
// Object
//***************************************************************************

QAlsaSound::QAlsaSound(const QString& aFilename, QObject* aParent)
{
   parent = aParent;
   finished = true;

   filename = "";
   playLoops = 1;
   remainingLoops = 0;
   handle = 0;
   running = false;
   initialized = false;
   datastart = 0;

   initialized = init(aFilename);
}

QAlsaSound::~QAlsaSound()
{
   if (handle)
      snd_pcm_close(handle);

   if (fd)
      ::close(fd);
}

//***************************************************************************
// Tnitialise
//***************************************************************************

bool QAlsaSound::init(const QString& aFilename)
{
   const int maxHeader = 1024;

	char buffer[maxHeader];
   WaveFormat waveformat;
	char* ptr;
	u_long databytes;
	snd_pcm_format_t format;
	snd_pcm_hw_params_t* params;
   int err;
   // u_long samples;

   if (!QFile::exists(aFilename))
   {
      printf("File '%s' not exists, aborting playback\n", aFilename.toLatin1().constData());
      return false;
   }

	filename = aFilename;

	// Open the file for reading:

	if ((fd = open(filename.toLatin1().constData(), O_RDONLY)) < 0)
   {
		printf("Error Opening WAV file %s\n", filename.toLatin1().constData());
		return false;
	}

	if (lseek(fd, 0L, SEEK_SET) != 0L)
   {
		// Wav file must be seekable device

		printf("Error nRewinding WAV file %s\n", filename.toLatin1().constData());
		return false;
	}

	::read(fd, buffer, maxHeader);

	if (findchunk(buffer, "RIFF", maxHeader) != buffer)
   {
		printf("Bad format: Cannot find RIFF file marker\n");
		return false;
	}

	if (!findchunk(buffer, "WAVE", maxHeader))
   {
		printf("Bad format: Cannot find WAVE file marker\n");
		return  false ;
	}

	if (!(ptr = findchunk(buffer, "fmt ", maxHeader)))
   {
		printf("Bad format: Can't find 'fmt' file marker\n");
		return  false;
	}

   // Move past 'fmt '

	ptr += 4;
	memcpy(&waveformat, ptr, sizeof(WaveFormat));

	if (!(ptr = findchunk(buffer, "data", maxHeader)))
   {
		printf("Bad format: unable to find 'data' file marker\n");
		return false;
	}

	ptr += 4;              //  Move past "data"
	memcpy(&databytes, ptr, sizeof(u_long));

	// samples = databytes / waveformat.wBlockAlign ;
	datastart = ((u_long)(ptr + 4)) - ((u_long)(&(buffer[0])));

	switch (waveformat.wBitsPerSample)
	{
		case 8:  format = SND_PCM_FORMAT_U8;     break;
		case 16: format = SND_PCM_FORMAT_S16_LE; break;
		case 32: format = SND_PCM_FORMAT_S32_LE; break;

		default:
      {
         printf("Unsupported format (%d) bits per seconds!\n",
                waveformat.wBitsPerSample);
			return false;
      }
	}

   // ALSA pain

   // printf("opening alsa device '%s'\n", devicename.toAscii().constData());

	if ((err = snd_pcm_open(&handle, devicename.toLatin1(),
                           SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC)) < 0)
   {
      const char* device = devicename.toLatin1();

      printf("cannot open audio device %s (%s)\n",
             device, snd_strerror(err));

      return false;
   }

   // alloc parameter object and fit with default values

   snd_pcm_hw_params_alloca(&params);
   snd_pcm_hw_params_any(handle, params);

   // set parameters ..

   snd_pcm_nonblock(handle, true);
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
   snd_pcm_hw_params_set_format(handle, params, format);
	snd_pcm_hw_params_set_channels(handle, params, waveformat.wChannels);
	snd_pcm_hw_params_set_rate_near(handle, params, &waveformat.dwSamplesPerSec, 0);

   // apply parameters

	if (snd_pcm_hw_params(handle, params) < 0)
   {
		printf("Unable to install hw params\n");
		return false;
	}

	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
   buffer_size = chunk_size * waveformat.wChannels * 2;
	bits_per_sample = snd_pcm_format_physical_width(format);
	bits_per_frame = bits_per_sample * waveformat.wChannels;

	return true;
}

//***************************************************************************
// Play
//***************************************************************************

void QAlsaSound::playSound()
{
	int count, f;
	char* buffer;
   int written;

	if (!initialized)
		return;

	lseek(fd, datastart, SEEK_SET);
	buffer = (char*)malloc(buffer_size);

   // start playback

   while (running && (count = ::read(fd, buffer, buffer_size)))
	{
		f = count * 8 / bits_per_frame;
		written = 0;

		while (running && f > 0)
      {
			frames = snd_pcm_writei(handle, buffer + written, f);

         if (frames == -EPIPE)
         {
            printf("underrun occurred\n");
            snd_pcm_prepare(handle);
         }
         else if (frames == -EAGAIN)
         {
 				snd_pcm_wait(handle, 100);
         }

         else if (frames < 0)
         {
            printf("error from writei: %s\n", snd_strerror(frames));
            frames = snd_pcm_recover(handle, frames, 0);
         }
         else
         {
				f -= frames;
				written += frames * bits_per_frame / 8;
         }
		}
   }

   sleep(1);  // to avoid click at end of playback !
   free(buffer);
	snd_pcm_drain(handle);
}

//***************************************************************************
// Find Chunk
//***************************************************************************

char* QAlsaSound::findchunk(char* pstart, const char* fourcc, size_t n)
{
   char* pend;
	int k, test;

	pend = pstart + n;

	while (pstart < pend)
	{
      if (*pstart == *fourcc)       // found match for first char
		{
         test = true;

			for (k = 1 ; fourcc[k] != 0 ; k++)
				test = (test ? (pstart[k] == fourcc[k]) : false);

			if (test)
				return pstart;
      };

		pstart++;
   }

	return 0;
}

//***************************************************************************
// Static Play
//***************************************************************************

void QAlsaSound::play(const QString& aFilename)
{
   QAlsaSound* sound;

   // first cleanup old static instances

   QAlsaSound::cleanup();

   if (!QAlsaSound::isAvailable())
      return ;

   sound = new QAlsaSound(aFilename);

   // append to the list an play

   if (sound->isInitialized())
   {
      sounds.append(sound);
      sound->play();
   }
   else
      delete sound;
}

//***************************************************************************
// Cleanup
//***************************************************************************

void QAlsaSound::cleanup()
{
   QAlsaSound* sound;

   for (int i = 0; i < sounds.size(); ++i)
   {
      if (sounds.at(i)->isFinished())
      {
         sound = sounds.takeAt(i);
         delete sound;
         i--;
      }
   }
}

//***************************************************************************
// Is Available
//***************************************************************************

bool QAlsaSound::isAvailable()
{
   int err;
   snd_pcm_t* handle;

   if (available == -1)
   {
      available = true;

      if ((err = snd_pcm_open(&handle, devicename.toLatin1(),
                              SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC)) < 0)
      {
         available = false;

         printf("cannot open audio device %s (%s)\n",
                devicename.toLatin1().constData(), snd_strerror(err));
      }
      else
         snd_pcm_close(handle);
   }

   return available;
}

//***************************************************************************
// Thread Control
//***************************************************************************

void QAlsaSound::play()
{
   if (!QAlsaSound::isAvailable() || !finished)
      return ;

   finished = false;
   running = true;
   start();
}

void QAlsaSound::stop()
{
   if (!running)
      return ;

   running = false;

   if (!wait(200))
      printf("Sound thread not responding, terminated!\n");

   finished = true;
}

void QAlsaSound::run()
{
   remainingLoops = playLoops;

   while (running && remainingLoops)
   {
      playSound();
      remainingLoops--;
   }

   finished = true;
   running = false;
}

//***************************************************************************
// Get Device
//***************************************************************************

const QAlsaSound::Device* QAlsaSound::getDevice(int index)
{
   if (!devices.size())
      fillDeviceList();

   if (index >= devices.size())
      return 0;

   return devices.at(index);
}

//***************************************************************************
// Fill Device List
//***************************************************************************

int QAlsaSound::fillDeviceList()
{
   void** hints;
   void** p;
   char* name;
   char* desc;

   if (devices.size())
      return true;

   snd_config_update();

   if (snd_device_name_hint(-1, "pcm", &hints) < 0)
   {
      printf("Getting device hints failed\n");
      return false;
   }

   p = hints;

   while (*p)
   {
      Device* dev = new Device;
      devices.append(dev);

      name = snd_device_name_get_hint(*p, "NAME");
      desc = snd_device_name_get_hint(*p, "DESC");

      dev->name = QString(name);
      dev->desc = QString(desc);

      free(name);
      free(desc);

      p++;
   }

   snd_device_name_free_hint(hints);

   return true;
}

#ifdef ALSA_TEST

//***************************************************************************
// Main
//***************************************************************************

int main()
{

   int listDevices();

   QAlsaSound sound("test.wav");
   sound.play();

   // QAlsaSound::play("start.wav");

   while (!sound.isFinished())
   {
      printf("sound still running\n");
      usleep(500000);
   }

   return 0;
}
//***************************************************************************

#endif // ALSA_TEST
