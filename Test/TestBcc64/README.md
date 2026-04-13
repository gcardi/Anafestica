# TestBcc64

Compatibility-focused test suite for legacy `bcc64` (Boost variant path).

Goals:

- Reproduce legacy compiler regressions deterministically.
- Keep tests minimal and focused on variant construction/get semantics.
- Avoid coupling with modern `bcc64x`-only behavior.

Suggested initial cases:

- Primitive `PutItem`/`GetItem` roundtrips for `int`, `long`, `unsigned long`.
- Enum roundtrip via RTTI and fallback integer path.
- Type mismatch behavior (`GetItem<T>` returns `T{}` when stored alternative differs).

Current status:

- Folder scaffold created.
- Existing full suite moved to `Test/TestBcc64x`.
