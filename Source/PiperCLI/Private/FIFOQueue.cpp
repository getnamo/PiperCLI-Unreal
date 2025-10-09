#include "FIFOQueue.h"

FByteFIFOQueue::FByteFIFOQueue()
	: Head(0)
{
}

void FByteFIFOQueue::Feed(const uint8* Data, int32 Len)
{
	if (!Data || Len <= 0) return;
	const int32 OldNum = Buffer.Num();
	Buffer.AddUninitialized(Len);
	FMemory::Memcpy(Buffer.GetData() + OldNum, Data, Len);
}

void FByteFIFOQueue::Feed(const TArray<uint8>& Data)
{
	Feed(Data.GetData(), Data.Num());
}

void FByteFIFOQueue::Feed(TArray<uint8>&& Data)
{
	if (Data.Num() <= 0) return;
	if (IsEmpty())
	{
		Buffer = MoveTemp(Data);
		Head = 0;
	}
	else
	{
		Feed(Data.GetData(), Data.Num());
	}
}

int32 FByteFIFOQueue::Length() const
{
	return Buffer.Num() - Head;
}

bool FByteFIFOQueue::IsEmpty() const
{
	return Length() == 0;
}

bool FByteFIFOQueue::Has(int32 N) const
{
	return Length() >= N;
}

TArray<uint8> FByteFIFOQueue::Peek(int32 N) const
{
	const int32 Avail = Length();
	if (N > Avail) N = Avail;
	TArray<uint8> Out;
	Out.SetNumUninitialized(N);
	if (N > 0)
	{
		FMemory::Memcpy(Out.GetData(), Buffer.GetData() + Head, N);
	}
	return Out;
}

TArrayView<const uint8> FByteFIFOQueue::PeekView(int32 K) const
{
	const int32 Avail = Length();
	if (K > Avail) K = Avail;
	return TArrayView<const uint8>(Buffer.GetData() + Head, K);
}

TArray<uint8> FByteFIFOQueue::Take(int32 N)
{
	const int32 Avail = Length();
	if (N > Avail) N = Avail;
	TArray<uint8> Out;
	Out.SetNumUninitialized(N);
	if (N > 0)
	{
		FMemory::Memcpy(Out.GetData(), Buffer.GetData() + Head, N);
		Advance(N);
	}
	return Out;
}

void FByteFIFOQueue::Skip(int32 N)
{
	const int32 Avail = Length();
	if (N > Avail) N = Avail;
	if (N > 0) Advance(N);
}

const uint8* FByteFIFOQueue::DataPtr() const
{
	return Buffer.GetData() + Head;
}

int32 FByteFIFOQueue::ContiguousLength() const
{
	return Length();
}

void FByteFIFOQueue::Clear()
{
	Buffer.Reset();
	Head = 0;
}

void FByteFIFOQueue::Advance(int32 Count)
{
	check(Count >= 0 && Count <= Length());
	Head += Count;
	CompactIfNeeded();
}

void FByteFIFOQueue::CompactIfNeeded()
{
	const int32 Num = Buffer.Num();
	if (Head == Num)
	{
		// fully consumed
		Buffer.Reset();
		Head = 0;
		return;
	}

	constexpr int32 CompactThresholdBytes = 64 * 1024;
	const int32 Remaining = Num - Head;
	const bool bHeadBig = Head >= CompactThresholdBytes;
	const bool bHeadHalf = Head * 2 >= Num;

	if (bHeadBig || bHeadHalf)
	{
		if (Remaining > 0)
		{
			FMemory::Memmove(Buffer.GetData(), Buffer.GetData() + Head, Remaining);
		}
		Buffer.SetNum(Remaining, EAllowShrinking::No);
		Head = 0;
	}
}