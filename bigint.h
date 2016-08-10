#ifndef BIGINT_H
#define BIGINT_H

struct bigint {
  char *v;
  int n;
  int neg;
};

int bigint_load(struct bigint *b, char *v, int v_len, int negative); /* v is a vector of digits with integer values from 0 to 15. From least to most significant */
int bigint_sum(struct bigint *res, const struct bigint *i1, const struct bigint *i2); /* res must be zero before call. (load zero vector or call bigint_zero) */
int bigint_zero(struct bigint *b);
int bigint_tostr(char *s, int s_len, const struct bigint *);
int bigint_toint(int *i, const struct bigint *);

#endif
