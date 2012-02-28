/** \file
 * \brief Implementation of BinDiff class and its companions.
 */
#include "BinDiff.h"
#include <string.h>

void BinDiff::put(const unsigned char *dst, patchsize_t dstsize){
	while(dstsize){
		if(src + size <= cur){
			if(patchClosed){
				Patch pa;
				pa.start = patchsize_t(cur - src);
				pa.size = patchsize_t(cur - srcdiff);
				patches.push_back(pa);
				patchClosed = false;
			}
			patches.back().buf.push_back(*dst);
		}
		else if(*cur == *dst && srcdiff && sizeof(patchsize_t) * 3 < cur - srcdiff){
			patchClosed = true;
			srcdiff = NULL;
		}
		else if(*cur != *dst || srcdiff){
			if(*cur != *dst)
				srcdiff = cur;
			if(patchClosed){
				Patch pa;
				pa.start = patchsize_t(cur - src);
				pa.size = 0;
				patches.push_back(pa);
				patchClosed = false;
			}
			patches.back().size++;
			patches.back().buf.push_back(*dst);
		}
		cur++;
		dst++;
		dstsize--;
	}
}

void BinDiff::close(){
	if(cur - src < size){
		Patch pa;
		pa.start = patchsize_t(cur - src);
		pa.size = patchsize_t(size - (cur - src));
		patches.push_back(pa);
		patchClosed = true;
	}
	else
		patchClosed = true;
}

void applyPatches(std::vector<unsigned char> &result, const std::list<Patch> &patches){
	// You must apply the patches in reverse order of creation because the former patches could change the offset of the latter patches.
	std::list<Patch>::const_reverse_iterator it = patches.rbegin();
	for(; it != patches.rend(); it++){
		std::vector<unsigned char> after;
		bool copyAfter = false;
		if(it->size != it->buf.size() && it->start + it->size < result.size()){
			copyAfter = true;
			after.resize(result.size() - (it->start + it->size));
			memcpy(&after.front(), &result[it->start + it->size], after.size());
		}
		result.resize(result.size() + it->buf.size() - it->size);
		if(0 < it->buf.size())
			memcpy(&result[it->start], &it->buf.front(), it->buf.size());
		if(copyAfter)
			memcpy(&result[it->start + it->buf.size()], &after.front(), after.size());
	}
}
