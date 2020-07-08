/*
 * The original MSD format is simply:
 *
 * #PARAM0:PARAM1:PARAM2:PARAM3;
 * #NEXTPARAM0:PARAM1:PARAM2:PARAM3;
 *
 * (The first field is typically an identifier, but doesn't have to be.)
 *
 * The semicolon is not optional, though if we hit a # on a new line, eg:
 * #VALUE:PARAM1
 * #VALUE2:PARAM2
 * we'll recover.
 */

#include "Etterna/Globals/global.h"
#include "MsdFile.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Utils/RageUtil.h"

void
MsdFile::AddParam(const char* buf, int len)
{
	values.back().params.push_back(std::string(buf, len));
}

void
MsdFile::AddValue() /* (no extra charge) */
{
	values.push_back(value_t());
	values.back().params.reserve(32);
}

void
MsdFile::ReadBuf(const char* buf, int len, bool bUnescape)
{
	values.reserve(64);

	auto ReadingValue = false;
	auto i = 0;
	auto* cProcessed = new char[len];
	auto iProcessedLen = -1;
	while (i < len) {
		if (i + 1 < len && buf[i] == '/' && buf[i + 1] == '/' &&
			ReadingValue == false) {
			/* Skip a comment entirely; don't copy the comment to the
			 * parameter */

			// there is no legitimate reason to allow people to place comments
			// inside value fields this is indescribably stupid -mina
			do {
				i++;
			} while (i < len && buf[i] != '\n');

			continue;
		}

		if (ReadingValue && buf[i] == '#') {
			/* Unfortunately, many of these files are missing ;'s.
			 * If we get a # when we thought we were inside a value, assume we
			 * missed the ;.  Back up and end the value. */
			// Make sure this # is the first non-whitespace character on the
			// line.
			auto FirstChar = true;
			auto j = iProcessedLen;
			while (j > 0 && cProcessed[j - 1] != '\r' &&
				   cProcessed[j - 1] != '\n') {
				if (cProcessed[j - 1] == ' ' || cProcessed[j - 1] == '\t') {
					--j;
					continue;
				}

				FirstChar = false;
				break;
			}

			// sm5 devs make code block to hand hold idiots who don't know how
			// delimiters work in the process sm5 devs show they don't know how
			// delimiters work time to handhold -mina
			if (buf[i - 1] == ':')
				FirstChar = false;

			if (!FirstChar) {
				/* We're not the first char on a line.  Treat it as if it were a
				 * normal character. */
				cProcessed[iProcessedLen++] = buf[i++];
				continue;
			}

			/* Skip newlines and whitespace before adding the value. */
			iProcessedLen = j;
			while (iProcessedLen > 0 &&
				   (cProcessed[iProcessedLen - 1] == '\r' ||
					cProcessed[iProcessedLen - 1] == '\n' ||
					cProcessed[iProcessedLen - 1] == ' ' ||
					cProcessed[iProcessedLen - 1] == '\t'))
				--iProcessedLen;

			AddParam(cProcessed, iProcessedLen);
			iProcessedLen = 0;
			ReadingValue = false;
		}

		/* # starts a new value. */
		if (!ReadingValue && buf[i] == '#') {
			AddValue();
			ReadingValue = true;
		}

		if (!ReadingValue) {
			if (bUnescape && buf[i] == '\\')
				i += 2;
			else
				++i;
			continue; /* nothing else is meaningful outside of a value */
		}

		/* : and ; end the current param, if any. */
		if (iProcessedLen != -1 && (buf[i] == ':' || buf[i] == ';'))
			AddParam(cProcessed, iProcessedLen);

		/* # and : begin new params. */
		if (buf[i] == '#' || buf[i] == ':') {
			++i;
			iProcessedLen = 0;
			continue;
		}

		/* ; ends the current value. */
		if (buf[i] == ';') {
			ReadingValue = false;
			++i;
			continue;
		}

		/* We've gone through all the control characters.  All that is left is
		 * either an escaped character, ie \#, \\, \:, etc., or a regular
		 * character. */
		if (bUnescape && i < len && buf[i] == '\\')
			++i;
		if (i < len) {
			cProcessed[iProcessedLen++] = buf[i++];
		}
	}

	/* Add any unterminated value at the very end. */
	if (ReadingValue)
		AddParam(cProcessed, iProcessedLen);

	delete[] cProcessed;
}

// returns true if successful, false otherwise
bool
MsdFile::ReadFile(const std::string& sNewPath, bool bUnescape)
{
	error = "";

	RageFile f;
	/* Open a file. */
	if (!f.Open(sNewPath)) {
		error = f.GetError();
		return false;
	}

	// allocate a string to hold the file
	std::string FileString;
	FileString.reserve(f.GetFileSize());

	auto iBytesRead = f.Read(FileString);
	if (iBytesRead == -1) {
		error = f.GetError();
		return false;
	}

	ReadBuf(FileString.c_str(), iBytesRead, bUnescape);

	return true;
}

void
MsdFile::ReadFromString(const std::string& sString, bool bUnescape)
{
	ReadBuf(sString.c_str(), sString.size(), bUnescape);
}

std::string
MsdFile::GetParam(unsigned val, unsigned par) const
{
	if (val >= GetNumValues() || par >= GetNumParams(val))
		return std::string();

	return values[val].params[par];
}
