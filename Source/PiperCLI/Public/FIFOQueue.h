#pragma once

#include "CoreMinimal.h"

/**
 * Byte FIFO queue backed by a single TArray<uint8>.
 * - Feed(): append bytes
 * - Peek(): copy out up to N bytes without consuming (clamped)
 * - Take(): copy out up to N bytes and consume (clamped)
 * - Skip(): consume up to N bytes (clamped)
 * - PeekView(): zero-copy view of the next K contiguous bytes
 *
 * Data is contiguous from Head .. Buffer.Num()-1.
 * We compact lazily when Head grows large, or becomes equal to Num.
 */
class FByteFIFOQueue
{
public:
	FByteFIFOQueue();

	/** Append bytes from a pointer. */
	void Feed(const uint8* Data, int32 Len);

	/** Append bytes from an array (copy). */
	void Feed(const TArray<uint8>& Data);

	/** Move-append: if queue is empty, take ownership; otherwise copy-append. */
	void Feed(TArray<uint8>&& Data);

	/** Number of available bytes. */
	int32 Length() const;

	bool IsEmpty() const;
	bool Has(int32 N) const;

	/** Copy out up to N bytes without consuming (clamped). */
	TArray<uint8> Peek(int32 N) const;

	/** Zero-copy view of the next K contiguous bytes (clamped). */
	TArrayView<const uint8> PeekView(int32 K) const;

	/** Copy out up to N bytes and consume them (clamped). */
	TArray<uint8> Take(int32 N);

	/** Consume up to N bytes (clamped). */
	void Skip(int32 N);

	/** Pointer to contiguous unread bytes; length is ContiguousLength(). */
	const uint8* DataPtr() const;

	/** Number of contiguous unread bytes from DataPtr(). */
	int32 ContiguousLength() const;

	/** Clear the queue. */
	void Clear();

private:
	void Advance(int32 Count);
	void CompactIfNeeded();

private:
	TArray<uint8> Buffer;
	int32 Head;
};