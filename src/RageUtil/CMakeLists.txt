# TODO: Turn Rage into a libary.

list(APPEND SMDATA_RAGE_UTILS_SRC
  "Utils/RageUtil.cpp"
  "Utils/RageUtil_CachedObject.cpp"
  "Utils/RageUtil_CharConversions.cpp"
  "Utils/RageUtil_FileDB.cpp"
  "Utils/RageUtil_WorkerThread.cpp"
)

list(APPEND SMDATA_RAGE_UTILS_HPP
     "Utils/RageUtil.h"
     "Utils/RageUtil_CachedObject.h"
     "Utils/RageUtil_CharConversions.h"
     "Utils/RageUtil_CircularBuffer.h"
     "Utils/RageUtil_FileDB.h"
     "Utils/RageUtil_WorkerThread.h"
     )

source_group("Rage\\\\Utils" FILES ${SMDATA_RAGE_UTILS_SRC} ${SMDATA_RAGE_UTILS_HPP})

list(APPEND SMDATA_RAGE_MISC_SRC
  "Misc/RageException.cpp"
  "Misc/RageInput.cpp"
  "Misc/RageInputDevice.cpp"
  "Misc/RageMath.cpp"
  "Misc/RageTypes.cpp"
  "Misc/RageThreads.cpp"
  "Misc/RageTimer.cpp"
  "Misc/RageUnicode.cpp"
)

list(APPEND SMDATA_RAGE_MISC_HPP
  "Misc/RageException.h"
  "Misc/RageInput.h"
  "Misc/RageInputDevice.h"
  "Misc/RageMath.h"
  "Misc/RageTypes.h"
  "Misc/RageThreads.h"
  "Misc/RageTimer.h"
  "Misc/RageUnicode.h"
)

source_group("Rage\\\\Misc" FILES ${SMDATA_RAGE_MISC_SRC} ${SMDATA_RAGE_MISC_HPP})

list(APPEND SMDATA_RAGE_GRAPHICS_SRC
  "Graphics/RageBitmapTexture.cpp"
  "Graphics/RageDisplay.cpp"
  "Graphics/RageDisplay_Null.cpp"
  "Graphics/RageDisplay_OGL.cpp"
  "Graphics/RageDisplay_OGL_Helpers.cpp"
  "Graphics/RageModelGeometry.cpp"
  "Graphics/RageSurface.cpp"
  "Graphics/RageSurface_Load.cpp"
  "Graphics/RageSurface_Load_XPM.cpp"
  "Graphics/RageSurface_Save_BMP.cpp"
  "Graphics/RageSurface_Save_JPEG.cpp"
  "Graphics/RageSurface_Save_PNG.cpp"
  "Graphics/RageSurfaceUtils.cpp"
  "Graphics/RageSurfaceUtils_Dither.cpp"
  "Graphics/RageSurfaceUtils_Palettize.cpp"
  "Graphics/RageSurfaceUtils_Zoom.cpp"
  "Graphics/RageTexture.cpp"
  "Graphics/RageTextureID.cpp"
  "Graphics/RageTextureManager.cpp"
  "Graphics/RageTextureRenderTarget.cpp"
)
list(APPEND SMDATA_RAGE_GRAPHICS_HPP
  "Graphics/RageBitmapTexture.h"
  "Graphics/RageDisplay.h"
  "Graphics/RageDisplay_Null.h"
  "Graphics/RageDisplay_OGL.h"
  "Graphics/RageDisplay_OGL_Helpers.h"
  "Graphics/RageModelGeometry.h"
  "Graphics/RageSurface.h"
  "Graphics/RageSurface_Load.h"
  "Graphics/RageSurface_Load_XPM.h"
  "Graphics/RageSurface_Save_BMP.h"
  "Graphics/RageSurface_Save_JPEG.h"
  "Graphics/RageSurface_Save_PNG.h"
  "Graphics/RageSurfaceUtils.h"
  "Graphics/RageSurfaceUtils_Dither.h"
  "Graphics/RageSurfaceUtils_Palettize.h"
  "Graphics/RageSurfaceUtils_Zoom.h"
  "Graphics/RageTexture.h"
  "Graphics/RageTextureID.h"
  "Graphics/RageTextureManager.h"
  "Graphics/RageTextureRenderTarget.h"
)

if(WIN32)
  list(APPEND SMDATA_RAGE_GRAPHICS_SRC "Graphics/RageDisplay_D3D.cpp")
  list(APPEND SMDATA_RAGE_GRAPHICS_HPP "Graphics/RageDisplay_D3D.h")
elseif(LINUX)
  if (WITH_GLES2)
    list(APPEND SMDATA_RAGE_GRAPHICS_SRC "Graphics/RageDisplay_GLES2.cpp")
    list(APPEND SMDATA_RAGE_GRAPHICS_HPP "Graphics/RageDisplay_GLES2.h")
  endif()
endif()

source_group("Rage\\\\Graphics" FILES ${SMDATA_RAGE_GRAPHICS_SRC} ${SMDATA_RAGE_GRAPHICS_HPP})

list(APPEND SMDATA_RAGE_FILE_SRC
  "File/RageFile.cpp"
  "File/RageFileBasic.cpp"
  "File/RageFileDriver.cpp"
  "File/RageFileDriverDeflate.cpp"
  "File/RageFileDriverDirect.cpp"
  "File/RageFileDriverDirectHelpers.cpp"
  "File/RageFileDriverMemory.cpp"
  "File/RageFileDriverReadAhead.cpp"
  "File/RageFileDriverSlice.cpp"
  "File/RageFileDriverTimeout.cpp"
  "File/RageFileDriverZip.cpp"
  "File/RageFileManager.cpp"
  "File/RageFileManager_ReadAhead.cpp"
)

list(APPEND SMDATA_RAGE_FILE_HPP
  "File/RageFile.h"
  "File/RageFileBasic.h"
  "File/RageFileDriver.h"
  "File/RageFileDriverDeflate.h"
  "File/RageFileDriverDirect.h"
  "File/RageFileDriverDirectHelpers.h"
  "File/RageFileDriverMemory.h"
  "File/RageFileDriverReadAhead.h"
  "File/RageFileDriverSlice.h"
  "File/RageFileDriverTimeout.h"
  "File/RageFileDriverZip.h"
  "File/RageFileManager.h"
  "File/RageFileManager_ReadAhead.h"
)

source_group("Rage\\\\File" FILES ${SMDATA_RAGE_FILE_SRC} ${SMDATA_RAGE_FILE_HPP})

list(APPEND SMDATA_RAGE_SOUND_SRC
  "Sound/RageSound.cpp"
  "Sound/RageSoundManager.cpp"
  "Sound/RageSoundMixBuffer.cpp"
  "Sound/RageSoundPosMap.cpp"
  "Sound/RageSoundReader.cpp"
  "Sound/RageSoundReader_Chain.cpp"
  "Sound/RageSoundReader_ChannelSplit.cpp"
  "Sound/RageSoundReader_Extend.cpp"
  "Sound/RageSoundReader_FileReader.cpp"
  "Sound/RageSoundReader_Merge.cpp"
  "Sound/RageSoundReader_Pan.cpp"
  "Sound/RageSoundReader_PitchChange.cpp"
  "Sound/RageSoundReader_PostBuffering.cpp"
  "Sound/RageSoundReader_Preload.cpp"
  "Sound/RageSoundReader_Resample_Good.cpp"
  "Sound/RageSoundReader_SpeedChange.cpp"
  "Sound/RageSoundReader_ThreadedBuffer.cpp"
  "Sound/RageSoundReader_WAV.cpp"
  "Sound/RageSoundUtil.cpp"
)
list(APPEND SMDATA_RAGE_SOUND_HPP
  "Sound/RageSound.h"
  "Sound/RageSoundManager.h"
  "Sound/RageSoundMixBuffer.h"
  "Sound/RageSoundPosMap.h"
  "Sound/RageSoundReader.h"
  "Sound/RageSoundReader_Chain.h"
  "Sound/RageSoundReader_ChannelSplit.h"
  "Sound/RageSoundReader_Extend.h"
  "Sound/RageSoundReader_FileReader.h"
  "Sound/RageSoundReader_Filter.h"
  "Sound/RageSoundReader_Merge.h"
  "Sound/RageSoundReader_Pan.h"
  "Sound/RageSoundReader_PitchChange.h"
  "Sound/RageSoundReader_PostBuffering.h"
  "Sound/RageSoundReader_Preload.h"
  "Sound/RageSoundReader_Resample_Good.h"
  "Sound/RageSoundReader_SpeedChange.h"
  "Sound/RageSoundReader_ThreadedBuffer.h"
  "Sound/RageSoundReader_WAV.h"
  "Sound/RageSoundUtil.h"
)

list(APPEND SMDATA_RAGE_SOUND_SRC "Sound/RageSoundReader_Vorbisfile.cpp")
list(APPEND SMDATA_RAGE_SOUND_HPP "Sound/RageSoundReader_Vorbisfile.h")

list(APPEND SMDATA_RAGE_SOUND_SRC "Sound/RageSoundReader_MP3.cpp")
list(APPEND SMDATA_RAGE_SOUND_HPP "Sound/RageSoundReader_MP3.h")

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

target_sources(Etterna PUBLIC ${SMDATA_ALL_RAGE_SRC})