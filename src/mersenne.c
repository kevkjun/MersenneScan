#include<stdio.h>
#include<stdbool.h>
#include<string.h>

#define MAX_DIGITS 350

// We will store Big integers as an array of integers, with each
// integer's value limited to base 10. That is, each digit of a large
// integer value is stored separately in its own integer in the array.
// The values are stored in little endian order.
// E.g., a value of "7360" would be stored in an array as:
// int big_val[4] = {0, 6, 3, 7};
// There are of course more efficient representations, but we will
// sacrifice some efficiency here to make our code as simple as
// possible for when we convert to assembly.
typedef struct{
	int n;                  // Number of active digits stored
	int digits[MAX_DIGITS]; // Stores the digits in little endian order
} Bigint;

// Checks for leading zeros and reduces the number of active digits.
// This is used frequently after a matematical operation is performed.
// For instance, if we perform the operation C = A - B, where:
// A = 7500
// B = 7499
// then we will get C = 0001
// For the sake of efficiency in later computations made with this
// value, we scan it and truncate so as to remove the leading zeros.
// We could remove the need for this function by making sure each
// arithmetic operation outputs big integers of the correct size,
// but that involves adding some additional logic, and we want to
// keep our functions as simple as possible when converting to assembly.
void compress(Bigint * a)
{
	for( int i = a->n - 1; i >= 0; i-- )
	{
		if( a->digits[i] == 0 && i != 0 )
			a->n -= 1;
		else
			return;
	}
}

// Converts a single digit integer (0,9) to a big Int
// We limit the functionality of this function so as
// to make it as simple as possible while still meeting
// our program's needs
Bigint digit_to_big(int a)
{
	Bigint b;
	b.n = 1;
	b.digits[0] = a;
	return b;
}

// Prints out the big integer to stdout. Descending
// order is used as we have stored the big integer
// in little endian format.
void print_big(Bigint b)
{
	for( int c = b.n -1; c >= 0; c--)
		printf("%d", b.digits[c]);
	printf("\n");
}

// Computes c = a * b
// Based on this algorithm by Matthew Crumley:
// https://silentmatt.com/blog/2011/10/how-bigintegers-work-part-2-multiplication/
// which has javascript source code available here:
// https://github.com/silentmatt/javascript-biginteger/blob/master/biginteger.js
Bigint mult_big(Bigint a, Bigint b)
{
	Bigint c;

	// c can have (at most) the number of digits in a and b, added
	c.n = a.n + b.n;

	// Initialize all digits in c to zero
	for( int i = 0; i < c.n; i++ )
		c.digits[i] = 0;

	// Perform basic multiplication, with some more efficient indexing
	for( int i = 0; i < b.n; i++ )
	{
		int carry = 0;
		int j;
		for( j = i; j < a.n + i; j++ )
		{
			int val = c.digits[j] + (b.digits[i] * a.digits[j-i]) + carry;
			carry       = val / 10;
			c.digits[j] = val % 10;
		}
		if( carry > 0 )
		{
			int val = c.digits[j] + carry;
			carry       = val / 10;
			c.digits[j] = val % 10;
		}
	}

	// Trim any leading zeros
	compress(&c);

	return c;
}

// Computes c = a - b
// Simple elementary subtraction. Be sure that
// the return value (c) has been compressed so it
// is appropriately sized
Bigint sub_big(Bigint a, Bigint b)
{
	Bigint c;

	// Set number of active digits to a.n (b/c a >= b) b/c # of digits is at most # of digits of max(a,b)
	c.n = a.n;

	// // boolean to see if b's digit is larger than a's digit
	// bool largerThan = false;

	// Check if largerThan is true. If so, the current digit should be one less (value was borrowed)
	// Also need to check if a's index is greater than or equal to number of digits in b - if so, can bring
	// a's digit to c (as if there are leading zeroes in b).

	// Create j index and aDigit holder value
	int j, aDigit;

	/*
	Loop starting at j. Initially, aDigit is the jth index of a (normal). 
	If largerThan (i.e. a's previous index's digit was smaller than b's and required a borrow), decrement aDigit and reset boolean. 
	If b's j index is greater than a's, add ten to aDigit (borrow from next index) and set bool = true.
	Set c's j index.
	If there are still numbers not iterated in a's digits, just bring them down to c as if b had 
	leading zeroes.
	*/
	// This algorithm assumes that running over b.digits[] into 0s is okay/expected
	for (j = 0; j < b.n; j++) {
		aDigit = a.digits[j];
		// if (largerThan) {
		// 	aDigit--;
		// 	largerThan = false;
		// }
		if (b.digits[j] > aDigit) {
			aDigit += 10;
			//largerThan = true;
			a.digits[j+1]--;
		}
		c.digits[j] = aDigit - b.digits[j];
	}

	// If there are still digits that we haven't iterated through
	while (j < a.n) {
		if (a.digits[j] < 0) {
			a.digits[j] += 10;
			a.digits[j+1]--;
		}
		c.digits[j] = a.digits[j];
		j++;
	}

	// // In the case that we borrowed from the largest digit of a
	// if (largerThan) {
	// 	c.digits[j] = a.digits[j] - 1;
	// 	j++;
	// }

	// // If there are still digits that we haven't iterated through
	// while (j < a.n) {
	// 	if (a.digits[j] < 0) {
	// 		a.digits[j] += 10;
	// 		a.digits[j+1]--;
	// 	}
	// 	c.digits[j] = a.digits[j];
	// 	j++;
	// }

	compress(&c);

	return c;
}

// Computes b = a^p
// Works by multiplying 'a' by itself p-1 times
Bigint pow_big(Bigint a, int p )
{
	Bigint b = a;

	// Just multiply by itself p-1 times
	for( int i = 1; i < p; i++ )
		b = mult_big(b, a);

	return b;
}

// compares a and b
// Assumes that a and b have been correctly "compressed", in that they
// are the right length and do not contain leading zeros.
// returns -1 if a < b
// returns 0 if a == b
// returns 1 if a > b
int compare_big(Bigint a, Bigint b)
{
	// if a has fewer digits than b, its definitely smaller
	if( a.n < b.n )
		return -1;

	// if a has more digits than b, its definitely larger
	if( a.n > b.n )
		return 1;

	// otherwise, if they have the same number of digits
	// we have to scan through them and check. We compare
	// in descending order, as we want to look at the highest
	// value digits first.
	for( int i = a.n - 1; i >= 0; i-- )
	{
		if(      a.digits[i] > b.digits[i] )
			return 1;
		else if( a.digits[i] < b.digits[i] )
			return -1;
	}

	// if we got to here, we are equal
	return 0;
}

// Multiplies by 10
// Since we have stored our big int value in digit-wise form,
// we only need to shift our integer array to the right once.
void shift_right(Bigint * a )
{
	// Copy stuff
	for( int i = a->n; i > 0; i-- )
		a->digits[i] = a->digits[i-1];

	// Set lowest digit to 0
	a->digits[0] = 0;

	// Set to new larger size
	a->n += 1;
}

// Divides by 10
// Since we hvae stored our big int value in digit-wise form,
// we only need to shift our integer array to the left once.

// Idea: Set the i index equal to i+1 index then set the digit size equal to smaller size
void shift_left(Bigint * a )
{	
	// need base case if n == 0 b/c below for loop would not work. would return immediately
	// technically not necessary because a >= 10
	if (a->n == 0) return;

	// for loop needs to be less than one less than number of digits b/c we're looking forward one index
	for (int i = 0; i < (a->n)-1; i++) {
		a->digits[i] = a->digits[i+1];
	}
	a->digits[(a->n) - 1] = 0;
	a->n -= 1;
}

// Computes c = a % b
/* Algorithm based in spirit on response 3 to this post:
 * https://stackoverflow.com/questions/980702/efficient-cycles-wise-algorithm-to-compute-modulo-25
 * The basic idea though is that this is an improved version of a basic subtraction algorithm for mod.
 * In the basic algorithm, we just keep subtracting the denominator from the numerator until
 * the numerator is less than the denominator, at which point the numerator is our solution.
 * In this improved version, instead of subtracting off the denominator, we subtract off the
 * largest possible 10x multiple of the denonomiator given the state of the numerator. E.g., if the
 * numerator was 40,000 and the denominator was 30, we would multiply 30 by 10 to get 300, then again
 * and again until we reach 300,000, detect that this is too large, then back off one order of magnitude
 * to arrive at a denominator of 30,000. We then subtract this off, to get a new numerator of
 * 10,000, and re-adjust our denominator downwards, repeating the process until the denominator returns
 * to its original size. There are of course much more efficient methods that can be easily imagined,
 * but this one can be done in only a few lines of code while remaining efficient enough for
 * our purposes.
 */
Bigint mod_big(Bigint a, Bigint b)
{
	// Store original denominator for use later
	Bigint original_b = b;

	// Keep multiplying the denominator (b) by 10 until it is larger than the numerator (a)
	while( compare_big(a, b) == 1 ) // tests if a > b
		shift_right(&b);

	// We went too far, so divide once by 10
	shift_left(&b);

	// At this point, we have the largest possible multiple of the denominator
	// without being larger than the numerator.

	// Keep reducing size of denominator by factor of 10 until it equals its original size
	while( compare_big(b,original_b) != -1 ) // tests if b >= original_b
	{
		// Keep subtracting off denominator from numerator (A)
		while( compare_big(a,b) != -1 ) // tests if a >= b
			a = sub_big(a,b);

		// Once we've gone as far as we can go, reduce size of denominator
		shift_left(&b);
	}

	return a;
}

// The Lucas–Lehmer primality test (LLT) algorithm
// Tests if Mp = 2^p - 1 is a prime number
// Returns 1 if Mp is prime, 0 otherwise
int LLT(int p)
{
	// Some values we'll use
	Bigint zero = digit_to_big(0);
	Bigint one  = digit_to_big(1);
	Bigint two  = digit_to_big(2);

	// Mp = 2^p - 1
	Bigint Mp = pow_big(two, p);
	Mp =  sub_big(Mp, one);

	// s = 4
	Bigint s = digit_to_big(4);

	for( int i = 0; i < p - 2; i++ )
	{
		// s = ((s × s) − 2) mod Mp
		s = mult_big(s, s);
		s = sub_big(s, two);
		s = mod_big(s, Mp);
	}

	if( compare_big(s, zero) == 0 ) // check if s == 0
		return 1; // PRIME
	else
		return 0; // NOT_PRIME
}

// Naive algorithm for checking primacy of a regular integer
// We only use this function for testing if the small "p" values
// are primes themselves, as Mp = 2^p - 1 will not be prime if
// p is not also prime.
// Returns 1 if p is prime, 0 otherwise
int is_small_prime(int p)
{
	for( int i = 2; i < p-1; i++ )
		if( p % i == 0 )
			return 0;
	return 1;
}
/*
// Main function to test functions
int main(void) 
{
	Bigint two = digit_to_big(2);
	Bigint one = digit_to_big(1);
	print_big(sub_big(two, one));

	Bigint eight = digit_to_big(8);
	Bigint sixteen = mult_big(two, eight);
	printf("%i", sixteen.n);
	printf("\n");

	print_big(sub_big(sixteen, one));
	print_big(sub_big(sixteen, eight));

	shift_left(&sixteen);
	print_big(sixteen);

	Bigint zero = digit_to_big(0);
	shift_left(&zero);
	print_big(zero);

	shift_left(&one);
	print_big(one);

	Bigint nine = digit_to_big(9);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	shift_right(&nine);
	Bigint seven = digit_to_big(7);
	shift_right(&seven);
	seven.digits[0] = 6;
	shift_right(&seven);
	seven.digits[0] = 5;
	shift_right(&seven);
	seven.digits[0] = 4;
	shift_right(&seven);
	seven.digits[0] = 3;
	shift_right(&seven);
	seven.digits[0] = 2;
	shift_right(&seven);
	seven.digits[0] = 1;
	shift_left(&seven);
	shift_left(&seven);
	shift_left(&seven);
	print_big(nine);
	print_big(seven);

	Bigint four = digit_to_big(4);
	Bigint two = digit_to_big(2);
	Bigint four_squared = mult_big(four, four);
	print_big(four_squared);
	print_big(sub_big(four_squared, two));
}
*/

// Scan through to p = 550, checking for prime Mp's along the way
int main(void) {
	// Test all p values, 2 to 550
	for( int p = 2; p < 550; p++ ) {
		// Only test Mp for primacy if p itself is also prime
		if( is_small_prime(p) ) {
			printf("Testing p = %d ", p);
            int is_odd = p % 2;
			
            if (is_odd) { // Run LLT test of Mp
				int is_prime = LLT(p);

				if(is_prime) {
					printf("found prime Mp = ");
					Bigint one  = digit_to_big(1);
					Bigint two  = digit_to_big(2);

					// Mp = 2^p - 1
					Bigint Mp = pow_big(two, p);
					Mp =  sub_big(Mp, one);
					print_big(Mp);
				}
				else
					printf("Mp not prime\n");
            }
            // LLT is only valid for odd primes
            // 2 is the only even prime
            else
            	printf("found prime p = 2\n");
		}
	}
	return 0;
}