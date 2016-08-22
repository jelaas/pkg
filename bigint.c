#include <stdlib.h>
#include <limits.h>
#include "bigint.h"

int bigint_load(struct bigint *b, char *v, int v_len, int negative)
{
	b->v = v;
	b->n = v_len;
	b->neg = negative;
	return 0;
}

int bigint_loadi(struct bigint *b, char *v, int v_len, int intval)
{
	int i;

	bigint_load(b, v, v_len, 0);
	bigint_zero(b);
	b->neg = intval < 0;
	intval = abs(intval);
	
	for(i=0;i<v_len; i++) {
		if(i >= v_len) return -1;
		v[i] = intval & 0xf;
		intval >>= 4;
		if(intval == 0) break;
	}
	return 0;
}

/* swap values (and underlaying vectors) of i1 and i2 */
int bigint_swap(struct bigint *i1, struct bigint *i2)
{
	int n, neg;
	char *v;
	
	n = i1->n;
	v = i1->v;
	neg = i1->neg;

	i1->n = i2->n;
	i1->v = i2->v;
	i1->neg = i2->neg;

	i2->n = n;
	i2->v = v;
	i2->neg = neg;

	return 0;
}


int bigint_zero(struct bigint *b)
{
	int i;
	for (i = 0; i < b->n; i++)
		b->v[i] = 0;
	b->neg = 0;
	return 0;
}

int bigint_iszero(struct bigint *b)
{
	int i;
	for (i = 0; i < b->n; i++)
		if(b->v[i]) return 0;
	return 1;
}

static int _bigint_add(struct bigint *res, const struct bigint *i1, const struct bigint *i2, int res_sign)
{
	int carry = 0;
	int i;
	int t, t1, t2;
	int maxlen;

	maxlen = i1->n > i2->n ? i1->n : i2->n;
	
	for (i = 0; i < maxlen; i++) {
		t1 = i < i1->n ? i1->v[i] : 0;
		t2 = i < i2->n ? i2->v[i] : 0;
		t = (t1 + t2 + carry) & 0xf;
		if(i < res->n)
			res->v[i] = t & 0xf;
		else {
			if(t1 + t2 + carry) return -1;
		}
		carry = (t1 + t2 + carry) >> 4;
	}
	if (carry) {
		if( i >= res->n) return -1;
		res->v[i] = carry;
	}
	res->neg = res_sign;
	return 0;
}

/* subtract abs(i2) from abs(i1) with resulting sign = res_sign */
int _bigint_sub(struct bigint *res, const struct bigint *i1, const struct bigint *i2, int res_sign)
{
	int borrow = 0;
	int i;

	int t, t1, t2;
	int maxlen;

	maxlen = i1->n > i2->n ? i1->n : i2->n;
	
	for (i = 0; i < maxlen; i++) {
		t1 = i < i1->n ? i1->v[i] : 0;
		t2 = i < i2->n ? i2->v[i] : 0;
		t = t1 - t2 - borrow;
		borrow = 0;
		if(t < 0) {
			borrow = 1;
			t += 0x10;
		}
		if(i < res->n)
			res->v[i] = t;
		else {
			if(t) return -1;
		}
	}
	res->neg = res_sign;
	return 0;
}


int bigint_sum(struct bigint *res, const struct bigint *i1, const struct bigint *i2)
{
	const struct bigint *t;

	if( i1->neg == i2->neg )
		return _bigint_add(res, i1, i2, i1->neg);

	{
		int s;
		
		/* make i2 the negative bigint */
		if( i1->neg) {
			t = i2;
			i2 = i1;
			i1 = t;
		}
		
		s = 0;

		{
			int i, maxlen;
			int t1, t2;
			
			/* if abs(i2) > abs(i1) then result will be negative. so switch places and mark result as negative */
			maxlen = i1->n > i2->n ? i1->n : i2->n;
			for(i=maxlen-1;i>=0;i--) {
				t1 = i < i1->n ? i1->v[i] : 0;
				t2 = i < i2->n ? i2->v[i] : 0;
				if(t2 > t1) {
					s = 1;
					t = i2;
					i2 = i1;
					i1 = t;
					break;
				}
				if(t1 > t2) break;
			}
		}

		/* subtract i2 from i1 */
		return _bigint_sub(res, i1, i2, s);
	}
}

static char _hexdigit[] = "0123456789abcdef";
int bigint_tostr(char *s, int s_len, const struct bigint *b)
{
	int i;
	int j = 0;
	
	if(j >= s_len) return -1;
	if(b->neg) s[j++] = '-';
	for(i=b->n-1;i>=1;) {
		if(b->v[i]) break;
		i--;
	}
	for(;i>=0;i--) {
		if(j >= s_len) return -1;
		s[j++] = _hexdigit[(int)b->v[i]];
	}
	if(j >= s_len) return -1;
	s[j] = 0;
	return 0;
}

int bigint_toint(int *res, const struct bigint *b)
{
	int i;
	*res = 0;
	
	for(i=b->n-1;i>=0;i--) {
		if(*res > (INT_MAX >> 4)) return -1;
		*res = (*res) << 4;
		*res += b->v[i];
	}
	if(b->neg) *res = 0 - *res;
	return 0;
}

int bigint_bits(const struct bigint *b)
{
	int i, j, t;

	for(i=b->n-1;i>=0;i--) {
		if(b->v[i]) {
			int bits = 0;
			t = b->v[i];
			for(j=0;j<4;j++) {
				if(t) bits++;
				t >>= 1;
			}
			return i*4+bits;
		}
	}
	return 0;
}
