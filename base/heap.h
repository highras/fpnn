#ifndef HEAP_H_
#define HEAP_H_ 1

/* This heap implementation use zero-based indexing array.
 * It's a max heap, ie, heap[0] is always the largest element.
 * After HEAP_SORT(), the array is sorted, and heap[0] is the smallest.
 * If you want a min heap and sort the array in descending order, 
 * you should define the LESS(heap, i1, i2) function (or macro) so that
 * it returns true when (a>b).
 */


#define HEAP_FIX_UP(h, k, LESS, SWAP) 	do {				\
	register int _i__, _k__ = (k);					\
	for (; _k__>0 && (_i__ = (_k__ - 1) / 2, LESS((h), _i__, _k__)); _k__ = _i__)	\
		SWAP((h), _k__, _i__);					\
} while(0)


#define HEAP_FIX_DOWN(h, k, N, LESS, SWAP)  do {			\
	register int _i__, _k__ = (k), _n__ = (N);			\
	for (; _i__ = 2 * _k__ + 1, _i__ < _n__; _k__ = _i__) {		\
		if (_i__ < _n__ - 1 && LESS((h), _i__, (_i__ + 1))) ++_i__;	\
		if (LESS((h), _k__, _i__)) SWAP((h), _k__, _i__);	\
		else break;						\
	}								\
} while(0)


#define HEAP_FIX_UPDOWN(h, k, N, LESS, SWAP) do {			\
	register int _i__, _k__ = (k), _n__ = (N);			\
	int _saved_k__ = _k__;						\
	for (; _k__ > 0 && (_i__ = (_k__ - 1) / 2, LESS((h), _i__, _k__)); _k__ = _i__)	\
		SWAP((h), _k__, _i__);					\
	if (_k__ == _saved_k__) {					\
		for (; _i__ = 2 * _k__ + 1, _i__ < _n__; _k__ = _i__) {	\
			if (_i__ < _n__ - 1 && LESS((h), _i__, (_i__ + 1))) ++_i__;	\
			if (LESS((h), _k__, _i__)) SWAP((h), _k__, _i__);	\
			else break;					\
		}							\
	}								\
} while(0)


#define HEAP_PUSH(h, N, e, LESS, SWAP)	do {				\
	(h)[N] = e;							\
	HEAP_FIX_UP((h), N, LESS, SWAP);				\
	++N;								\
} while(0)


#define HEAP_POP(h, N, e, LESS, SWAP)  do {				\
	--N;								\
	SWAP((h), 0, N);						\
	HEAP_FIX_DOWN((h), 0, N, LESS, SWAP);				\
	e = (h)[N];							\
} while(0)


#define HEAP_PUT(h, k, N, e, LESS, SWAP) do {				\
	(h)[k] = e;							\
	HEAP_FIX_UPDOWN((h), k, N, LESS, SWAP);				\
} while(0)


/* NB: *h* must be a heap. That is, *h* must constructed by HEAP_PUSH()
 * or by HEAPIFY().
 */
#define HEAP_SORT(h, NN, LESS, SWAP)	do {				\
	register int _nn__ = (NN);					\
	while (_nn__ > 1) {						\
		--_nn__;						\
		SWAP((h), 0, _nn__);					\
		HEAP_FIX_DOWN((h), 0, _nn__, LESS, SWAP);		\
	}								\
} while(0)


/* Heapify an array in place */
#define HEAPIFY(a, NN, LESS, SWAP) 	do {				\
	register int _kk__, _nn__ = (NN);				\
	for (_kk__ = (_nn__-2)/2; _kk__ >= 0; --_kk__)			\
		HEAP_FIX_DOWN((a), _kk__, _nn__, LESS, SWAP);		\
} while(0)


#define BSEARCH_POS(a, N, key, r, p, CMP)	do {			\
	int _l__, _u__, _i__, _r__, _comp__;				\
	_l__ = 0; _u__ = (N); _r__ = -1;				\
	while (_l__ < _u__) { 						\
		_i__ = (_l__ + _u__) / 2;				\
		_comp__ = CMP(key, (a)[_i__]);				\
		if (_comp__ < 0) _u__ = _i__;				\
		else if (_comp__ > 0) _l__ = _i__ + 1;			\
		else { _r__ = _i__; break; }				\
	}								\
	(r) = _r__;							\
	(p) = _r__ >= 0 ? _r__ : _l__;					\
} while(0)


#define BSEARCH(a, N, key, r, CMP)	do {				\
	int _p__;							\
	BSEARCH_POS((a), N, key, r, _p__, CMP);				\
} while(0)


#endif

