#pragma once
#include <vector>

#include "Common.h"

template<typename Type>
union CanvasSize {
	Type S[2];

	struct { 
		Type X;
		Type Y;
	};
};


template<typename CanvasType, typename IndexType=int8_t, typename SizeType=size_t>
class Canvas {
public:
	std::vector<CanvasType*> Buffers = {};
	CanvasSize<SizeType> Size = { 0 };
	IndexType Current = 0;

	FORCE_INLINE SizeType BufferSize() {
		return this->Size.S[0] * this->Size.S[1];
	}
	FORCE_INLINE void NextBuffer(IndexType Offset) {
		this->IndexType = (this->Current + Offset) % this->Buffers.size();
	}
	FORCE_INLINE CanvasType* Buffer() {
		return this->Buffers[this->Current];
	}

	void Allocate(uint8_t BufferCount) {
		SizeType Size = this->BufferSize();
		for (uint8_t i = 0; i < BufferCount; i++) this->Buffers.push_back(new CanvasType[Size]);
	}

	void Deallocate(uint8_t BufferCount) {
		for (uint8_t i = 0; i < BufferCount; i++) delete[] this->Buffers.pop_back();
	}
};