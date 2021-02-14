#ifndef MOVIE_TEXTURE_NULL_H
#define MOVIE_TEXTURE_NULL_H

#include "MovieTexture.h"

class RageMovieTextureDriver_Null : public RageMovieTextureDriver
{
  public:
	virtual RageMovieTexture* Create(const RageTextureID& ID,
									 std::string& sError);
};

#endif
