# TODO: Turn Rage into a libary.

list(APPEND SMDATA_RAGE_UTILS_SRC
  "RageUtil/Utils/RageUtil.cpp"
  "RageUtil/Utils/RageUtil_CachedObject.cpp"
  "RageUtil/Utils/RageUtil_CharConversions.cpp"
  "RageUtil/Utils/RageUtil_FileDB.cpp"
  "RageUtil/Utils/RageUtil_WorkerThread.cpp"
)

list(APPEND SMDATA_RAGE_UTILS_HPP
  "RageUtil/Utils/RageUtil.h"
  "RageUtil/Utils/RageUtil_AutoPtr.h" # TODO: Remove the need for this and replace with c++11 smart pointers
  "RageUtil/Utils/RageUtil_CachedObject.h"
  "RageUtil/Utils/RageUtil_CharConversions.h"
  "RageUtil/Utils/RageUtil_CircularBuffer.h"
  "RageUtil/Utils/RageUtil_FileDB.h"
  "RageUtil/Utils/RageUtil_WorkerThread.h"
)

source_group("Rage\\\\Utils" FILES ${SMDATA_RAGE_UTILS_SRC} ${SMDATA_RAGE_UTILS_HPP})

list(APPEND SMDATA_RAGE_MISC_SRC
  "RageUtil/Misc/RageException.cpp"
  "RageUtil/Misc/RageInput.cpp"
  "RageUtil/Misc/RageInputDevice.cpp"
  "RageUtil/Misc/RageLog.cpp"
  "RageUtil/Misc/RageMath.cpp"
  "RageUtil/Misc/RageString.cpp"
  "RageUtil/Misc/RageTypes.cpp"
  "RageUtil/Misc/RageThreads.cpp"
  "RageUtil/Misc/RageTimer.cpp"
  "RageUtil/Misc/RageUnicode.h"
)

list(APPEND SMDATA_RAGE_MISC_HPP
  "RageUtil/Misc/RageException.h"
  "RageUtil/Misc/RageInput.h"
  "RageUtil/Misc/RageInputDevice.h"
  "RageUtil/Misc/RageLog.h"
  "RageUtil/Misc/RageMath.h"
  "RageUtil/Misc/RageString.h"
  "RageUtil/Misc/RageTypes.h"
  "RageUtil/Misc/RageThreads.h"
  "RageUtil/Misc/RageTimer.h"
  "RageUtil/Misc/RageUnicode.cpp"
)

source_group("Rage\\\\Misc" FILES ${SMDATA_RAGE_MISC_SRC} ${SMDATA_RAGE_MISC_HPP})

list(APPEND SMDATA_RAGE_GRAPHICS_SRC
  "RageUtil/Graphics/RageBitmapTexture.cpp"
  "RageUtil/Graphics/RageDisplay.cpp"
  "RageUtil/Graphics/RageDisplay_Null.cpp"
  "RageUtil/Graphics/RageDisplay_OGL.cpp"
  "RageUtil/Graphics/RageDisplay_OGL_Helpers.cpp"
  "RageUtil/Graphics/RageModelGeometry.cpp"
  "RageUtil/Graphics/RageSurface.cpp"
  "RageUtil/Graphics/RageSurface_Load.cpp"
  "RageUtil/Graphics/RageSurface_Load_XPM.cpp"
  "RageUtil/Graphics/RageSurface_Save_BMP.cpp"
  "RageUtil/Graphics/RageSurface_Save_JPEG.cpp"
  "RageUtil/Graphics/RageSurface_Save_PNG.cpp"
  "RageUtil/Graphics/RageSurfaceUtils.cpp"
  "RageUtil/Graphics/RageSurfaceUtils_Dither.cpp"
  "RageUtil/Graphics/RageSurfaceUtils_Palettize.cpp"
  "RageUtil/Graphics/RageSurfaceUtils_Zoom.cpp"
  "RageUtil/Graphics/RageTexture.cpp"
  "RageUtil/Graphics/RageTextureID.cpp"
  "RageUtil/Graphics/RageTextureManager.cpp"
  "RageUtil/Graphics/RageTextureRenderTarget.cpp"
)
list(APPEND SMDATA_RAGE_GRAPHICS_HPP
  "RageUtil/Graphics/RageBitmapTexture.h"
  "RageUtil/Graphics/RageDisplay.h"
  "RageUtil/Graphics/RageDisplay_Null.h"
  "RageUtil/Graphics/RageDisplay_OGL.h"
  "RageUtil/Graphics/RageDisplay_OGL_Helpers.h"
  "RageUtil/Graphics/RageModelGeometry.h"
  "RageUtil/Graphics/RageSurface.h"
  "RageUtil/Graphics/RageSurface_Load.h"
  "RageUtil/Graphics/RageSurface_Load_XPM.h"
  "RageUtil/Graphics/RageSurface_Save_BMP.h"
  "RageUtil/Graphics/RageSurface_Save_JPEG.h"
  "RageUtil/Graphics/RageSurface_Save_PNG.h"
  "RageUtil/Graphics/RageSurfaceUtils.h"
  "RageUtil/Graphics/RageSurfaceUtils_Dither.h"
  "RageUtil/Graphics/RageSurfaceUtils_Palettize.h"
  "RageUtil/Graphics/RageSurfaceUtils_Zoom.h"
  "RageUtil/Graphics/RageTexture.h"
  "RageUtil/Graphics/RageTextureID.h"
  "RageUtil/Graphics/RageTextureManager.h"
  "RageUtil/Graphics/RageTextureRenderTarget.h"
)

if(WIN32)
  list(APPEND SMDATA_RAGE_GRAPHICS_SRC "RageUtil/Graphics/RageDisplay_D3D.cpp")
  list(APPEND SMDATA_RAGE_GRAPHICS_HPP "RageUtil/Graphics/RageDisplay_D3D.h")
elseif(LINUX)
  if (WITH_GLES2)
    list(APPEND SMDATA_RAGE_GRAPHICS_SRC "RageUtil/Graphics/RageDisplay_GLES2.cpp")
    list(APPEND SMDATA_RAGE_GRAPHICS_HPP "RageUtil/Graphics/RageDisplay_GLES2.h")
  endif()
endif()

source_group("Rage\\\\Graphics" FILES ${SMDATA_RAGE_GRAPHICS_SRC} ${SMDATA_RAGE_GRAPHICS_HPP})

list(APPEND SMDATA_RAGE_FILE_SRC
  "RageUtil/File/RageFile.cpp"
  "RageUtil/File/RageFileBasic.cpp"
  "RageUtil/File/RageFileDriver.cpp"
  "RageUtil/File/RageFileDriverDeflate.cpp"
  "RageUtil/File/RageFileDriverDirect.cpp"
  "RageUtil/File/RageFileDriverDirectHelpers.cpp"
  "RageUtil/File/RageFileDriverMemory.cpp"
  "RageUtil/File/RageFileDriverReadAhead.cpp"
  "RageUtil/File/RageFileDriverSlice.cpp"
  "RageUtil/File/RageFileDriverTimeout.cpp"
  "RageUtil/File/RageFileDriverZip.cpp"
  "RageUtil/File/RageFileManager.cpp"
  "RageUtil/File/RageFileManager_ReadAhead.cpp"
)

list(APPEND SMDATA_RAGE_FILE_HPP
  "RageUtil/File/RageFile.h"
  "RageUtil/File/RageFileBasic.h"
  "RageUtil/File/RageFileDriver.h"
  "RageUtil/File/RageFileDriverDeflate.h"
  "RageUtil/File/RageFileDriverDirect.h"
  "RageUtil/File/RageFileDriverDirectHelpers.h"
  "RageUtil/File/RageFileDriverMemory.h"
  "RageUtil/File/RageFileDriverReadAhead.h"
  "RageUtil/File/RageFileDriverSlice.h"
  "RageUtil/File/RageFileDriverTimeout.h"
  "RageUtil/File/RageFileDriverZip.h"
  "RageUtil/File/RageFileManager.h"
  "RageUtil/File/RageFileManager_ReadAhead.h"
)

source_group("Rage\\\\File" FILES ${SMDATA_RAGE_FILE_SRC} ${SMDATA_RAGE_FILE_HPP})

list(APPEND SMDATA_RAGE_SOUND_SRC
  "RageUtil/Sound/RageSound.cpp"
  "RageUtil/Sound/RageSoundManager.cpp"
  "RageUtil/Sound/RageSoundMixBuffer.cpp"
  "RageUtil/Sound/RageSoundPosMap.cpp"
  "RageUtil/Sound/RageSoundReader.cpp"
  "RageUtil/Sound/RageSoundReader_Chain.cpp"
  "RageUtil/Sound/RageSoundReader_ChannelSplit.cpp"
  "RageUtil/Sound/RageSoundReader_Extend.cpp"
  "RageUtil/Sound/RageSoundReader_FileReader.cpp"
  "RageUtil/Sound/RageSoundReader_Merge.cpp"
  "RageUtil/Sound/RageSoundReader_Pan.cpp"
  "RageUtil/Sound/RageSoundReader_PitchChange.cpp"
  "RageUtil/Sound/RageSoundReader_PostBuffering.cpp"
  "RageUtil/Sound/RageSoundReader_Preload.cpp"
  "RageUtil/Sound/RageSoundReader_Resample_Good.cpp"
  "RageUtil/Sound/RageSoundReader_SpeedChange.cpp"
  "RageUtil/Sound/RageSoundReader_ThreadedBuffer.cpp"
  "RageUtil/Sound/RageSoundReader_WAV.cpp"
  "RageUtil/Sound/RageSoundUtil.cpp"
)
list(APPEND SMDATA_RAGE_SOUND_HPP
  "RageUtil/Sound/RageSound.h"
  "RageUtil/Sound/RageSoundManager.h"
  "RageUtil/Sound/RageSoundMixBuffer.h"
  "RageUtil/Sound/RageSoundPosMap.h"
  "RageUtil/Sound/RageSoundReader.h"
  "RageUtil/Sound/RageSoundReader_Chain.h"
  "RageUtil/Sound/RageSoundReader_ChannelSplit.h"
  "RageUtil/Sound/RageSoundReader_Extend.h"
  "RageUtil/Sound/RageSoundReader_FileReader.h"
  "RageUtil/Sound/RageSoundReader_Filter.h"
  "RageUtil/Sound/RageSoundReader_Merge.h"
  "RageUtil/Sound/RageSoundReader_Pan.h"
  "RageUtil/Sound/RageSoundReader_PitchChange.h"
  "RageUtil/Sound/RageSoundReader_PostBuffering.h"
  "RageUtil/Sound/RageSoundReader_Preload.h"
  "RageUtil/Sound/RageSoundReader_Resample_Good.h"
  "RageUtil/Sound/RageSoundReader_SpeedChange.h"
  "RageUtil/Sound/RageSoundReader_ThreadedBuffer.h"
  "RageUtil/Sound/RageSoundReader_WAV.h"
  "RageUtil/Sound/RageSoundUtil.h"
)

list(APPEND SMDATA_RAGE_SOUND_SRC "RageUtil/Sound/RageSoundReader_Vorbisfile.cpp")
list(APPEND SMDATA_RAGE_SOUND_HPP "RageUtil/Sound/RageSoundReader_Vorbisfile.h")

list(APPEND SMDATA_RAGE_SOUND_SRC "RageUtil/Sound/RageSoundReader_MP3.cpp")
list(APPEND SMDATA_RAGE_SOUND_HPP "RageUtil/Sound/RageSoundReader_MP3.h")

source_group("Rage\\\\Sound" FILES ${SMDATA_RAGE_SOUND_SRC} ${SMDATA_RAGE_SOUND_HPP})

list(APPEND SMDATA_ALL_RAGE_SRC
  ${SMDATA_RAGE_FILE_SRC}
  ${SMDATA_RAGE_GRAPHICS_SRC}
  ${SMDATA_RAGE_MISC_SRC}
  ${SMDATA_RAGE_SOUND_SRC}
  ${SMDATA_RAGE_UTILS_SRC}
)

list(APPEND SMDATA_ALL_RAGE_HPP
  ${SMDATA_RAGE_FILE_HPP}
  ${SMDATA_RAGE_GRAPHICS_HPP}
  ${SMDATA_RAGE_MISC_HPP}
  ${SMDATA_RAGE_SOUND_HPP}
  ${SMDATA_RAGE_UTILS_HPP}
)
