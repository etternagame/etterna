/* MAD is available from: http://www.underbit.com/products/mad/ */

#include "Etterna/Globals/global.h"
#include "RageSoundReader_MP3.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"

#include <cstdio>
#include <cerrno>
#include <map>

#include "mad.h"

// ID3 code from libid3:
enum tagtype
{
	TAGTYPE_NONE = 0,
	TAGTYPE_ID3V1,
	TAGTYPE_ID3V2,
	TAGTYPE_ID3V2_FOOTER
};

static const int ID3_TAG_FLAG_FOOTERPRESENT = 0x10;

static tagtype
tagtype(const unsigned char* data, id3_length_t length)
{
	if (length >= 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
		return TAGTYPE_ID3V1;

	if (length >= 10 &&
		((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
		 (data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
		data[3] < 0xff && data[4] < 0xff && data[6] < 0x80 && data[7] < 0x80 &&
		data[8] < 0x80 && data[9] < 0x80)
		return data[0] == 'I' ? TAGTYPE_ID3V2 : TAGTYPE_ID3V2_FOOTER;

	return TAGTYPE_NONE;
}

static unsigned long
id3_parse_uint(const unsigned char** ptr, unsigned int bytes)
{
	unsigned long value = 0;

	ASSERT(bytes >= 1 && bytes <= 4);

	switch (bytes) {
		case 4:
			value = (value << 8) | *(*ptr)++;
		case 3:
			value = (value << 8) | *(*ptr)++;
		case 2:
			value = (value << 8) | *(*ptr)++;
		case 1:
			value = (value << 8) | *(*ptr)++;
	}

	return value;
}

static unsigned long
id3_parse_syncsafe(const unsigned char** ptr, unsigned int bytes)
{
	unsigned long value = 0;

	ASSERT(bytes == 4 || bytes == 5);

	switch (bytes) {
		case 5:
			value = (value << 4) | (*(*ptr)++ & 0x0f);
		case 4:
			value = (value << 7) | (*(*ptr)++ & 0x7f);
			value = (value << 7) | (*(*ptr)++ & 0x7f);
			value = (value << 7) | (*(*ptr)++ & 0x7f);
			value = (value << 7) | (*(*ptr)++ & 0x7f);
	}

	return value;
}

static void
parse_header(const unsigned char** ptr,
			 unsigned int* version,
			 int* flags,
			 id3_length_t* size)
{
	*ptr += 3;

	*version = id3_parse_uint(ptr, 2);
	*flags = id3_parse_uint(ptr, 1);
	*size = id3_parse_syncsafe(ptr, 4);
}

/*
 * NAME:	tag->query()
 * DESCRIPTION:	if a tag begins at the given location, return its size
 */
signed long
id3_tag_query(const unsigned char* data, id3_length_t length)
{
	unsigned int version;
	int flags;
	id3_length_t size;

	switch (tagtype(data, length)) {
		case TAGTYPE_ID3V1:
			return 128;

		case TAGTYPE_ID3V2:
			parse_header(&data, &version, &flags, &size);

			if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
				size += 10;

			return 10 + size;

		case TAGTYPE_ID3V2_FOOTER:
			parse_header(&data, &version, &flags, &size);
			return -(int)size - 10;

		case TAGTYPE_NONE:
			break;
	}

	return 0;
}

/* XING code ripped out of madplay (header)
 *
 * mad - MPEG audio decoder
 * Copyright (C) 2000-2001 Robert Leslie
 */
struct xing
{
	long flags;				/* valid fields (see below) */
	unsigned long frames;   /* total number of frames */
	unsigned long bytes;	/* total number of bytes */
	unsigned char toc[100]; /* 100-point seek table */
	long scale;				/* ?? */
	enum
	{
		XING,
		INFO
	} type;
};

enum
{
	XING_FRAMES = 0x00000001L,
	XING_BYTES = 0x00000002L,
	XING_TOC = 0x00000004L,
	XING_SCALE = 0x00000008L
};

void
xing_init(struct xing* xing);
int
xing_parse(struct xing* xing, struct mad_bitptr ptr, unsigned int bitlen);

/* end XING header */

/* a -= b */
static void
mad_timer_sub(mad_timer_t* a, mad_timer_t b)
{
	/* a = a - b -> a = a + -b */
	mad_timer_negate(&b);
	mad_timer_add(a, b);
}

/* internal->decoder_private field */
struct madlib_t
{
	madlib_t()
	{
		outpos = 0;
		outleft = 0;
		memset(inbuf, 0, sizeof(inbuf));
		memset(outbuf, 0, sizeof(outbuf));
		memset(&Stream, 0, sizeof(Stream));
		memset(&Frame, 0, sizeof(Frame));
		memset(&Synth, 0, sizeof(Synth));
		Timer = mad_timer_zero;
		timer_accurate = false;
		inbuf_filepos = 0;
		filesize = 0;
		header_bytes = 0;
		first_frame = 0;
		has_xing = 0;
		memset(&xingtag, 0, sizeof(xingtag));
		length = 0;
		framelength = mad_timer_zero;
		bitrate = 0;
	}

	uint8_t inbuf[16384];
	float outbuf[8192];
	int outpos;
	unsigned outleft;

	struct mad_stream Stream;
	struct mad_frame Frame;
	struct mad_synth Synth;
	/* Timestamp of the next frame. */
	mad_timer_t Timer;

	/* Whether Timer is trusted; this is false after doing a quick seek. */
	int timer_accurate;

	/*
	 * Frame index of each percentage of the file.  This is constructed
	 * as we read the file, so we can quickly seek back later.
	 */
	struct mad_timer_compare_lt
	{
		bool operator()(const mad_timer_t& timer1,
						const mad_timer_t& timer2) const
		{
			return mad_timer_compare(timer1, timer2) < 0;
		}
	};

	typedef std::map<mad_timer_t, int, mad_timer_compare_lt> tocmap_t;
	tocmap_t tocmap;

	/* Position in the file of inbuf: */
	int inbuf_filepos;

	/* File size. */
	int filesize;

	/* Number of bytes of header data at the beginning; used for seeking. */
	int header_bytes;

	/* Whether we've decoded the first frame of real audio yet.  If this
	 * is false, we're somewhere before the first frame; we might be in
	 * the middle of headers. */
	int first_frame;

	/* This data is filled in when the first frame is decoded. */
	int has_xing; /* whether xingtag is valid */
	struct xing xingtag;

	/* If has_xing is true, this is filled in based on the xing header.
	 * If it's false, this is filled in based on the size of the file
	 * and the first frame. */
	int length;
	mad_timer_t framelength;

	/* If we have a Xing tag, this is the average bitrate; otherwise it's
	 * the bitrate of the first frame. */
	int bitrate;
};

static float
scale(mad_fixed_t sample)
{
	return float(double(sample) / (1 << MAD_F_FRACBITS));
}

static int
get_this_frame_byte(const madlib_t* mad)
{
	int ret = mad->inbuf_filepos;

	/* If we have a frame, adjust. */
	if (mad->Stream.this_frame != nullptr)
		ret += mad->Stream.this_frame - mad->inbuf;

	return ret;
}

/* Called on the first frame decoded.  Returns true if this frame
 * should be ignored. */
bool
RageSoundReader_MP3::handle_first_frame()
{
	using std::max;
	bool ret = false;

	/* Check for a XING tag. */
	xing_init(&mad->xingtag);
	if (xing_parse(
		  &mad->xingtag, mad->Stream.anc_ptr, mad->Stream.anc_bitlen) == 0) {
		/*
		 * "Info" tags are written by some tools.  They're just Xing tags, but
		 * for CBR files.
		 *
		 * However, DWI's decoder, BASS, doesn't understand this, and treats it
		 * as a corrupt frame, outputting a frame of silence.  Let's ignore the
		 * tag, so it'll be treated as an invalid frame, so we match DWI sync.
		 *
		 * The information in it isn't very useful to us.  The TOC is less
		 * accurate then computing it ourself (low resolution).  The file length
		 * computation might be wrong, if the tag is incorrect, and we can
		 * compute that accurately ourself for CBR files.
		 */
		if (mad->xingtag.type == xing::INFO)
			return false;

		mad->header_bytes = std::max(mad->header_bytes, get_this_frame_byte(mad));

		mad->has_xing = true;

		mad_timer_t tm = mad->Frame.header.duration;
		/* XXX: does this include the Xing header itself? */
		mad_timer_multiply(&tm, mad->xingtag.frames);
		mad->length = mad_timer_count(tm, MAD_UNITS_MILLISECONDS);

		/* XXX: an id3v2 footer tag would throw this off a little. This also
		 * assumes the Xing tag is the last header; it always is, I think. */
		int bytes = mad->filesize - mad->header_bytes;
		mad->bitrate = (int)(bytes * 8 / (mad->length / 1000.f));

		if (mad->xingtag.type == xing::XING)
			ret = 1;
	}

	/* If there's no Xing tag, mad->length will be filled in by _open. */

	return ret;
}

int
RageSoundReader_MP3::fill_buffer()
{
	/* Need more data. */
	int inbytes = 0;
	if (mad->Stream.next_frame != nullptr) {
		/* Pull out remaining data from the last buffer. */
		inbytes = mad->Stream.bufend - mad->Stream.next_frame;
		memmove(mad->inbuf, mad->Stream.next_frame, inbytes);
		mad->inbuf_filepos += mad->Stream.next_frame - mad->inbuf;
	}

	const bool bWasAtEOF = m_pFile->AtEOF();

	int rc = m_pFile->Read(mad->inbuf + inbytes,
						   sizeof(mad->inbuf) - inbytes - MAD_BUFFER_GUARD);
	if (rc < 0) {
		SetError(m_pFile->GetError());
		return -1;
	}

	if (m_pFile->AtEOF() && !bWasAtEOF) {
		/* We just reached EOF.  Append MAD_BUFFER_GUARD bytes of NULs to the
		 * buffer, to ensure that the last frame is flushed. */
		memset(mad->inbuf + inbytes + rc, 0, MAD_BUFFER_GUARD);
		rc += MAD_BUFFER_GUARD;
	}

	if (rc == 0)
		return 0;

	inbytes += rc;
	mad_stream_buffer(&mad->Stream, mad->inbuf, inbytes);
	return rc;
}

void
fill_frame_index_cache(madlib_t* mad)
{
	int pos;

	/* Only update the frame cache if our timer is consistent. */
	if (!mad->timer_accurate)
		return;

	/* the frame we just decoded: */
	pos = get_this_frame_byte(mad);

	/* Fill in the TOC percent. */
	if (!mad->tocmap.empty()) {
		/* Don't add an entry if one already exists near this time. */
		madlib_t::tocmap_t::iterator it = mad->tocmap.upper_bound(mad->Timer);
		if (it != mad->tocmap.begin()) {
			--it;
			int iDiffSeconds = mad->Timer.seconds - it->first.seconds;
			if (iDiffSeconds < 5)
				return;
		}
	}

	mad->tocmap[mad->Timer] = pos;
}

/* Handle first-stage decoding: extracting the MP3 frame data. */
int
RageSoundReader_MP3::do_mad_frame_decode(bool headers_only)
{
	int bytes_read = 0;

	for (;;) {
		int ret;

		/* Always actually decode the first packet, so we cleanly parse Xing
		 * tags. */
		if (headers_only && !mad->first_frame)
			ret = mad_header_decode(&mad->Frame.header, &mad->Stream);
		else
			ret = mad_frame_decode(&mad->Frame, &mad->Stream);

		if (ret == -1 && (mad->Stream.error == MAD_ERROR_BUFLEN ||
						  mad->Stream.error == MAD_ERROR_BUFPTR)) {
			if (bytes_read > 25000) {
				/* We've read this much without actually getting a frame; error.
				 */
				SetError("Can't find data");
				return -1;
			}

			ret = fill_buffer();
			if (ret <= 0)
				return ret;
			bytes_read += ret;

			continue;
		}

		if (ret == -1 && mad->Stream.error == MAD_ERROR_LOSTSYNC) {
			/* This might be an ID3V2 tag. */
			const int tagsize =
			  id3_tag_query(mad->Stream.this_frame,
							mad->Stream.bufend - mad->Stream.this_frame);

			if (tagsize) {
				mad_stream_skip(&mad->Stream, tagsize);

				/* Don't count the tagsize against the max-read-per-call figure.
				 */
				bytes_read -= tagsize;

				continue;
			}
		}

		if (ret == -1 && mad->Stream.error == MAD_ERROR_BADDATAPTR) {
			/*
			 * Something's corrupt.  One cause of this is cutting an MP3 in the
			 * middle without reencoding; the first two frames will reference
			 * data from previous frames that have been removed.  The frame is
			 * valid--we can get a header from it, we just can't synth useful
			 * data.
			 *
			 * BASS pretends the bad frames are silent.  Emulate that, for
			 * compatibility.
			 */
			ret = 0; /* pretend success */
		}

		if (!ret) {
			/* OK. */
			if (mad->first_frame) {
				/* We're at the beginning.  Is this a Xing tag? */
				if (handle_first_frame()) {
					/* The first frame contained a header. Continue searching.
					 */
					continue;
				}

				/* We've decoded the first frame of data.
				 *
				 * We want mad->Timer to represent the timestamp of the first
				 * sample of the currently decoded frame.  Don't increment
				 * mad->Timer on the first frame, or it'll be the time of the
				 * *next* frame.  (All frames have the same duration.) */
				mad->first_frame = false;
				mad->Timer = mad_timer_zero;
				mad->header_bytes = get_this_frame_byte(mad);
			} else {
				mad_timer_add(&mad->Timer, mad->Frame.header.duration);
			}

			fill_frame_index_cache(mad);

			return 1;
		}

		if (mad->Stream.error == MAD_ERROR_BADCRC) {
			/* XXX untested */
			mad_frame_mute(&mad->Frame);
			mad_synth_mute(&mad->Synth);

			continue;
		}

		if (!MAD_RECOVERABLE(mad->Stream.error)) {
			/* We've received an unrecoverable error. */
			SetError(mad_stream_errorstr(&mad->Stream));
			return -1;
		}
	}
}

void
RageSoundReader_MP3::synth_output()
{
	mad->outleft = mad->outpos = 0;

	if (MAD_NCHANNELS(&mad->Frame.header) != this->Channels) {
		/* This frame contains a different number of channels than the first.
		 * I've never actually seen this--it's just to prevent exploding if
		 * it happens--and we could continue on easily enough, so if it happens,
		 * just continue. */
		return;
	}

	mad_synth_frame(&mad->Synth, &mad->Frame);
	for (int i = 0; i < mad->Synth.pcm.length; i++) {
		for (int chan = 0; chan < this->Channels; ++chan) {
			float Sample = scale(mad->Synth.pcm.samples[chan][i]);
			mad->outbuf[mad->outleft] = Sample;
			++mad->outleft;
		}
	}
}

/* Seek to a byte in the file.  If you're going to put the file position
 * back when you're done, and not going to read any data, you don't have
 * to use this. */
int
RageSoundReader_MP3::seek_stream_to_byte(int byte)
{
	if (m_pFile->Seek(byte) == -1) {
		SetError(strerror(errno));
		return 0;
	}

	mad_frame_mute(&mad->Frame);
	mad_synth_mute(&mad->Synth);
	mad_stream_finish(&mad->Stream);
	mad_stream_init(&mad->Stream);

	mad->outleft = mad->outpos = 0;

	mad->inbuf_filepos = byte;

	/* If the position is <= the position of the first audio sample, then
	 * we're at the beginning. */
	if (!mad->tocmap.empty())
		mad->first_frame = (byte <= mad->tocmap.begin()->second);

	return 1;
}

#if 0
/* Call this after seeking the stream.  We'll back up a bit and reread
 * frames until we're back where we started, so the next read is aligned
 * to a frame and synced.  This must never leave the position ahead of where
 * it way, since that can confuse the seek optimizations. */
int RageSoundReader_MP3::resync()
{
	using std::max;
	/* Save the timer; decoding will change it, and we need to put it back. */
	mad_timer_t orig = mad->Timer;

	/* Seek backwards up to 4k. */
	const int origpos = mad->inbuf_filepos;
	const int seekpos = std::max( 0, origpos - 1024*4 );
	seek_stream_to_byte( seekpos );

	/* Agh.  This is annoying.  We want to decode enough so that the next frame
	 * read will be the first frame after the current file pointer.  If we just
	 * read until the file pointer is >= what it was, we've passed it already.
	 * So, read until it's >= what it was, counting the number of times we had
	 * to read; then back up again and read n-1 times.  Gross. */
	int reads = 0;
	do
	{
		if( do_mad_frame_decode() <= 0 ) /* XXX eof */
			return -1; /* it set the error */

		reads++;
	} while( get_this_frame_byte(mad) < origpos );

	seek_stream_to_byte( seekpos );

	reads--;
	while( reads-- > 0 )
	{
		if( do_mad_frame_decode() <= 0 ) /* XXX eof */
			return -1; /* it set the error */
	}

	/* Restore the timer. */
	mad->Timer = orig;
	mad->outpos = mad->outleft = 0;

	return 1;
}
#endif

RageSoundReader_MP3::RageSoundReader_MP3()
{
	mad = new madlib_t;
	m_bAccurateSync = false;

	mad_stream_init(&mad->Stream);
	mad_frame_init(&mad->Frame);
	mad_synth_init(&mad->Synth);

	mad_frame_mute(&mad->Frame);
	mad_timer_reset(&mad->Timer);
	mad->length = -1;
	mad->inbuf_filepos = 0;
	mad->header_bytes = 0;
	mad->has_xing = false;
	mad->timer_accurate = 1;
	mad->bitrate = -1;
	mad->first_frame = true;
}

RageSoundReader_MP3::~RageSoundReader_MP3()
{
	mad_synth_finish(&mad->Synth);
	mad_frame_finish(&mad->Frame);
	mad_stream_finish(&mad->Stream);

	delete mad;
}

RageSoundReader_FileReader::OpenResult
RageSoundReader_MP3::Open(RageFileBasic* pFile)
{
	m_pFile = pFile;

	mad->filesize = m_pFile->GetFileSize();
	ASSERT(mad->filesize != -1);

	/* Make sure we can decode at least one frame.  This will also fill in
	 * header info. */
	mad->outpos = 0;

	int ret = do_mad_frame_decode();
	switch (ret) {
		case 0:
			SetError("Failed to read any data at all");
			return OPEN_UNKNOWN_FILE_FORMAT;
		case -1:
			SetError(GetError() + " (not an MP3 stream?)");
			return OPEN_UNKNOWN_FILE_FORMAT;
	}

	/* Store the bitrate of the frame we just got. */
	if (mad->bitrate == -1) /* might have been set by a Xing tag */
		mad->bitrate = mad->Frame.header.bitrate;

	SampleRate = mad->Frame.header.samplerate;
	mad->framelength = mad->Frame.header.duration;
	this->Channels = MAD_NCHANNELS(&mad->Frame.header);

	/* Since we've already decoded a frame, just synth it instead of rewinding
	 * the stream. */
	synth_output();

	if (mad->length == -1) {
		/* If vbr and !xing, this is just an estimate. */
		int bps = mad->bitrate / 8;
		float secs =
		  static_cast<float>(mad->filesize - mad->header_bytes) / bps;
		mad->length = (int)(secs * 1000.f);
	}

	return OPEN_OK;
}

RageSoundReader_MP3*
RageSoundReader_MP3::Copy() const
{
	RageSoundReader_MP3* ret = new RageSoundReader_MP3;

	ret->m_pFile = m_pFile->Copy();
	ret->m_pFile->Seek(0);
	ret->m_bAccurateSync = m_bAccurateSync;
	ret->mad->filesize = mad->filesize;
	ret->mad->bitrate = mad->bitrate;
	ret->SampleRate = SampleRate;
	ret->mad->framelength = mad->framelength;
	ret->Channels = Channels;
	ret->mad->length = mad->length;

	//	int n = ret->do_mad_frame_decode();
	//	ASSERT( n > 0 );
	//	ret->synth_output();

	return ret;
}

int
RageSoundReader_MP3::Read(float* buf, int iFrames)
{
	using std::min;
	int iFramesWritten = 0;

	while (iFrames > 0) {
		if (mad->outleft > 0) {
			int iFramesToCopy =
			  min(iFrames, int(mad->outleft / GetNumChannels()));
			const int iSamplesToCopy = iFramesToCopy * GetNumChannels();
			const int iBytesToCopy = iSamplesToCopy * sizeof(float);

			memcpy(buf, mad->outbuf + mad->outpos, iBytesToCopy);

			buf += iSamplesToCopy;
			iFrames -= iFramesToCopy;
			iFramesWritten += iFramesToCopy;
			mad->outpos += iSamplesToCopy;
			mad->outleft -= iSamplesToCopy;
			continue;
		}

		/* Decode more from the MP3 stream. */
		int ret = do_mad_frame_decode();
		if (ret == 0)
			return END_OF_FILE;
		if (ret == -1)
			return RageSoundReader::RSRERROR;

		synth_output();
	}

	return iFramesWritten;
}

bool
RageSoundReader_MP3::MADLIB_rewind()
{
	m_pFile->Seek(0);

	mad_frame_mute(&mad->Frame);
	mad_synth_mute(&mad->Synth);
	mad_timer_reset(&mad->Timer);
	mad->outpos = mad->outleft = 0;

	mad_stream_finish(&mad->Stream);
	mad_stream_init(&mad->Stream);
	mad_stream_buffer(&mad->Stream, nullptr, 0);
	mad->inbuf_filepos = 0;

	/* Be careful.  We need to leave header_bytes alone, so if we try to
	 * SetPosition_estimate immediately after this, we still know the header
	 * size.  However, we need to set first_frame to true, since the first frame
	 * is handled specially in do_mad_frame_decode; if we don't set it, then
	 * we'll be desynced by a frame after an accurate seek. */
	//	mad->header_bytes = 0;
	mad->first_frame = true;
	mad->Stream.this_frame = nullptr;

	return true;
}

/* Methods of seeking:
 *
 * 1. We can jump based on a TOC.  We potentially have two; the Xing TOC and our
 *    own index.  The Xing TOC is only accurate to 1/256th of the file size,
 *    so it's unsuitable for precise seeks.  Our own TOC is byte-accurate.
 *    (SetPosition_toc)
 *
 * 2. We can jump based on the bitrate.  This is fast, but not accurate.
 *    (SetPosition_estimate)
 *
 * 3. We can seek from any position to any higher position by decoding headers.
 *    (SetPosition_hard)
 *
 * Both 1 and 2 will leave the position behind the actual requested position;
 * combine them with 3 to catch up. Never do 3 alone in "fast" mode, since it's
 * slow if it ends up seeking from the beginning of the file.  Never do 2 in
 * "precise" mode.
 */

/* Returns actual position on success, 0 if past EOF, -1 on error. */
int
RageSoundReader_MP3::SetPosition_toc(int iFrame, bool Xing)
{
	using std::max;
	ASSERT(!Xing || mad->has_xing);
	ASSERT(mad->length != -1);

	/* This leaves our timer accurate if we're using our own TOC, and inaccurate
	 * if we're using Xing. */
	mad->timer_accurate = !Xing;

	int bytepos = -1;
	if (Xing) {
		/* We can speed up the seek using the XING tag.  First, figure
		 * out what percentage the requested position falls in. */
		ASSERT(SampleRate != 0);
		int ms = int((iFrame * 1000LL) / SampleRate);
		ASSERT(mad->length != 0);
		int percent = ms * 100 / mad->length;
		if (percent < 100) {
			int jump = mad->xingtag.toc[percent];
			bytepos = mad->filesize * jump / 256;
		} else
			bytepos = 2000000000; /* force EOF */

		mad_timer_set(&mad->Timer, 0, percent * mad->length, 100000);
	} else {
		mad_timer_t desired;
		mad_timer_set(&desired, 0, iFrame, SampleRate);

		if (mad->tocmap.empty())
			return 1; /* don't have any info */

		/* Find the last entry <= iFrame that we actually have an entry for;
		 * this will get us as close as possible. */
		madlib_t::tocmap_t::iterator it = mad->tocmap.upper_bound(desired);
		if (it == mad->tocmap.begin())
			return 1; /* don't have any info */
		--it;

		mad->Timer = it->first;
		bytepos = it->second;
	}

	if (bytepos != -1) {
		/* Seek backwards up to 4k. */
		const int seekpos = std::max(0, bytepos - 1024 * 4);
		seek_stream_to_byte(seekpos);

		do {
			int ret = do_mad_frame_decode();
			if (ret <= 0)
				return ret; /* it set the error */
		} while (get_this_frame_byte(mad) < bytepos);
		synth_output();
	}

	return 1;
}

int
RageSoundReader_MP3::SetPosition_hard(int iFrame)
{
	mad_timer_t desired;
	mad_timer_set(&desired, 0, iFrame, mad->Frame.header.samplerate);

	/* This seek doesn't change the accuracy of our timer. */

	/* If we're already exactly at the requested position, OK. */
	if (mad_timer_compare(mad->Timer, desired) == 0)
		return 1;

	/* We always come in here with data synthed.  Be careful not to synth the
	 * same frame twice. */
	bool synthed = true;

	/* If we're already past the requested position, rewind. */
	if (mad_timer_compare(mad->Timer, desired) > 0) {
		MADLIB_rewind();
		do_mad_frame_decode();
		synthed = false;
	}

	/* Decode frames until the current frame contains the desired offset. */
	for (;;) {
		/* If desired < next_frame_timer, this frame contains the position.
		 * Since we've already decoded the frame, synth it, too. */
		mad_timer_t next_frame_timer = mad->Timer;
		mad_timer_add(&next_frame_timer, mad->framelength);

		if (mad_timer_compare(desired, next_frame_timer) < 0) {
			if (!synthed) {
				synth_output();
			} else {
				mad->outleft += mad->outpos;
				mad->outpos = 0;
			}

			/* We just synthed data starting at mad->Timer, containing the
			 * desired offset. Skip (desired - mad->Timer) worth of frames in
			 * the output to line up. */
			mad_timer_t skip = desired;
			mad_timer_sub(&skip, mad->Timer);

			int samples = mad_timer_count(skip, (mad_units)SampleRate);

			/* Skip 'samples' samples. */
			mad->outpos = samples * this->Channels;
			mad->outleft -= samples * this->Channels;
			return 1;
		}

		/* Otherwise, if the desired time will be in the *next* decode, then
		 * synth this one, too. */
		mad_timer_t next_next_frame_timer = next_frame_timer;
		mad_timer_add(&next_next_frame_timer, mad->framelength);

		if (mad_timer_compare(desired, next_next_frame_timer) < 0 && !synthed) {
			synth_output();
			synthed = true;
		}

		int ret = do_mad_frame_decode();
		if (ret <= 0) {
			mad->outleft = mad->outpos = 0;
			return ret; /* it set the error */
		}

		synthed = false;
	}
}

/* Do a seek based on the bitrate. */
int
RageSoundReader_MP3::SetPosition_estimate(int iFrame)
{
	/* This doesn't leave us accurate. */
	mad->timer_accurate = 0;

	mad_timer_t seekamt;
	mad_timer_set(&seekamt, 0, iFrame, mad->Frame.header.samplerate);
	{
		/* We're going to skip ahead two samples below, so seek earlier than
		 * we were asked to. */
		mad_timer_t back_len = mad->framelength;
		mad_timer_multiply(&back_len, -2);
		mad_timer_add(&seekamt, back_len);
		if (mad_timer_compare(seekamt, mad_timer_zero) < 0)
			seekamt = mad_timer_zero;
	}

	int seekpos = mad_timer_count(seekamt, MAD_UNITS_MILLISECONDS) *
				  (mad->bitrate / 8 / 1000);
	seekpos += mad->header_bytes;
	seek_stream_to_byte(seekpos);

	/* We've jumped across the file, so the decoder is currently desynced.
	 * Don't use resync(); it's slow.  Just decode a few frames. */
	for (int i = 0; i < 2; ++i) {
		int ret = do_mad_frame_decode();
		if (ret <= 0)
			return ret;
	}

	/* Throw out one synth. */
	synth_output();
	mad->outleft = 0;

	/* Find out where we really seeked to. */
	int ms = (get_this_frame_byte(mad) - mad->header_bytes) /
			 (mad->bitrate / 8 / 1000);
	mad_timer_set(&mad->Timer, 0, ms, 1000);

	return 1;
}

int
RageSoundReader_MP3::SetPosition(int iFrame)
{
	if (m_bAccurateSync) {
		/* Seek using our own internal (accurate) TOC. */
		int ret = SetPosition_toc(iFrame, false);
		if (ret <= 0)
			return ret; /* it set the error */

		/* Align exactly. */
		return SetPosition_hard(iFrame);
	} else {
		/* Rewinding is always fast and accurate, and SetPosition_estimate is
		 * bad at 0. */
		if (!iFrame) {
			MADLIB_rewind();
			return 1; /* ok */
		}

		/* We can do a fast jump in VBR with Xing with more accuracy than
		 * without Xing. */
		if (mad->has_xing)
			return SetPosition_toc(iFrame, true);

		/* Guess.  This is only remotely accurate when we're not VBR, but also
		 * do it if we have no Xing tag. */
		return SetPosition_estimate(iFrame);
	}
}

bool
RageSoundReader_MP3::SetProperty(const std::string& sProperty, float fValue)
{
	if (sProperty == "AccurateSync") {
		m_bAccurateSync = (fValue > 0.001f);
		return true;
	}

	return RageSoundReader_FileReader::SetProperty(sProperty, fValue);
}

int
RageSoundReader_MP3::GetNextSourceFrame() const
{
	int iFrame =
	  mad_timer_count(mad->Timer, mad_units(mad->Frame.header.samplerate));
	iFrame += mad->outpos / this->Channels;
	return iFrame;
}

int
RageSoundReader_MP3::GetLengthInternal(bool fast)
{
	if (mad->has_xing && mad->length != -1)
		return mad->length; /* should be accurate */

	/* Check to see if a frame in the middle of the file is the same
	 * bitrate as the first frame.  If it is, assume the file is really CBR. */
	seek_stream_to_byte(mad->filesize / 2);

	/* XXX use mad_header_decode and check more than one frame */
	if (mad->length != -1 && do_mad_frame_decode() &&
		mad->bitrate == (int)mad->Frame.header.bitrate) {
		return mad->length;
	}

	if (!MADLIB_rewind())
		return 0;

	/* Worst-case: vbr && !xing.  We've made a guess at the length, but let's
	 * actually scan the size, since the guess is probably wrong. */
	if (fast) {
		SetError("Can't estimate file length");
		return -1;
	}

	MADLIB_rewind();
	for (;;) {
		int ret = do_mad_frame_decode(true);
		if (ret == -1) {
			return -1; /* it set the error */
		}
		if (ret == 0) /* EOF */
		{
			break;
		}
	}

	/* mad->Timer is the timestamp of the current frame; find the timestamp of
	 * the very end. */
	mad_timer_t end = mad->Timer;
	mad_timer_add(&end, mad->framelength);

	/* Count milliseconds. */
	return mad_timer_count(end, MAD_UNITS_MILLISECONDS);
}

int
RageSoundReader_MP3::GetLengthConst(bool fast) const
{
	RageSoundReader_MP3* pCopy = this->Copy();

	int iLength = pCopy->GetLengthInternal(fast);

	delete pCopy;
	return iLength;
}

/* begin XING code ripped out of madplay */

/*
 * NAME:	xing->init()
 * DESCRIPTION:	initialize Xing structure
 */
void
xing_init(struct xing* xing)
{
	xing->flags = 0;
}

/*
 * NAME:	xing->parse()
 * DESCRIPTION:	parse a Xing VBR header
 */
int
xing_parse(struct xing* xing, struct mad_bitptr ptr, unsigned int bitlen)
{
	const unsigned XING_MAGIC = (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g');
	const unsigned INFO_MAGIC = (('I' << 24) | ('n' << 16) | ('f' << 8) | 'o');
	unsigned data;
	if (bitlen < 64)
		goto fail;
	data = mad_bit_read(&ptr, 32);

	if (data == XING_MAGIC)
		xing->type = xing::XING;
	else if (data == INFO_MAGIC)
		xing->type = xing::INFO;
	else
		goto fail;

	xing->flags = mad_bit_read(&ptr, 32);
	bitlen -= 64;

	if (xing->flags & XING_FRAMES) {
		if (bitlen < 32)
			goto fail;

		xing->frames = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if (xing->flags & XING_BYTES) {
		if (bitlen < 32)
			goto fail;

		xing->bytes = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if (xing->flags & XING_TOC) {
		if (bitlen < 800)
			goto fail;

		for (int i = 0; i < 100; ++i)
			xing->toc[i] = (unsigned char)mad_bit_read(&ptr, 8);

		bitlen -= 800;
	}

	if (xing->flags & XING_SCALE) {
		if (bitlen < 32)
			goto fail;

		xing->scale = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	return 0;

fail:
	xing->flags = 0;
	return -1;
}

/* end XING code ripped out of madplay */

/*
 * Copyright (c) 2003-2004 Glenn Maynard
 * Copyright (c) 2000-2003 Robert Leslie (Xing code from madplay)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
