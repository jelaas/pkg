#define INTBITS 32

struct bigint {
  int n;
  int *v;
};

void bigint_init(struct bigint *i, int n, int *v);
void bigint_loadi(struct bigint *i, int value);
int bigint_addi(struct bigint *i, int value);
int bigint_subi(struct bigint *i, int value);
void bigint_cmpi(struct bigint *i, int value);
