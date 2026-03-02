
typedef unsigned char int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

template<class T> int8 __SETS__(T x)
{
  if ( sizeof(T) == 1 )
    return int8(x) < 0;
  if ( sizeof(T) == 2 )
    return int16(x) < 0;
  if ( sizeof(T) == 4 )
    return int32(x) < 0;
  return int64(x) < 0;
}

template<class T, class U> int8 __OFADD__(T x, U y)
{
  if ( sizeof(T) < sizeof(U) )
  {
    U x2 = x;
    int8 sx = __SETS__(x2);
    return ((1 ^ sx) ^ __SETS__(y)) & (sx ^ __SETS__(x2+y));
  }
  else
  {
    T y2 = y;
    int8 sx = __SETS__(x);
    return ((1 ^ sx) ^ __SETS__(y2)) & (sx ^ __SETS__(x+y2));
  }
}

int64_t __fastcall compress_line_8(char *__src, int64_t a2, unsigned int *a3)
{
  char *v4; // rsi
  char *v5; // r13
  int64_t result; // rax
  char v7; // r14
  char *v8; // rbx
  int v9; // r15d
  int v10; // edi
  int v11; // r12d
  char v12; // cc
  unsigned int v13; // r12d
  unsigned int *v14; // [rsp+0h] [rbp-40h]
  int v15; // [rsp+8h] [rbp-38h]

  if ( a2 <= 0 )
    return 0LL;
  v4 = __src;
  v5 = &__src[a2];
  result = 0LL;
  do
  {
    v7 = *v4;
    v8 = v4 + 1;
    v9 = 1;
    v10 = 0;
    if ( v4 + 1 < v5 )
    {
      v9 = 1;
      do
      {
        if ( *v8 == v7 )
        {
          ++v9;
        }
        else
        {
          if ( v9 > 9 )
            goto LABEL_11;
          v10 += v9;
          v9 = 1;
          v7 = *v8;
        }
        ++v8;
      }
      while ( v5 != v8 );
      v8 = v5;
    }
LABEL_11:
    v11 = 0;
    if ( v9 < 10 )
      v11 = v9;
    if ( v9 < 10 )
      v9 = 0;
    v12 = (v10 + v11 < 0) ^ __OFADD__(v10, v11) | (v10 + v11 == 0);
    v13 = v10 + v11;
    if ( !v12 )
    {
      *a3 = v13;
      v15 = result;
      v14 = a3;
      memcpy(a3 + 1, v4, v13);
      result = v13 + v15 + 4;
      a3 = (unsigned int *)((char *)v14 + v13 + 4);
    }
    if ( v9 > 0 )
    {
      *a3 = v9 | 0x80000000;
      *((unsigned char *)a3 + 4) = v7;
      result = (unsigned int)(result + 5);
      a3 = (unsigned int *)((char *)a3 + 5);
    }
    v4 = v8;
  }
  while ( v8 < v5 );
  return result;
}

int64_t __fastcall compress_line_16(char *__src, int64_t a2, unsigned int *a3)
{
  char *v3; // r13
  int v4; // eax
  char *v5; // rsi
  __int16 v6; // cx
  char *v7; // rbx
  int v8; // r12d
  int v9; // edi
  __int16 v10; // r14
  int v11; // r15d
  char v12; // cc
  unsigned int v13; // r15d
  unsigned int *v15; // [rsp+0h] [rbp-40h]
  int v16; // [rsp+8h] [rbp-38h]

  v3 = &__src[2 * a2];
  v4 = 0;
  if ( v3 > __src )
  {
    v5 = __src;
    v4 = 0;
    do
    {
      v6 = *(uint32_t *)v5;
      v7 = v5 + 2;
      v8 = 1;
      v9 = 0;
      if ( v5 + 2 >= v3 )
      {
LABEL_11:
        v10 = v6;
      }
      else
      {
        v8 = 1;
        do
        {
          v10 = *(uint32_t *)v7;
          if ( *(uint32_t *)v7 == v6 )
          {
            ++v8;
            v10 = v6;
          }
          else
          {
            if ( v8 > 5 )
              goto LABEL_11;
            v9 += v8;
            v8 = 1;
            v6 = *(uint32_t *)v7;
          }
          v7 += 2;
        }
        while ( v7 < v3 );
      }
      v11 = 0;
      if ( v8 < 6 )
        v11 = v8;
      if ( v8 < 6 )
        v8 = 0;
      v12 = (v9 + v11 < 0) ^ __OFADD__(v9, v11) | (v9 + v11 == 0);
      v13 = v9 + v11;
      if ( !v12 )
      {
        *a3 = v13;
        v16 = v4;
        v15 = a3;
        memcpy(a3 + 1, v5, 2LL * v13);
        v4 = v13 + v16 + 2;
        a3 = (unsigned int *)((char *)v15 + 2 * v13 + 4);
      }
      if ( v8 > 0 )
      {
        *a3 = v8 | 0x80000000;
        *((uint32_t *)a3 + 2) = v10;
        v4 += 3;
        a3 = (unsigned int *)((char *)a3 + 6);
      }
      v5 = v7;
    }
    while ( v7 < v3 );
  }
  return (unsigned int)(2 * v4);
}

int64_t __fastcall compress_line_32(char *__src, int64_t a2, unsigned int *a3)
{
  char *v3; // r13
  int v4; // eax
  char *v5; // rsi
  unsigned int v6; // ecx
  char *v7; // rbx
  int v8; // r12d
  int v9; // edi
  unsigned int v10; // r14d
  int v11; // r15d
  char v12; // cc
  unsigned int v13; // r15d
  unsigned int *v15; // [rsp+8h] [rbp-38h]
  int v16; // [rsp+10h] [rbp-30h]

  v3 = &__src[4 * a2];
  v4 = 0;
  if ( v3 <= __src )
    return (unsigned int)(4 * v4);
  v5 = __src;
  v4 = 0;
  do
  {
    v6 = *(unsigned int *)v5;
    v7 = v5 + 4;
    v8 = 1;
    if ( v5 + 4 >= v3 )
    {
      v10 = *(unsigned int *)v5;
      v9 = 0;
      goto LABEL_13;
    }
    v9 = 0;
    v8 = 1;
    while ( 1 )
    {
      v10 = *(unsigned int *)v7;
      if ( *(unsigned int *)v7 != v6 )
        break;
      ++v8;
      v10 = v6;
LABEL_9:
      v7 += 4;
      if ( v7 >= v3 )
        goto LABEL_13;
    }
    if ( v8 <= 3 )
    {
      v9 += v8;
      v8 = 1;
      v6 = *(unsigned int *)v7;
      goto LABEL_9;
    }
    v10 = v6;
LABEL_13:
    v11 = 0;
    if ( v8 < 4 )
      v11 = v8;
    if ( v8 < 4 )
      v8 = 0;
    v12 = (v9 + v11 < 0) ^ __OFADD__(v9, v11) | (v9 + v11 == 0);
    v13 = v9 + v11;
    if ( !v12 )
    {
      *a3 = v13;
      v16 = v4;
      v15 = a3;
      memcpy(a3 + 1, v5, (int)(4 * v13));
      v4 = v13 + v16 + 1;
      a3 = &v15[v13 + 1];
    }
    if ( v8 > 0 )
    {
      *a3 = v8 | 0x80000000;
      a3[1] = v10;
      v4 += 2;
      a3 += 2;
    }
    v5 = v7;
  }
  while ( v7 < v3 );
  return (unsigned int)(4 * v4);
}

//https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/bcmp.3.html
extern int bcmp(const void *s1, const void *s2, size_t n);

int64_t __fastcall pk_compressData(
        int64_t src,
        unsigned int method,
        int64_t a3,
        int64_t a4,
        int64_t a5,
        unsigned int *dest,
        uint64_t dst_len)
{
  uint64_t scan_line; // r8
  uint64_t y_len; // rcx
  unsigned int v9; // r14d
  int64_t v10; // r10
  int64_t (__fastcall *fn_compress)(char *, int64_t a2, unsigned int *a3); // rcx
  int64_t (__fastcall *v12)(char *, int64_t a2, unsigned int *a3); // rax
  int64_t v13; // rcx
  int64_t v14; // r13
  unsigned int *line_dest; // rbx
  int v16; // r12d
  size_t v17; // r14
  int64_t v18; // r15
  int v19; // r14d
  uint64_t v22; // [rsp+8h] [rbp-68h]
  uint32_t *v23; // [rsp+10h] [rbp-60h]
  int64_t (__fastcall *compress)(char *, int64_t a2, unsigned int *a3); // [rsp+18h] [rbp-58h]
  void *v25; // [rsp+28h] [rbp-48h]
  unsigned int *v26; // [rsp+30h] [rbp-40h]
  int64_t v27; // [rsp+38h] [rbp-38h]

  scan_line = dst_len;
  y_len = 4 * a4 + 12;
  if ( y_len >= dst_len )
  {
    v9 = 0;
    //_CUILog(
    //  4,
    //  (unsigned int)"compressData: destinatinon buffer size %ld too small for y index (%ld)\n",
    //  a7,
    //  v8,
    //  a7,
    //  (uint32_t)a6);
  }
  else
  {
    if ( method == 6 )
    {
      a3 *= 4LL;
    }
    else if ( method == 5 )
    {
      a3 *= 2LL;
    }
    *dest = method;
    dest[1] = a3;
    dest[2] = a4;
    v10 = (int64_t)&dest[a4 + 3];
    if ( a4 )
    {
      v23 = dest + 3;
      v22 = 4 * (a3 + ((uint64_t)(a3 + 1) >> 1) + 1);

      // default
      fn_compress = compress_line_32;
      if ( method == 3 )
        fn_compress = compress_line_16;
      v12 = compress_line_8;
      if ( method >= 3 )
        v12 = fn_compress;
      
      compress = v12;
      v13 = -1LL;
      v14 = 0LL;
      line_dest = &dest[a4 + 3];
      v26 = dest;
      v27 = a3;
      while ( 1 )
      {
        v16 = (uint32_t)line_dest - (uint32_t)dest;
        if ( (char *)line_dest - (char *)dest + v22 > scan_line )
          break;
        v25 = (void *)v10;
        v17 = v13;
        v18 = ((int (__fastcall *)(int64_t, int64_t, unsigned int *))compress)(src, a3, line_dest);
        if ( v17 == v18 && !bcmp(v25, line_dest, v17) )
        {
          v13 = v17;
          v10 = (int64_t)v25;
          dest = v26;
          v16 = (uint32_t)v25 - (uint32_t)v26;
          v19 = (int)line_dest;
          scan_line = dst_len;
        }
        else
        {
          v19 = (uint32_t)line_dest + v18;
          v13 = v18;
          v10 = (int64_t)line_dest;
          line_dest = (unsigned int *)((char *)line_dest + v18);
          scan_line = dst_len;
          dest = v26;
        }
        a3 = v27;
        v23[v14++] = v16;
        src += a5;
        if ( a4 == v14 )
          return (unsigned int)(v19 - (uint32_t)dest);
      }
      v9 = 0;
      //_CUILog(
      //  4,
      //  (unsigned int)"compressData: overflow: %ld bytes in %ld byte buffer at scanline %ld (of %ld).\n",
      //  (uint32_t)v15 - (uint32_t)a6,
      //  v7,
      //  v14,
      //  a4);
    }
    else
    {
      v19 = (uint32_t)dest + 12;
      return (unsigned int)(v19 - (uint32_t)dest);
    }
  }
  return v9;
}