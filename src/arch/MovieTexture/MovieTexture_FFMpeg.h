/* MovieTexture_FFMpeg - FFMpeg movie renderer. */

#ifndef RAGE_MOVIE_TEXTURE_FFMPEG_H
#define RAGE_MOVIE_TEXTURE_FFMPEG_H

#include "MovieTexture_Generic.h"
struct RageSurface;

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define STEPMANIA_FFMPEG_BUFFER_SIZE 4096
static const int sws_flags = SWS_BICUBIC; // XXX: Reasonable default?

class MovieTexture_FFMpeg : public MovieTexture_Generic
{
  public:
	MovieTexture_FFMpeg(const RageTextureID& ID);

	static RageSurface* AVCodecCreateCompatibleSurface(
	  int iTextureWidth,
	  int iTextureHeight,
	  bool bPreferHighColor,
	  int& iAVTexfmt,
	  MovieDecoderPixelFormatYCbCr& fmtout);
};

class RageMovieTextureDriver_FFMpeg : public RageMovieTextureDriver
{
  public:
	virtual RageMovieTexture* Create(const RageTextureID& ID,
									 std::string& sError);
	static RageSurface* AVCodecCreateCompatibleSurface(
	  int iTextureWidth,
	  int iTextureHeight,
	  bool bPreferHighColor,
	  int& iAVTexfmt,
	  MovieDecoderPixelFormatYCbCr& fmtout);
};

class MovieDecoder_FFMpeg : public MovieDecoder
{
  public:
	MovieDecoder_FFMpeg();
	~MovieDecoder_FFMpeg();

	std::string Open(const std::string& sFile);
	void Close();
	void Rewind();

	void GetFrame(RageSurface* pOut);
	int DecodeFrame(float fTargetTime);

	int GetWidth() const;
	int GetHeight() const;

	RageSurface* CreateCompatibleSurface(int iTextureWidth,
										 int iTextureHeight,
										 bool bPreferHighColor,
										 MovieDecoderPixelFormatYCbCr& fmtout);

	float GetTimestamp() const;
	float GetFrameDuration() const;

  private:
	void Init();
	std::string OpenCodec();
	int ReadPacket();
	int DecodePacket(float fTargetTime);

	AVStream* m_pStream;
	AVCodecContext* m_CodecContext;
	AVFrame* m_Frame;
	AVPixelFormat m_AVTexfmt; /* PixelFormat of output surface */
	SwsContext* m_swsctx;

	AVFormatContext* m_fctx;
	float m_fTimestamp;
	float m_fTimestampOffset;
	float m_fLastFrameDelay;
	int m_iFrameNumber;

	unsigned char* m_buffer;
	AVIOContext* m_avioContext;

	AVPacket m_Packet;
	int m_iCurrentPacketOffset;
	float m_fLastFrame;

	/* 0 = no EOF
	 * 1 = EOF from ReadPacket
	 * 2 = EOF from ReadPacket and DecodePacket */
	int m_iEOF;
};

static struct AVPixelFormat_t
{
	int bpp;
	uint32_t masks[4];
	AVPixelFormat pf;
	bool bHighColor;
	bool bByteSwapOnLittleEndian;
	MovieDecoderPixelFormatYCbCr YUV;
} AVPixelFormats[] = { {
						 32,
						 { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF },
						 AV_PIX_FMT_YUYV422,
						 false, /* N/A */
						 true,
						 PixelFormatYCbCr_YUYV422,
					   },
					   {
						 32,
						 { 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF },
						 AV_PIX_FMT_BGRA,
						 true,
						 true,
						 PixelFormatYCbCr_Invalid,
					   },
					   {
						 32,
						 { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 },
						 AV_PIX_FMT_ARGB,
						 true,
						 true,
						 PixelFormatYCbCr_Invalid,
					   },
					   /*
					   {
						   32,
						   { 0x000000FF,
							 0x0000FF00,
							 0x00FF0000,
							 0xFF000000 },
						   avcodec::PIX_FMT_ABGR,
						   true,
						   true,
						   PixelFormatYCbCr_Invalid,
					   },
					   {
						   32,
						   { 0xFF000000,
							 0x00FF0000,
							 0x0000FF00,
							 0x000000FF },
						   avcodec::PIX_FMT_RGBA,
						   true,
						   true,
						   PixelFormatYCbCr_Invalid,
					   }, */
					   {
						 24,
						 { 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 },
						 AV_PIX_FMT_RGB24,
						 true,
						 true,
						 PixelFormatYCbCr_Invalid,
					   },
					   {
						 24,
						 { 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 },
						 AV_PIX_FMT_BGR24,
						 true,
						 true,
						 PixelFormatYCbCr_Invalid,
					   },
					   {
						 16,
						 { 0x7C00, 0x03E0, 0x001F, 0x0000 },
						 AV_PIX_FMT_RGB555,
						 false,
						 false,
						 PixelFormatYCbCr_Invalid,
					   },
					   { 0,
						 { 0, 0, 0, 0 },
						 AV_PIX_FMT_NB,
						 true,
						 false,
						 PixelFormatYCbCr_Invalid } };

#endif
