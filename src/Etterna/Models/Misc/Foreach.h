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

/** @brief General foreach loop iterating over a deque. */
#define FOREACHD(elemType, vect, var)                                          \
                                                                               \
	for (deque<elemType>::iterator var = (vect).begin();                       \
		 (var) != (vect).end();                                                \
		 ++(var))
/** @brief General foreach loop iterating over a deque, using a constant
 * iterator. */
#define FOREACHD_CONST(elemType, vect, var)                                    \
                                                                               \
	for (deque<elemType>::const_iterator var = (vect).begin();                 \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a set. */
#define FOREACHS(elemType, vect, var)                                          \
                                                                               \
	for (set<elemType>::iterator var = (vect).begin(); (var) != (vect).end();  \
		 ++(var))
/** @brief General foreach loop iterating over a set, using a constant iterator.
 */
#define FOREACHS_CONST(elemType, vect, var)                                    \
                                                                               \
	for (set<elemType>::const_iterator var = (vect).begin();                   \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a list. */
#define FOREACHL(elemType, vect, var)                                          \
                                                                               \
	for (list<elemType>::iterator var = (vect).begin(); (var) != (vect).end(); \
		 ++(var))
/** @brief General foreach loop iterating over a list, using a constant
 * iterator. */
#define FOREACHL_CONST(elemType, vect, var)                                    \
                                                                               \
	for (list<elemType>::const_iterator var = (vect).begin();                  \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a map. */
#define FOREACHM(keyType, valType, vect, var)                                  \
                                                                               \
	for (map<keyType, valType>::iterator var = (vect).begin();                 \
		 (var) != (vect).end();                                                \
		 ++(var))
/** @brief General foreach loop iterating over a map, using a constant iterator.
 */
#define FOREACHM_CONST(keyType, valType, vect, var)                            \
                                                                               \
	for (map<keyType, valType>::const_iterator var = (vect).begin();           \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a multimap. */
#define FOREACHMM(keyType, valType, vect, var)                                 \
                                                                               \
	for (multimap<keyType, valType>::iterator var = (vect).begin();            \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over a multimap, using a constant
 * iterator. */
#define FOREACHMM_CONST(keyType, valType, vect, var)                           \
                                                                               \
	for (multimap<keyType, valType>::const_iterator var = (vect).begin();      \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over an unordered map. */
#define FOREACHUM(keyType, valType, vect, var)                                 \
                                                                               \
	for (unordered_map<keyType, valType>::iterator var = (vect).begin();       \
		 (var) != (vect).end();                                                \
		 ++(var))

/** @brief General foreach loop iterating over an unordered map. blah blah const
 * blah*/
#define FOREACHUM_CONST(keyType, valType, vect, var)                           \
                                                                               \
	for (unordered_map<keyType, valType>::const_iterator var = (vect).begin(); \
		 (var) != (vect).end();                                                \
		 ++(var))

#endif
