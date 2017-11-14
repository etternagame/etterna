#include "global.h"

#include "RageUtil.h"
#include "RageSoundReader_Vorbisfile.h"
#include "RageLog.h"

#if defined(INTEGER_VORBIS)
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

#include <cstring>
#include <cerrno>
#include "RageFile.h"
static size_t OggRageFile_read_func( void *ptr, size_t size, size_t nmemb, void *datasource )
{
	auto *f = (RageFileBasic *) datasource;
	return f->Read( ptr, size, nmemb );
}

static int OggRageFile_seek_func( void *datasource, ogg_int64_t offset, int whence )
{
	auto *f = (RageFileBasic *) datasource;
	return f->Seek( (int) offset, whence );
}

static int OggRageFile_close_func( void *datasource )
{
	return 0;
}

static long OggRageFile_tell_func( void *datasource )
{
	auto *f = (RageFileBasic *) datasource;
	return f->Tell();
}

static RString ov_ssprintf( int err, const char *fmt, ...)
{
	va_list	va;
	va_start( va, fmt );
	RString s = vssprintf( fmt, va );
	va_end( va );

	RString errstr;
	switch( err )
	{
		// OV_FALSE, OV_EOF, and OV_HOLE were added to this switch because
		// OV_EOF cases were being reported as unknown. -Kyz
	case OV_FALSE: errstr = "OV_FALSE"; break;
	case OV_EOF: errstr = "OV_EOF"; break;
	case OV_HOLE: errstr = "OV_HOLE"; break;
	/* XXX: In the case of OV_EREAD, can we snoop at errno? */
	case OV_EREAD:		errstr = "Read error"; break;
	case OV_EFAULT:		errstr = "Internal error"; break;
	case OV_EIMPL:		errstr = "Feature not implemented"; break;
	case OV_EINVAL:		errstr = "Invalid argument"; break;
	case OV_ENOTVORBIS:	errstr = "Not Vorbis data"; break;
	case OV_EBADHEADER:	errstr = "Invalid Vorbis bitstream header"; break;
	case OV_EVERSION:	errstr = "Vorbis version mismatch"; break;
	case OV_ENOTAUDIO:	errstr = "OV_ENOTAUDIO"; break;
	case OV_EBADPACKET:	errstr = "OV_EBADPACKET"; break;
	case OV_EBADLINK:	errstr = "Link corrupted"; break;
	case OV_ENOSEEK:	errstr = "Stream is not seekable"; break;
	default:		errstr = ssprintf( "unknown error %i", err ); break;
	}

	return s + ssprintf( " (%s)", errstr.c_str() );
}

RageSoundReader_FileReader::OpenResult RageSoundReader_Vorbisfile::Open( RageFileBasic *pFile )
{
	m_pFile = pFile;
	vf = new OggVorbis_File;
	memset( vf, 0, sizeof(*vf) );

	ov_callbacks callbacks;
	callbacks.read_func  = OggRageFile_read_func;
	callbacks.seek_func  = OggRageFile_seek_func;
	callbacks.close_func = OggRageFile_close_func;
	callbacks.tell_func  = OggRageFile_tell_func;

	int ret = ov_open_callbacks( pFile, vf, NULL, 0, callbacks );
	if( ret < 0 )
	{
		SetError( ov_ssprintf(ret, "ov_open failed") );
		delete vf;
		vf = NULL;
		switch( ret )
		{
		case OV_ENOTVORBIS:
			return OPEN_UNKNOWN_FILE_FORMAT;
		default:
			return OPEN_FATAL_ERROR;
		}
	}

	eof = false;
	read_offset = (int) ov_pcm_tell(vf);

	vorbis_info *vi = ov_info( vf, -1 );
	channels = vi->channels;

	return OPEN_OK;
}

int RageSoundReader_Vorbisfile::GetLength() const
{
#if defined(INTEGER_VORBIS)
	int len = ov_time_total(vf, -1);
#else
	int len = int(ov_time_total(vf, -1) * 1000);
#endif
	if( len == OV_EINVAL )
		RageException::Throw( "RageSoundReader_Vorbisfile::GetLength: ov_time_total returned OV_EINVAL." );

	return len; 
}

int RageSoundReader_Vorbisfile::SetPosition( int iFrame )
{
	eof = false;

	const ogg_int64_t sample = ogg_int64_t(iFrame);

	int ret = ov_pcm_seek( vf, sample );
	if(ret < 0)
	{
		/* Returns OV_EINVAL on EOF. */
		if( ret == OV_EINVAL || ret == OV_EOF)
		{
			eof = true;
			return 0;
		}
		SetError( ov_ssprintf(ret, "ogg: SetPosition failed") );
		return -1;
	}
	read_offset = (int) ov_pcm_tell(vf);

	return 1;
}

int RageSoundReader_Vorbisfile::Read( float *buf, int iFrames )
{
	int frames_read = 0;

	while( iFrames && !eof )
	{
		const int bytes_per_frame = sizeof(float)*channels;

		int iFramesRead = 0;

		{
			int curofs = (int) ov_pcm_tell(vf);
			if( curofs < read_offset )
			{
				/* The timestamps moved backwards.  Ignore it.  This file probably
				 * won't sync correctly. */
				LOG->Trace( "p ahead %p %i < %i, we're ahead by %i", 
					this, curofs, read_offset, read_offset-curofs );
				read_offset = curofs;
			}
			else if( curofs > read_offset )
			{
				/* Our offset doesn't match.  We have a hole in the data, or corruption.
				 * If we're reading with accurate syncing, insert silence to line it up.
				 * That way, corruptions in the file won't casue desyncs. */

				/* In bytes: */
				int iSilentFrames = curofs - read_offset;
				iSilentFrames = min( iSilentFrames, (int) iFrames );
				int silence = iSilentFrames * bytes_per_frame;
				CHECKPOINT_M( ssprintf("p %i,%i: %i frames of silence needed", curofs, read_offset, silence) );

				memset( buf, 0, silence );
				iFramesRead = iSilentFrames;
			}
		}

		if( iFramesRead == 0 )
		{
			int bstream;
#if defined(INTEGER_VORBIS)
			int ret = ov_read( vf, (char *) buf, iFrames * channels * sizeof(int16_t), &bstream );
#else // float vorbis decoder
			float **pcm;
			int ret = ov_read_float( vf, &pcm, iFrames, &bstream );
#endif

			{
				vorbis_info *vi = ov_info( vf, -1 );
				ASSERT( vi != NULL );

				if( (unsigned) vi->channels != channels )
					RageException::Throw( "File \"%s\" changes channel count from %i to %i; not supported.",
							      filename.c_str(), channels, (int)vi->channels );
			}


			if( ret == OV_HOLE )
				continue;
			if( ret == OV_EBADLINK )
			{
				SetError( ssprintf("Read: OV_EBADLINK") );
				return ERROR;
			}

			if( ret == 0 )
			{
				eof = true;
				continue;
			}

#if defined(INTEGER_VORBIS)
			if( ret > 0 )
			{
				int iSamplesRead = ret / sizeof(int16_t);
				iFramesRead = iSamplesRead / channels;

				/* Convert in reverse, so we can do it in-place. */
				const int16_t *pIn = (int16_t *) buf;
				float *pOut = (float *) buf;
				for( int i = iSamplesRead-1; i >= 0; --i )
					pOut[i] = pIn[i] / 32768.0f;
			}
#else
			if( ret > 0 )
			{
				iFramesRead = ret;

				int iNumChannels = channels;
				for( int iChannel = 0; iChannel < iNumChannels; ++iChannel )
				{
					const float *pChannelIn = pcm[iChannel];
					float *pChannelOut = &buf[iChannel];
					for( int i = 0; i < iFramesRead; ++i )
					{
						*pChannelOut = *pChannelIn;
						++pChannelIn;
						pChannelOut += iNumChannels;
					}
				}
			}
#endif
		}

		read_offset += iFramesRead;

		buf += iFramesRead * channels;
		frames_read += iFramesRead;
		iFrames -= iFramesRead;
	}

	if( !frames_read )
		return END_OF_FILE;

	return frames_read;
}

int RageSoundReader_Vorbisfile::GetSampleRate() const
{
	ASSERT(vf != NULL);

	vorbis_info *vi = ov_info(vf, -1);
	ASSERT(vi != NULL);

	return vi->rate;
}

int RageSoundReader_Vorbisfile::GetNextSourceFrame() const
{
	ASSERT(vf != NULL);

	int iFrame = (int)ov_pcm_tell( vf );
	return iFrame;
}

RageSoundReader_Vorbisfile::RageSoundReader_Vorbisfile()
{
	vf = NULL;
}

RageSoundReader_Vorbisfile::~RageSoundReader_Vorbisfile()
{
	if(vf)
		ov_clear(vf);
	delete vf;
}

RageSoundReader_Vorbisfile *RageSoundReader_Vorbisfile::Copy() const
{
	RageFileBasic *pFile = m_pFile->Copy();
	pFile->Seek(0);
	auto *ret = new RageSoundReader_Vorbisfile;

	/* If we were able to open the sound in the first place, we expect to
	 * be able to reopen it. */
	if( ret->Open(pFile) != OPEN_OK )
		FAIL_M( ssprintf("Copying sound failed: %s", ret->GetError().c_str()) );

	return ret;
}

/*
 * Copyright (c) 2003 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

