## C API Style

### New Types

Types can be identified their copy costs via suffix, `_s` for struct, `_u` for
union, `_fnt` for function pointer, and `_t` for primitive types.

### Result Code and Context

#### Result Code

All APIs which expect to have fail modes should use result code `rc_t` as return
types. Unless special values are documented, only zero value `RC_OK` means OK
(successful).

#### Context and Associated APIs

All APIs using result code `rc_t` are expected to take a `ctx_s` as its first
argument. If non-`NULL`, error message can be recorded if new diagnosis note is
needed; otherwise, all functions are ignored.

Context `ctx_s` has few APIs to make coding easier.
```
/*
 * NOTE: ctx_set_xxx and ctx_get_xxx are used to store private data `udp`.
 * Settting with NULL clears the data. Returning NULL means no data. It manages
 * the ownership of the string key `zKey` internally. If `udp_free_fn` is not
 * NULL, `udp` will be cleaned when `ctx_free` is called, new data is set with
 * the same `zkey`, or the data is cleared (`udp==NULL`).
 *
 * In future, ctx can have modes to support concurrent accesses, malloc/free
   fns, copy-less key, etc.
 */
ctx_new( )             -> ctx_s
ctx_free( ctx_s* )
ctx_emit_note( ctx_s*, const char*, ... )
ctx_set_data( ctx_s*, const char* zKey, void* udp, void (*udp_free_fn)(void*) );
ctx_get_data( ctx_s*, const char* zKey );
```
