#ifndef Foreach_H
#define Foreach_H

/** @brief General foreach loop iterating over a vector. */
#define FOREACH(elemType, vect, var)                                           \
                                                                               \
	for (vector<elemType>::iterator var = (vect).begin();                      \
		 (var) != (vect).end();                                                \
		 ++(var))
/** @brief General foreach loop iterating over a vector, using a constant
 * iterator. */
#define FOREACH_CONST(elemType, vect, var)                                     \
                                                                               \
	for (vector<elemType>::const_iterator var = (vect).begin();                \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a set, using a constant iterator.
 */
#define FOREACHS_CONST(elemType, vect, var)                                    \
                                                                               \
	for (set<elemType>::const_iterator var = (vect).begin();                   \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a map, using a constant iterator.
 */
#define FOREACHM_CONST(keyType, valType, vect, var)                            \
                                                                               \
	for (map<keyType, valType>::const_iterator var = (vect).begin();           \
		 (var) != (vect).end();                                                \
		 ++(var))

#endif

/**
 * @file
 * @author Chris Danford (c) 2004-2005
 * @section LICENSE
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
