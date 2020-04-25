number |= 1 << x;
That will set bit x.

Clearing a bit
Use the bitwise AND operator (&) to clear a bit.

number &= ~(1 << x);
That will clear bit x. You must invert the bit string with the bitwise NOT operator (~), then AND it.

Toggling a bit
The XOR operator (^) can be used to toggle a bit.
number ^= 1 << x;
That will toggle bit x.

Checking a bit
To check a bit, AND it with the bit you want to check:
bit = number & (1 << x);