#include "clib/avec3.h"
#include "clib/aquatrot.h"
#include "clib/amat4.h"
#include "clib/c.h"
#include "clib/mathdef.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <float.h>


#define MAX_CIRCUT_ARRAY 256

/* Circle cuts
    Binary Tree vs. Pointer Array
  Binary Tree is fairly fast, but it gets slower when plenty of entries are added by O(log(N)).
  For very frequent usage, array indexing can be faster I think.
  But arrays can grow really large in memory, which is expected to be somewhat sparse.
  So I combined those algorithms.
  Entries less than MAX_CIRCUT_ARRAY is indexed in the array, otherwise searched from the binary tree.
 */
typedef struct circut{
	int n, m;
	struct circut *lo, *hi;
	double d[1][2];
} circut;
static circut **circut_array = NULL, *circut_root = NULL;
static int circut_num = 0;

double (*CircleCuts(int n))[2]{
	circut **node;
	if(n <= 0) return NULL;
	if(n < MAX_CIRCUT_ARRAY){
		if(circut_num <= n){
			circut_array = realloc(circut_array, (n + 1) * sizeof *circut_array);
			memset(&circut_array[circut_num], 0, (&circut_array[n + 1] - &circut_array[circut_num]) * sizeof *circut_array);
			circut_num = n + 1;
		}
		node = &circut_array[n];
	}
	else for(node = &circut_root; *node && (*node)->n != n; node = n < (*node)->n ? &(*node)->hi : &(*node)->lo);
	if(!*node){
		int i;
		*node = malloc(offsetof(circut, d) + n * sizeof *(*node)->d);
		(*node)->n = (*node)->m = n;
		(*node)->hi = (*node)->lo = NULL;
		for(i = 0; i < n; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	else if((*node)->m < n){
		int i;
		*node = realloc(*node, offsetof(circut, d) + n * sizeof *(*node)->d);
		(*node)->n = (*node)->m = n;
/*		(*node)->hi = (*node)->lo = NULL;*/
		for(i = 0; i < n; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	return (*node)->d;
}

double (*CircleCutsPartial(int n, int nmax))[2]{
	static circut *root = NULL;
	circut **node;
	if(n <= 0 || n < nmax) return NULL;
	if(n < MAX_CIRCUT_ARRAY){
		if(circut_num <= n){
			circut_array = realloc(circut_array, (n + 1) * sizeof *circut_array);
			memset(&circut_array[circut_num], 0, (&circut_array[n + 1] - &circut_array[circut_num]) * sizeof *circut_array);
			circut_num = n + 1;
		}
		node = &circut_array[n];
	}
	else for(node = &circut_root; *node && (*node)->n != n; node = n < (*node)->n ? &(*node)->hi : &(*node)->lo);
	if(!*node){
		int i;
		*node = malloc(offsetof(circut, d) + nmax * sizeof *(*node)->d);
		(*node)->n = n;
		(*node)->m = nmax;
		(*node)->hi = (*node)->lo = NULL;
		for(i = 0; i < nmax; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	else if((*node)->m < nmax){
		int i;
		*node = realloc(*node, offsetof(circut, d) + nmax * sizeof *(*node)->d);
		(*node)->n = n;
		(*node)->m = nmax;
/*		(*node)->hi = (*node)->lo = NULL;*/
		for(i = 0; i < nmax; i++){
			(*node)->d[i][0] = sin(i * 2. * M_PI / n);
			(*node)->d[i][1] = cos(i * 2. * M_PI / n);
		}
	}
	return (*node)->d;
}

static size_t circuts_memory(circut *root){
	size_t ret;
	if(!root)
		return 0;
	ret = offsetof(circut, d) + root->m * sizeof *root->d;
	ret += circuts_memory(root->lo);
	ret += circuts_memory(root->hi);
	return ret;
}

size_t CircleCutsMemory(void){
	int i;
	size_t ret;
	ret = circut_num * sizeof *circut_array;
	for(i = 0; i < circut_num; i++) if(circut_array[i])
		ret += offsetof(circut, d) + circut_array[i]->m * sizeof *circut_array[i]->d;
	ret += circuts_memory(circut_root);
	return ret;
}
