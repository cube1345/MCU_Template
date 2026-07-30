# Engineering Review Checklist

## Abstraction Fit

- Is substitution, portability, or testability a real requirement?
- Would a plain C module be simpler and sufficient?
- Is inheritance limited to one embedded base and independent behavior composed?

## API and Dispatch

- Are public symbols prefixed and headers minimal/self-contained?
- Are helpers, concrete instances, and ops tables private and `static`?
- Does application code use only base/interface handles?
- Are vendor SDK types absent from reusable driver APIs?
- Is the base first by default, upcasts written as `&child.base`, and downcasts
  performed with `container_of`?
- Are ops tables `static const`, designated-initialized, and validated at init?
- Do wrappers centralize state checks and dispatch policy?
- Can multiple instances run without unintended shared state?

## Lifecycle, Errors, and Concurrency

- Is storage ownership explicit and heap policy obeyed?
- Are init, partial failure, retry, deinit, and reverse-order cleanup defined?
- Are external inputs validated and internal invariants asserted safely?
- Are meaningful errors propagated without silent success?
- Are callback context, thread/ISR safety, blocking behavior, and synchronization
  documented? Is `volatile` avoided as the sole synchronization mechanism?

## Platform and Verification

- Are HAL/register calls confined to concrete drivers or platform backends?
- Are vendor HAL types absent from reusable base and application APIs?
- Is startup order explicit, duplicate-safe, and checked for errors?
- If linker sections are used, are boundaries, LTO, GC, and toolchains verified?
- Does code compile with strict repository warnings and have a target build,
  flash, or board-level smoke-test path? Are unverified hardware paths reported?
